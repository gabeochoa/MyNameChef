#pragma once

#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../game_state_manager.h"
#include "../query.h"
#include <afterhours/ah.h>

struct AdvanceCourseSystem : afterhours::System<CombatQueue> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &, CombatQueue &cq, float) override {
    if (cq.complete) {
      // Quiet skip once combat is complete
      return;
    }

    // (quiet)

    // Check if both dishes for current course are finished
    if (both_dishes_finished(cq.current_index)) {
      log_info("COMBAT: Course {} finished", cq.current_index);

      cq.current_index++;

      if (cq.current_index >= cq.total_courses) {
        cq.complete = true;
        log_info("COMBAT: All courses complete; transitioning to results");
        GameStateManager::get().to_results();
      }
    }
  }

private:
  bool both_dishes_finished(int slot_index) {
    afterhours::OptEntity player_dish =
        EQ().whereHasComponent<DishBattleState>()
            .whereInSlotIndex(slot_index)
            .whereTeamSide(DishBattleState::TeamSide::Player)
            .gen_first();

    afterhours::OptEntity opponent_dish =
        EQ().whereHasComponent<DishBattleState>()
            .whereInSlotIndex(slot_index)
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
