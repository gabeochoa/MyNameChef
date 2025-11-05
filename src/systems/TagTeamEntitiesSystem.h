#pragma once

#include "../components/battle_team_tags.h"
#include "../components/dish_battle_state.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

struct TagTeamEntitiesSystem : afterhours::System<> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void once(float) override {
    for (afterhours::Entity &entity :
         afterhours::EntityQuery({.force_merge = true})
             .whereHasComponent<DishBattleState>()
             .gen()) {
      const DishBattleState &dbs = entity.get<DishBattleState>();

      bool should_have_player_tag = dbs.team_side == DishBattleState::TeamSide::Player;
      bool should_have_opponent_tag = dbs.team_side == DishBattleState::TeamSide::Opponent;

      bool has_player_tag = entity.has<IsPlayerTeamItem>();
      bool has_opponent_tag = entity.has<IsOpponentTeamItem>();

      if (should_have_player_tag && !has_player_tag) {
        entity.addComponent<IsPlayerTeamItem>();
      }
      if (should_have_opponent_tag && !has_opponent_tag) {
        entity.addComponent<IsOpponentTeamItem>();
      }

      if (should_have_player_tag && has_opponent_tag) {
        entity.removeComponent<IsOpponentTeamItem>();
      }
      if (should_have_opponent_tag && has_player_tag) {
        entity.removeComponent<IsPlayerTeamItem>();
      }
    }
  }
};

