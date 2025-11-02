#pragma once

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
    // Check for blocking animations - in headless mode hasActiveAnimation() returns false
    return !hasActiveAnimation();
  }

  void for_each_with(afterhours::Entity &, CombatQueue &cq, float) override {
    if (cq.complete) {
      // Quiet skip once combat is complete
      return;
    }

    bool prereqs = prerequisites_complete();
    if (!prereqs) {
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

      if (!player_has_remaining || !opponent_has_remaining) {
        cq.complete = true;

        uint64_t fp = BattleFingerprint::compute();
        log_info("AUDIT_FP checkpoint=end hash={}", fp);

        GameStateManager::get().to_results();
        return;
      }

      // Both teams have dishes but not at index 0 yet (reorganization pending)
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

    const float enter_start_delay = 0.25f;

    if (player_dbs.phase == DishBattleState::Phase::InQueue &&
        opponent_dbs.phase == DishBattleState::Phase::InQueue) {
      player_dbs.phase = DishBattleState::Phase::Entering;
      player_dbs.enter_progress = -enter_start_delay;
      opponent_dbs.phase = DishBattleState::Phase::Entering;
      opponent_dbs.enter_progress = -enter_start_delay;
      cq.current_index++;

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
    return EQ().whereHasComponent<DishBattleState>()
               .whereTeamSide(side)
               .whereLambda([](const afterhours::Entity &e) {
                 const DishBattleState &dbs = e.get<DishBattleState>();
                 return dbs.phase == DishBattleState::Phase::InQueue ||
                        dbs.phase == DishBattleState::Phase::Entering ||
                        dbs.phase == DishBattleState::Phase::InCombat;
               })
               .gen_count() > 0;
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
