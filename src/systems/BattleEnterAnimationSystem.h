#pragma once

#include "../components/dish_battle_state.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

struct BattleEnterAnimationSystem : afterhours::System<DishBattleState> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &e, DishBattleState &dbs,
                     float dt) override {
    if (dbs.phase != DishBattleState::Phase::Entering)
      return;

    const float enter_duration = 0.45f; // seconds
    dbs.enter_progress =
        std::min(1.0f, dbs.enter_progress + dt / enter_duration);

    if (dbs.enter_progress >= 1.0f) {
      dbs.phase = DishBattleState::Phase::InCombat;
      dbs.bite_timer = 0.0f;
    }
  }
};
