#pragma once

#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../utils/battle_fingerprint.h"
#include <afterhours/ah.h>

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
      // Quiet skip once combat is complete
      return;
    }

    // (quiet)

    // Check if both dishes at index 0 are finished (queues are reorganized when
    // dishes finish)
    if (both_dishes_finished()) {
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
    }
  }

private:
  bool both_dishes_finished() {
    afterhours::OptEntity player_dish =
        EQ({.ignore_temp_warning = true})
            .whereHasComponent<DishBattleState>()
            .whereInSlotIndex(0)
            .whereTeamSide(DishBattleState::TeamSide::Player)
            .gen_first();

    afterhours::OptEntity opponent_dish =
        EQ({.ignore_temp_warning = true})
            .whereHasComponent<DishBattleState>()
            .whereInSlotIndex(0)
            .whereTeamSide(DishBattleState::TeamSide::Opponent)
            .gen_first();

    bool player_finished = player_dish.has_value() &&
                           player_dish.value()->get<DishBattleState>().phase ==
                               DishBattleState::Phase::Finished;
    bool opponent_finished =
        opponent_dish.has_value() &&
        opponent_dish.value()->get<DishBattleState>().phase ==
            DishBattleState::Phase::Finished;

    return player_finished && opponent_finished;
  }
};
