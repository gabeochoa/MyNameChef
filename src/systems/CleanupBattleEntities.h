#pragma once

#include "../components/battle_load_request.h"
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

    // Check if we just left the battle screen
    if (last_screen == GameStateManager::Screen::Battle &&
        gsm.active_screen != GameStateManager::Screen::Battle) {

      log_info("Cleaning up battle entities after leaving battle screen");

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

      // Clean up the BattleLoadRequest singleton
      auto battleRequest =
          afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
      if (battleRequest.get().template has<BattleLoadRequest>()) {
        battleRequest.get().cleanup = true;
      }
    }

    last_screen = gsm.active_screen;
  }
};
