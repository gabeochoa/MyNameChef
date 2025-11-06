#pragma once

#include "../components/battle_load_request.h"
#include "../components/network_info.h"
#include "../game_state_manager.h"
#include "../log.h"
#include <afterhours/ah.h>
#include <magic_enum/magic_enum.hpp>

struct ServerDisconnectionSystem : afterhours::System<NetworkInfo> {
  bool previousConnectionState = true;

  void for_each_with(afterhours::Entity &, NetworkInfo &networkInfo,
                     float) override {
    bool currentlyConnected = networkInfo.hasConnection;
    bool justReconnected = !previousConnectionState && currentlyConnected;
    bool justDisconnected = previousConnectionState && !currentlyConnected;

    previousConnectionState = currentlyConnected;

    GameStateManager &gsm = GameStateManager::get();
    GameStateManager::Screen current_screen = gsm.active_screen;

    bool hasPendingRequest = false;
    auto request_entity_opt = afterhours::EntityQuery()
                                  .whereHasComponent<BattleLoadRequest>()
                                  .gen_first();
    if (request_entity_opt) {
      const BattleLoadRequest &request =
          request_entity_opt.asE().get<BattleLoadRequest>();
      hasPendingRequest = request.serverRequestPending;
    }

    if (justDisconnected) {
      log_warn("SERVER_DISCONNECTION: Server disconnected (screen: {})",
               magic_enum::enum_name(current_screen));
      // TODO: Show toast message to user when server disconnects

      if (hasPendingRequest &&
          current_screen == GameStateManager::Screen::Shop) {
        log_warn("SERVER_DISCONNECTION: Disconnected during pending request on "
                 "Shop screen, staying on Shop");
        // Stay on Shop screen, don't navigate
      }
    }

    if (justReconnected) {
      log_info("SERVER_DISCONNECTION: Server reconnected (screen: {})",
               magic_enum::enum_name(current_screen));
      // TODO: Show toast message to user when server reconnects
    }
  }
};
