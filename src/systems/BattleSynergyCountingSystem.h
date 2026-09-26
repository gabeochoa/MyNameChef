#pragma once

#include "../components/battle_synergy_counts.h"
#include "../components/battle_team_data.h"
#include "../components/combat_queue.h"
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
    if (gsm.active_screen != GameStateManager::Screen::Battle || calculated) {
      return false;
    }
    // Async server flow: the Battle screen opens before the server reply
    // loads and instantiates the teams. Counting before instantiation
    // would latch an empty count for the whole battle.
    auto manager_entity =
        afterhours::EntityHelper::get_singleton<CombatQueue>();
    if (!manager_entity.get().has<CombatQueue>()) {
      return false;
    }
    bool player_ready = manager_entity.get().has<BattleTeamDataPlayer>() &&
                        manager_entity.get().get<BattleTeamDataPlayer>().instantiated;
    bool opponent_ready =
        manager_entity.get().has<BattleTeamDataOpponent>() &&
        manager_entity.get().get<BattleTeamDataOpponent>().instantiated;
    return player_ready && opponent_ready;
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
    battle_synergy.counts_ready = false;

    for (afterhours::Entity &entity :
         afterhours::EntityQuery({.force_merge = true})
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

    battle_synergy.counts_ready = true;
    calculated = true;
  }
};
