#pragma once

#include "../components/network_info.h"
#include "../game_state_manager.h"
#include "../log.h"
#include <afterhours/ah.h>
#include <magic_enum/magic_enum.hpp>

struct ServerDisconnectionSystem : afterhours::System<NetworkInfo> {
  void for_each_with(afterhours::Entity &, NetworkInfo &networkInfo,
                     float) override {
    // Only act if we're disconnected
    if (networkInfo.hasConnection) {
      return;
    }

    // TODO eventually we only want to kick out once we finish what we are doing
    // since the battle is just playing what the server told us, its not a bad
    // idea to just let it finishe playing and then after ther estul screen go
    // back to main menu. when on shop, its okay until you hit"next round" and
    // then it should error and go back to main menu.

    GameStateManager &gsm = GameStateManager::get();
    GameStateManager::Screen current_screen = gsm.active_screen;

    // Only navigate to main menu if we're on Shop or Battle screen
    if (current_screen == GameStateManager::Screen::Shop ||
        current_screen == GameStateManager::Screen::Battle) {
      log_warn("SERVER_DISCONNECTION: Server disconnected during gameplay, "
               "returning to main menu (screen: {})",
               magic_enum::enum_name(current_screen));
      // TODO: Show error message to user when server disconnects
      gsm.set_next_screen(GameStateManager::Screen::Main);
    }
  }
};
