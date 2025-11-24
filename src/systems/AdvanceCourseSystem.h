#pragma once

#include "../components/combat_queue.h"
#include "../components/combat_stats.h"
#include "../components/dish_battle_state.h"
#include "../components/transform.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../render_backend.h"
#include "../render_constants.h"
#include "../shop.h"
#include "../systems/SimplifiedOnServeSystem.h"
#include "../utils/battle_fingerprint.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/texture_manager.h>

struct AdvanceCourseSystem : afterhours::System<CombatQueue> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Battle) {
      return false;
    }
    if (isReplayPaused()) {
      return false;
    }
    return true;
  }

  void for_each_with(afterhours::Entity &, CombatQueue &cq, float) override {
    if (cq.complete) {
      return;
    }

    if (both_dishes_finished(cq)) {
      log_info("COMBAT: Course {} finished (both dishes at index 0)",
               cq.current_index);

      if (auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
          tq.get().has<TriggerQueue>()) {
        auto &queue = tq.get().get<TriggerQueue>();
        queue.add_event(TriggerHook::OnCourseComplete, 0, 0,
                        DishBattleState::TeamSide::Player);
        log_info("COMBAT: Fired OnCourseComplete trigger for index 0");
      }

      uint64_t fp = BattleFingerprint::compute();
      log_info("AUDIT_FP checkpoint=course_{} hash={}", cq.current_index, fp);

      // Don't reorganize here - ResolveCombatTickSystem already did it when
      // dishes were defeated

      int player_active_count = 0;
      int opponent_active_count = 0;
      bool has_remaining_dishes = false;
      for (DishBattleState::TeamSide side :
           {DishBattleState::TeamSide::Player,
            DishBattleState::TeamSide::Opponent}) {
        afterhours::RefEntities all_dishes =
            EQ({.ignore_temp_warning = true})
                .whereHasComponent<DishBattleState>()
                .whereTeamSide(side)
                .gen();
        
        int active_count = 0;
        for (afterhours::Entity &dish : all_dishes) {
          const DishBattleState &dbs = dish.get<DishBattleState>();
          if (dbs.phase != DishBattleState::Phase::Finished) {
            active_count++;
          }
        }
        
        if (side == DishBattleState::TeamSide::Player) {
          player_active_count = active_count;
        } else {
          opponent_active_count = active_count;
        }
        if (active_count > 0) {
          has_remaining_dishes = true;
        }
      }

      if (!has_remaining_dishes) {
        cq.complete = true;
        log_info("COMBAT: All courses complete - no remaining dishes (Player: "
                 "{}, Opponent: {})",
                 player_active_count, opponent_active_count);
        GameStateManager::get().to_results();
      } else if (player_active_count == 0 || opponent_active_count == 0) {
        cq.complete = true;
        log_info("COMBAT: Battle complete - one team exhausted (Player: "
                 "{}, Opponent: {})",
                 player_active_count, opponent_active_count);
        GameStateManager::get().to_results();
      } else if (cq.current_index >= cq.total_courses - 1) {
        // Reached max courses, but both teams still have dishes
        // Don't mark complete - let the battle continue until one team is exhausted
        // This can happen in survivor carryover scenarios where dishes fight multiple courses
        log_info("COMBAT: Reached max courses {} but both teams have dishes "
                 "(Player: {} active, Opponent: {} active) - continuing battle",
                 cq.total_courses, player_active_count, opponent_active_count);
        // Extend total_courses to allow battle to continue
        cq.total_courses = cq.current_index + 2;
        cq.current_index++;
        // Clear tracked dish IDs so the next course can track its own dishes
        cq.current_player_dish_id = std::nullopt;
        cq.current_opponent_dish_id = std::nullopt;
        log_info("COMBAT: Extended total_courses to {} and advancing to course {}",
                 cq.total_courses, cq.current_index);
        
        // Reset OnServeState so SimplifiedOnServeSystem can fire OnServe
        for (afterhours::Entity &e :
             afterhours::EntityQuery({.ignore_temp_warning = true})
                 .whereHasComponent<OnServeState>()
                 .gen()) {
          OnServeState &state = e.get<OnServeState>();
          state.allFired = false;
          log_info("COMBAT: Reset OnServeState.allFired=false for course {}", cq.current_index);
        }
      } else {
        cq.current_index++;
        // Clear tracked dish IDs so the next course can track its own dishes
        cq.current_player_dish_id = std::nullopt;
        cq.current_opponent_dish_id = std::nullopt;
        log_info("COMBAT: Advancing to course {} - Player: {} active dishes, "
                 "Opponent: {} active dishes",
                 cq.current_index, player_active_count, opponent_active_count);
        
        // CRITICAL: Reset OnServeState so SimplifiedOnServeSystem can fire OnServe
        // for dishes that were reset to InQueue after reorganization
        // Dishes that survive a course are reset to InQueue and need OnServe to fire again
        for (afterhours::Entity &e :
             afterhours::EntityQuery({.ignore_temp_warning = true})
                 .whereHasComponent<OnServeState>()
                 .gen()) {
          OnServeState &state = e.get<OnServeState>();
          state.allFired = false;
          log_info("COMBAT: Reset OnServeState.allFired=false for course {}", cq.current_index);
        }
      }
    }
  }

