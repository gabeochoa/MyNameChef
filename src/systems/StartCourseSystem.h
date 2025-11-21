#pragma once

#include "../battle_timing.h"
#include "../components/animation_event.h"
#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../components/pairing_clash_modifiers.h"
#include "../components/persistent_combat_modifiers.h"
#include "../components/pre_battle_modifiers.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../render_backend.h"
#include "../shop.h"
#include "../utils/battle_fingerprint.h"
#include <afterhours/ah.h>

struct StartCourseSystem : afterhours::System<CombatQueue> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Battle) {
      return false;
    }
    if (isReplayPaused()) {
      return false;
    }
    // CRITICAL: Don't check hasActiveAnimation() here - prerequisites_complete() already checks SlideIn
    // FreshnessChain and StatBoost animations can be active without blocking course start
    // Only SlideIn needs to complete before starting courses
    return true;
  }

  void for_each_with(afterhours::Entity &, CombatQueue &cq, float) override {
    if (cq.complete) {
      log_info("COMBAT_START: CombatQueue is complete, skipping");
      return;
    }

    if (!prerequisites_complete()) {
      static int skip_count = 0;
      skip_count++;
      if (skip_count % 60 == 0) {
        log_info("COMBAT_START: Prerequisites not complete for course {}, skipping (check {})", 
                 cq.current_index, skip_count);
      }
      return;
    }

    // Always fight dishes at index 0 (queues are reorganized when dishes
    // finish)
    afterhours::OptEntity player_dish =
        find_dish_at_index_zero(DishBattleState::TeamSide::Player);
    afterhours::OptEntity opponent_dish =
        find_dish_at_index_zero(DishBattleState::TeamSide::Opponent);

    // Check if battle should end (one team has no remaining dishes)
    if (!player_dish || !opponent_dish) {
      // CRITICAL: Before checking if teams have remaining dishes, we need to ensure
      // that Finished dishes with body remaining have been reset by reorganization.
      // Reorganization happens in ResolveCombatTickSystem, but StartCourseSystem might
      // run before it completes. So we check for Finished dishes with body and reset them here.
      for (DishBattleState::TeamSide side :
           {DishBattleState::TeamSide::Player,
            DishBattleState::TeamSide::Opponent}) {
        afterhours::RefEntities all_dishes = EQ().whereHasComponent<DishBattleState>()
                   .whereTeamSide(side)
                   .gen();
        
        for (afterhours::RefEntity dish_ref : all_dishes) {
          afterhours::Entity &dish = dish_ref.get();
          DishBattleState &dbs = dish.get<DishBattleState>();
          if (dbs.phase == DishBattleState::Phase::Finished) {
            if (dish.has<CombatStats>()) {
              const CombatStats &cs = dish.get<CombatStats>();
              log_info("COMBAT_START: Found {} side Finished dish {} with currentBody: {}",
                       side == DishBattleState::TeamSide::Player ? "Player" : "Opponent",
                       dish.id, cs.currentBody);
              if (cs.currentBody > 0) {
                // Reset this dish - it shouldn't be Finished
                dbs.phase = DishBattleState::Phase::InQueue;
                dbs.enter_progress = 0.0f;
                dbs.first_bite_decided = false;
                dbs.bite_cadence = DishBattleState::BiteCadence::PrePause;
                dbs.bite_cadence_timer = 0.0f;
                dbs.onserve_fired = false;
                log_info("COMBAT_START: Reset {} side dish {} from Finished to InQueue (has {} body remaining)",
                         side == DishBattleState::TeamSide::Player ? "Player" : "Opponent",
                         dish.id, cs.currentBody);
              } else {
                log_info("COMBAT_START: {} side dish {} is Finished with currentBody <= 0, not resetting",
                         side == DishBattleState::TeamSide::Player ? "Player" : "Opponent",
                         dish.id);
              }
            } else {
              log_info("COMBAT_START: {} side Finished dish {} has no CombatStats",
                       side == DishBattleState::TeamSide::Player ? "Player" : "Opponent",
                       dish.id);
            }
          }
        }
      }
      
      bool player_has_remaining =
          has_remaining_active_dishes(DishBattleState::TeamSide::Player);
      bool opponent_has_remaining =
          has_remaining_active_dishes(DishBattleState::TeamSide::Opponent);

      log_info("COMBAT_START: No dishes at index 0 - Player has remaining: {}, Opponent has remaining: {}", 
               player_has_remaining, opponent_has_remaining);

      // CRITICAL: Only mark battle complete if BOTH teams have no remaining dishes
      // OR if we've reached the end of all courses. If one team has no remaining,
      // the battle should end, but we need to be careful about timing - dishes
      // might be in transition (reorganization) and not at index 0 yet.
      if (!player_has_remaining && !opponent_has_remaining) {
        // Both teams exhausted - battle is a tie
        cq.complete = true;
        uint64_t fp = BattleFingerprint::compute();
        log_info("AUDIT_FP checkpoint=end hash={}", fp);
        log_info("COMBAT_START: Battle ending - both teams exhausted");
        GameStateManager::get().to_results();
        return;
      } else if (!player_has_remaining || !opponent_has_remaining) {
        // One team exhausted - battle ends
        cq.complete = true;
        uint64_t fp = BattleFingerprint::compute();
        log_info("AUDIT_FP checkpoint=end hash={}", fp);
        log_info("COMBAT_START: Battle ending - Player remaining: {}, Opponent remaining: {}", 
                 player_has_remaining, opponent_has_remaining);
        GameStateManager::get().to_results();
        return;
      }

      // Both teams have dishes but not at index 0 yet (reorganization pending)
      log_info("COMBAT_START: Waiting for reorganization - dishes exist but not at index 0 yet");
      return;
    }

    DishBattleState &player_dbs = player_dish->get<DishBattleState>();
    DishBattleState &opponent_dbs = opponent_dish->get<DishBattleState>();

    // Check if both dishes for current course are finished
    if (player_dbs.phase == DishBattleState::Phase::InCombat &&
        opponent_dbs.phase == DishBattleState::Phase::InCombat) {
      // Both dishes are done, wait for combat to finish
      return;
    }

    float scaled_enter_start_delay = BattleTiming::get_enter_start_delay();

    if (player_dbs.phase == DishBattleState::Phase::InQueue &&
        opponent_dbs.phase == DishBattleState::Phase::InQueue) {
      player_dbs.phase = DishBattleState::Phase::Entering;
      player_dbs.enter_progress = -scaled_enter_start_delay;
      opponent_dbs.phase = DishBattleState::Phase::Entering;
      opponent_dbs.enter_progress = -scaled_enter_start_delay;
      log_info("COMBAT_START: Starting course {} - Player dish {}, Opponent dish {}", 
               cq.current_index, player_dish->id, opponent_dish->id);

      // Store dish IDs for this course so AdvanceCourseSystem can track when they finish
      cq.current_player_dish_id = player_dish->id;
      cq.current_opponent_dish_id = opponent_dish->id;

      if (auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
          tq.get().has<TriggerQueue>()) {
        auto &queue = tq.get().get<TriggerQueue>();
        queue.add_event(TriggerHook::OnCourseStart, 0, 0,
                        DishBattleState::TeamSide::Player);
      }
    }
  }

