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
      bool player_has_remaining =
          has_remaining_active_dishes(DishBattleState::TeamSide::Player);
      bool opponent_has_remaining =
          has_remaining_active_dishes(DishBattleState::TeamSide::Opponent);

      log_info("COMBAT_START: No dishes at index 0 - Player has remaining: {}, Opponent has remaining: {}", 
               player_has_remaining, opponent_has_remaining);

      if (!player_has_remaining || !opponent_has_remaining) {
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
    int count = static_cast<int>(EQ().whereHasComponent<DishBattleState>()
               .whereTeamSide(side)
               .whereLambda([](const afterhours::Entity &e) {
                 const DishBattleState &dbs = e.get<DishBattleState>();
                 return dbs.phase == DishBattleState::Phase::InQueue ||
                        dbs.phase == DishBattleState::Phase::Entering ||
                        dbs.phase == DishBattleState::Phase::InCombat;
               })
               .gen_count());
    return count > 0;
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
      return false;
    }

    // Check for blocking animations - in headless mode hasActiveAnimation() returns false
    if (hasActiveAnimation()) {
      return false;
    }

    return true;
  }
};