private:
  bool both_dishes_finished(const CombatQueue &cq) {
    // Check if the dishes that were fighting in this course are now Finished
    // We track these dish IDs in CombatQueue when the course starts
    if (!cq.current_player_dish_id.has_value() ||
        !cq.current_opponent_dish_id.has_value()) {
      // No dishes tracked yet, course hasn't started
      log_info("COMBAT_ADVANCE: No dishes tracked for current course");
      return false;
    }

    int player_dish_id = cq.current_player_dish_id.value();
    int opponent_dish_id = cq.current_opponent_dish_id.value();

    // Find the tracked dishes and check if they're Finished
    bool player_finished = false;
    bool opponent_finished = false;

    // Use EntityHelper to get entities by ID
    auto player_entity_opt =
        afterhours::EntityHelper::getEntityForID(player_dish_id);
    if (player_entity_opt) {
      const afterhours::Entity &player_entity = player_entity_opt.asE();
      const DishBattleState &dbs = player_entity.get<DishBattleState>();
      // Check both phase and CombatStats - dish is finished if phase is
      // Finished OR currentBody <= 0
      bool phase_finished = dbs.phase == DishBattleState::Phase::Finished;
      bool stats_defeated = false;
      if (player_entity.has<CombatStats>()) {
        const CombatStats &cs = player_entity.get<CombatStats>();
        stats_defeated = cs.currentBody <= 0;
      }
      player_finished = phase_finished || stats_defeated;
      log_info("COMBAT_ADVANCE: Tracked player dish {} - phase: {}, "
               "currentBody: {}, finished: {}",
               player_dish_id, static_cast<int>(dbs.phase),
               player_entity.has<CombatStats>()
                   ? player_entity.get<CombatStats>().currentBody
                   : -1,
               player_finished);
    } else {
      log_info("COMBAT_ADVANCE: Tracked player dish {} not found - treating as finished",
               player_dish_id);
      player_finished = true;
    }

    auto opponent_entity_opt =
        afterhours::EntityHelper::getEntityForID(opponent_dish_id);
    if (opponent_entity_opt) {
      const afterhours::Entity &opponent_entity = opponent_entity_opt.asE();
      const DishBattleState &dbs = opponent_entity.get<DishBattleState>();
      // Check both phase and CombatStats - dish is finished if phase is
      // Finished OR currentBody <= 0
      bool phase_finished = dbs.phase == DishBattleState::Phase::Finished;
      bool stats_defeated = false;
      if (opponent_entity.has<CombatStats>()) {
        const CombatStats &cs = opponent_entity.get<CombatStats>();
        stats_defeated = cs.currentBody <= 0;
      }
      opponent_finished = phase_finished || stats_defeated;
      log_info("COMBAT_ADVANCE: Tracked opponent dish {} - phase: {}, "
               "currentBody: {}, finished: {}",
               opponent_dish_id, static_cast<int>(dbs.phase),
               opponent_entity.has<CombatStats>()
                   ? opponent_entity.get<CombatStats>().currentBody
                   : -1,
               opponent_finished);
    } else {
      log_info("COMBAT_ADVANCE: Tracked opponent dish {} not found - treating as finished",
               opponent_dish_id);
      opponent_finished = true;
    }

    // Course is finished when either dish is Finished (one defeated means course is done)
    // The surviving dish will continue to the next course
    if (player_finished || opponent_finished) {
      log_info(
          "COMBAT_ADVANCE: Course complete - player finished: {}, opponent finished: {}",
          player_finished, opponent_finished);
      return true;
    }

    return false;
  }

  void reorganize_queues() {
    for (DishBattleState::TeamSide side :
         {DishBattleState::TeamSide::Player,
          DishBattleState::TeamSide::Opponent}) {
      afterhours::RefEntities active_dishes =
          EQ({.ignore_temp_warning = true})
              .whereHasComponent<DishBattleState>()
              .whereTeamSide(side)
              .whereLambda([](const afterhours::Entity &e) {
                const DishBattleState &dbs = e.get<DishBattleState>();
                return dbs.phase == DishBattleState::Phase::InQueue ||
                       dbs.phase == DishBattleState::Phase::Entering ||
                       dbs.phase == DishBattleState::Phase::InCombat;
              })
              .orderByLambda(
                  [](const afterhours::Entity &a, const afterhours::Entity &b) {
                    return a.get<DishBattleState>().queue_index <
                           b.get<DishBattleState>().queue_index;
                  })
              .gen();

      int new_index = 0;
      for (afterhours::Entity &dish : active_dishes) {
        DishBattleState &dbs = dish.get<DishBattleState>();
        log_info(
            "COMBAT_ADVANCE: Reorganizing {} side dish {} - current phase: {}, "
            "queue_index: {} -> {}",
            side == DishBattleState::TeamSide::Player ? "Player" : "Opponent",
            dish.id, static_cast<int>(dbs.phase), dbs.queue_index, new_index);

        if (dbs.phase == DishBattleState::Phase::Finished) {
          log_info(
              "COMBAT_ADVANCE: Skipping {} side dish {} - already Finished",
              side == DishBattleState::TeamSide::Player ? "Player" : "Opponent",
              dish.id);
          continue;
        }

        bool was_reorganized = dbs.queue_index != new_index;
        if (was_reorganized) {
          log_info("COMBAT: Reorganizing {} side dish {} from index {} to {}",
                   side == DishBattleState::TeamSide::Player ? "Player"
                                                             : "Opponent",
                   dish.id, dbs.queue_index, new_index);
          dbs.queue_index = new_index;

          if (dish.has<Transform>()) {
            float y = (side == DishBattleState::TeamSide::Player) ? 150.0f : 500.0f;
            float x = 120.0f + new_index * 100.0f;
            Transform &transform = dish.get<Transform>();
            transform.position = afterhours::vec2{x, y};

            if (dish.has<afterhours::texture_manager::HasSprite>()) {
              dish.get<afterhours::texture_manager::HasSprite>().update_transform(
                  transform.position, transform.size, transform.angle);
            }
          }
        }

        if (dbs.phase == DishBattleState::Phase::Entering) {
          log_info("COMBAT_ADVANCE: Resetting {} side dish {} from Entering to "
                   "InQueue",
                   side == DishBattleState::TeamSide::Player ? "Player"
                                                             : "Opponent",
                   dish.id);
          dbs.phase = DishBattleState::Phase::InQueue;
          dbs.enter_progress = 0.0f;
        }

        new_index++;
      }
    }
  }
};
