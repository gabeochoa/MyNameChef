#pragma once

#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

struct StartCourseSystem : afterhours::System<CombatQueue> {
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

    // Check if any dishes are currently entering or in combat
    bool any_active = false;
    for (auto &ref :
         afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
      auto &e = ref.get();
      auto &dbs = e.get<DishBattleState>();
      if (dbs.phase == DishBattleState::Phase::Entering ||
          dbs.phase == DishBattleState::Phase::InCombat) {
        any_active = true;
      }
    }

    if (any_active) {
      // (quiet)
      return; // Wait for current course to finish
    }

    // Find player and opponent dishes for current slot
    auto player_dish =
        find_dish_for_slot(cq.current_index, DishBattleState::TeamSide::Player);
    auto opponent_dish = find_dish_for_slot(
        cq.current_index, DishBattleState::TeamSide::Opponent);

    // (quiet)

    if (player_dish && opponent_dish) {
      const float enter_start_delay =
          0.25f; // delay after slide-in before enter animation
      // Start both dishes entering
      auto &player_dbs = player_dish.value()->get<DishBattleState>();
      auto &opponent_dbs = opponent_dish.value()->get<DishBattleState>();

      player_dbs.phase = DishBattleState::Phase::Entering;
      player_dbs.enter_progress = -enter_start_delay;

      opponent_dbs.phase = DishBattleState::Phase::Entering;
      opponent_dbs.enter_progress = -enter_start_delay;

      log_info("COMBAT: Starting course {}", cq.current_index);
    } else {
      // No more dishes, mark combat complete and transition to results
      cq.complete = true;
      log_info("COMBAT: No more dishes for slot {}, transitioning to results",
               cq.current_index);
      GameStateManager::get().to_results();
    }
  }

private:
  std::optional<afterhours::Entity *>
  find_dish_for_slot(int slot_index, DishBattleState::TeamSide side) {
    for (auto &ref :
         afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
      auto &e = ref.get();
      auto &dbs = e.get<DishBattleState>();
      if (dbs.team_side == side && dbs.queue_index == slot_index &&
          dbs.phase == DishBattleState::Phase::InQueue) {
        return &e;
      }
    }
    return std::nullopt;
  }
};
