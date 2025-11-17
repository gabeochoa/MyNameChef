#pragma once

#include "../components/dish_battle_state.h"
#include "../components/drink_effects.h"
#include "../components/drink_pairing.h"
#include "../components/is_dish.h"
#include "../game_state_manager.h"
#include "../log.h"
#include "../query.h"
#include <afterhours/ah.h>

struct ApplyDrinkPairingEffects
    : afterhours::System<IsDish, DishBattleState, DrinkPairing> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &e, IsDish &, DishBattleState &dbs,
                     DrinkPairing &drink_pairing, float) override {
    if (dbs.phase != DishBattleState::Phase::InQueue) {
      return;
    }

    if (e.has<DrinkEffects>()) {
      return;
    }

    if (!drink_pairing.drink.has_value()) {
      return;
    }

    e.addComponent<DrinkEffects>();
  }
};