private:
  afterhours::OptEntity
  find_dish_at_index_zero(DishBattleState::TeamSide side) {
    return EQ()
        .whereHasComponent<DishBattleState>()
        .whereInSlotIndex(0)
        .whereTeamSide(side)
        .whereLambda([](const afterhours::Entity &e) {
          const DishBattleState &dbs = e.get<DishBattleState>();
          return dbs.phase == DishBattleState::Phase::InQueue ||
                 dbs.phase == DishBattleState::Phase::Entering ||
                 dbs.phase == DishBattleState::Phase::InCombat;
        })
        .gen_first();
  }

  bool has_remaining_active_dishes(DishBattleState::TeamSide side) {
    // Check for any dish that isn't Finished - this includes dishes in transition
    // (reorganization) or any other active phase
    afterhours::RefEntities all_dishes = EQ().whereHasComponent<DishBattleState>()
               .whereTeamSide(side)
               .gen();
    
    int active_count = 0;
    int finished_count = 0;
    for (afterhours::Entity &dish : all_dishes) {
      const DishBattleState &dbs = dish.get<DishBattleState>();
      if (dbs.phase != DishBattleState::Phase::Finished) {
        active_count++;
      } else {
        finished_count++;
      }
    }
    
    if (active_count == 0 && all_dishes.size() > 0) {
      log_info("COMBAT_START: {} side has {} dishes, all Finished (total: {})", 
               side == DishBattleState::TeamSide::Player ? "Player" : "Opponent",
               finished_count, all_dishes.size());
    }
    
    return active_count > 0;
  }

  bool prerequisites_complete() {
    // Check if slide-in animations are complete
    bool slideInComplete = true;
    for (afterhours::Entity &animEntity :
         afterhours::EntityQuery()
             .whereHasComponent<AnimationEvent>()
             .whereHasComponent<AnimationTimer>()
             .gen()) {
      AnimationEvent &ev = animEntity.get<AnimationEvent>();
      if (ev.type == AnimationEventType::SlideIn) {
        slideInComplete = false;
        break;
      }
    }

    if (!slideInComplete) {
      static int log_count = 0;
      if (log_count++ % 60 == 0) {
        log_info("COMBAT_START_PREREQ: SlideIn not complete");
      }
      return false;
    }

    // Check if all dishes have fired OnServe
    afterhours::RefEntities unfinishedDishes =
        afterhours::EntityQuery()
            .whereHasComponent<IsDish>()
            .whereHasComponent<DishBattleState>()
            .whereLambda([](const afterhours::Entity &e) {
              const DishBattleState &dbs = e.get<DishBattleState>();
              return dbs.phase == DishBattleState::Phase::InQueue &&
                     !dbs.onserve_fired;
            })
            .gen();
    if (!unfinishedDishes.empty()) {
      static int log_count = 0;
      if (log_count++ % 60 == 0) {
        log_info("COMBAT_START_PREREQ: {} dishes in InQueue haven't fired OnServe", unfinishedDishes.size());
        for (afterhours::Entity &dish : unfinishedDishes) {
          const DishBattleState &dbs = dish.get<DishBattleState>();
          log_info("COMBAT_START_PREREQ: Dish {} (slot {}, team {}) hasn't fired OnServe", 
                   dish.id, dbs.queue_index, 
                   dbs.team_side == DishBattleState::TeamSide::Player ? "Player" : "Opponent");
        }
      }
      return false;
    }

    // Check for blocking animations - in headless mode hasActiveAnimation() returns false
    if (hasActiveAnimation()) {
      static int log_count = 0;
      if (log_count++ % 60 == 0) {
        log_info("COMBAT_START_PREREQ: Blocking animations active");
      }
      return false;
    }

    return true;
  }
};
