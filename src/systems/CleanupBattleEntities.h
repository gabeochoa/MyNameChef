#pragma once

#include "../components/battle_load_request.h"
#include "../components/battle_result.h"
#include "../components/battle_team_tags.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

struct CleanupBattleEntities : afterhours::System<> {
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.is_game_active();
  }

  void once(float) override {
    auto &gsm = GameStateManager::get();

    // Only clean up when leaving the results screen
    if (last_screen == GameStateManager::Screen::Results &&
        gsm.active_screen != GameStateManager::Screen::Results) {

      // Mark all battle team entities for cleanup
      for (auto &ref : afterhours::EntityQuery()
                           .template whereHasComponent<IsPlayerTeamItem>()
                           .gen()) {
        ref.get().cleanup = true;
      }

      for (auto &ref : afterhours::EntityQuery()
                           .template whereHasComponent<IsOpponentTeamItem>()
                           .gen()) {
        ref.get().cleanup = true;
      }

      // Clean up the BattleResult singleton
      auto battleResult = afterhours::EntityHelper::get_singleton<BattleResult>();
      if (battleResult.get().template has<BattleResult>()) {
        battleResult.get().cleanup = true;
      }

      // Don't clean up the BattleLoadRequest singleton - we need it for the next battle
      // It will be updated with new data when the next battle starts
    }

    last_screen = gsm.active_screen;
  }
};
