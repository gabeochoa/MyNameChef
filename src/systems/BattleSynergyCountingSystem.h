#pragma once

#include "../components/battle_synergy_counts.h"
#include "../components/cuisine_tag.h"
#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../shop.h"
#include <afterhours/ah.h>
#include <magic_enum/magic_enum.hpp>

struct BattleSynergyCountingSystem : afterhours::System<> {
  bool calculated = false;
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();

    if (last_screen == GameStateManager::Screen::Battle &&
        gsm.active_screen != GameStateManager::Screen::Battle) {
      calculated = false;
    }

    last_screen = gsm.active_screen;
    return gsm.active_screen == GameStateManager::Screen::Battle && !calculated;
  }

  void once(float) override {
    auto battle_synergy_entity =
        afterhours::EntityHelper::get_singleton<BattleSynergyCounts>();
    if (!battle_synergy_entity.get().has<BattleSynergyCounts>()) {
      return;
    }

    auto &battle_synergy =
        battle_synergy_entity.get().get<BattleSynergyCounts>();
    battle_synergy.player_cuisine_counts.clear();
    battle_synergy.opponent_cuisine_counts.clear();

    for (afterhours::Entity &entity :
         afterhours::EntityQuery()
             .whereHasComponent<IsDish>()
             .whereHasComponent<DishBattleState>()
             .whereHasComponent<CuisineTag>()
             // TODO why is this needed?
             .whereLambda(
                 [](const afterhours::Entity &e) { return !e.cleanup; })
             .whereLambda([](const afterhours::Entity &e) {
               const auto &dbs = e.get<DishBattleState>();
               return dbs.phase == DishBattleState::Phase::InQueue ||
                      dbs.phase == DishBattleState::Phase::Entering;
             })
             .gen()) {
      const auto &dbs = entity.get<DishBattleState>();
      const auto &tag = entity.get<CuisineTag>();
      auto &counts = (dbs.team_side == DishBattleState::TeamSide::Player)
                         ? battle_synergy.player_cuisine_counts
                         : battle_synergy.opponent_cuisine_counts;

      for (auto cuisine : magic_enum::enum_values<CuisineTagType>()) {
        if (tag.has(cuisine)) {
          counts[cuisine]++;
        }
      }
    }

    calculated = true;
  }
};
