#pragma once

#include "../components/battle_result.h"
#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../game_state_manager.h"
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

    // log_info("COMBAT: AdvanceCourseSystem checking course {} (index {})",
    //          cq.current_index + 1, cq.current_index);

    // Check if both dishes for current course are finished
    if (both_dishes_finished(cq.current_index)) {
      // log_info("COMBAT: Both dishes finished for course {}, advancing", cq.current_index + 1);

      cq.current_index++;

      if (cq.current_index >= cq.total_courses) {
        cq.complete = true;
        // log_info("COMBAT: All courses complete, transitioning to results");
        GameStateManager::get().to_results();
      } else {
        // log_info("COMBAT: Advanced to course {} (index {})", cq.current_index + 1, cq.current_index);
      }
    } else {
      // log_info("COMBAT: Course {} not finished yet, waiting", cq.current_index + 1);
    }
  }

private:
  bool both_dishes_finished(int slot_index) {
    bool player_finished = false;
    bool opponent_finished = false;

    for (auto &ref :
         afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
      auto &e = ref.get();
      auto &dbs = e.get<DishBattleState>();

      if (dbs.queue_index == slot_index) {
        if (dbs.team_side == DishBattleState::TeamSide::Player) {
          player_finished = (dbs.phase == DishBattleState::Phase::Finished);
        } else {
          opponent_finished = (dbs.phase == DishBattleState::Phase::Finished);
        }
      }
    }

    log_info(
        "COMBAT: both_dishes_finished for slot {} - Player: {}, Opponent: {}",
        slot_index, player_finished ? "finished" : "not finished",
        opponent_finished ? "finished" : "not finished");

    return player_finished && opponent_finished;
  }
};
