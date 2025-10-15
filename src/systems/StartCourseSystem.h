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

    // log_info("COMBAT: StartCourseSystem checking course {} (index {})",
    //          cq.current_index + 1, cq.current_index);

    // Check if any dishes are currently entering or in combat
    bool any_active = false;
    int active_count = 0;
    for (auto &ref :
         afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
      auto &e = ref.get();
      auto &dbs = e.get<DishBattleState>();
      if (dbs.phase == DishBattleState::Phase::Entering ||
          dbs.phase == DishBattleState::Phase::InCombat) {
        any_active = true;
        active_count++;
        // log_info("COMBAT: Found active dish {} - team: {}, slot: {}, phase: {}",
        //          e.id,
        //          (dbs.team_side == DishBattleState::TeamSide::Player)
        //              ? "Player"
        //              : "Opponent",
        //          dbs.queue_index,
        //          (dbs.phase == DishBattleState::Phase::Entering) ? "Entering"
        //                                                          : "InCombat");
      }
    }

    if (any_active) {
      // log_info("COMBAT: {} dishes still active, waiting for current course to finish", active_count);
      return; // Wait for current course to finish
    }

    // Find player and opponent dishes for current slot
    auto player_dish =
        find_dish_for_slot(cq.current_index, DishBattleState::TeamSide::Player);
    auto opponent_dish = find_dish_for_slot(
        cq.current_index, DishBattleState::TeamSide::Opponent);

    // log_info("COMBAT: Looking for dishes in slot {} - Player: {}, Opponent: {}",
    //          cq.current_index, player_dish ? "found" : "missing",
    //          opponent_dish ? "found" : "missing");

    if (player_dish && opponent_dish) {
      // Start both dishes entering
      auto &player_dbs = player_dish.value()->get<DishBattleState>();
      auto &opponent_dbs = opponent_dish.value()->get<DishBattleState>();

      player_dbs.phase = DishBattleState::Phase::Entering;
      player_dbs.enter_progress = 0.0f;

      opponent_dbs.phase = DishBattleState::Phase::Entering;
      opponent_dbs.enter_progress = 0.0f;

      // log_info("COMBAT: Started course {} - Player entity {} (slot {}), Opponent entity {} (slot {})",
      //          cq.current_index + 1, player_dish.value()->id,
      //          player_dbs.queue_index, opponent_dish.value()->id,
      //          opponent_dbs.queue_index);
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
