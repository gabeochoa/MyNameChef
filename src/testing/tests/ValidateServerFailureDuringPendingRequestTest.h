#pragma once

#include "../../components/battle_load_request.h"
#include "../../game_state_manager.h"
#include "../test_app.h"
#include "../test_macros.h"
#include "../test_server_helpers.h"
#include <afterhours/ah.h>

TEST(validate_server_failure_during_pending_request) {
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

  // Setup BattleLoadRequest with server URL
  test_server_helpers::server_integration_test_setup("PENDING_REQUEST_TEST");

  // Ensure we have dishes before going to battle
  app.create_inventory_item(DishType::Potato, 0);
  app.wait_for_frames(5);

  // Kill the server BEFORE clicking "Next Round"
  // This ensures the request will fail when it tries to connect
  app.kill_server();

  // Click "Next Round" to trigger server request
  // This will navigate to Battle screen and start the request
  // The request will fail because server is dead
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);

  // Wait for the request to start and fail
  // ServerBattleRequestSystem sets serverRequestPending = true when starting,
  // then sets it back to false when the request fails
  int max_wait_frames = 180; // 3 seconds at 60fps
  int frames_waited = 0;
  bool request_attempted = false;
  while (frames_waited < max_wait_frames) {
    app.wait_for_frames(1);
    frames_waited++;

    auto request_opt =
        afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    if (request_opt.get().has<BattleLoadRequest>()) {
      const BattleLoadRequest &req = request_opt.get().get<BattleLoadRequest>();
      // Request was attempted (either pending or failed)
      if (req.serverRequestPending || !req.playerJsonPath.empty() ||
          !req.opponentJsonPath.empty()) {
        request_attempted = true;
        break;
      }
    }
  }

  app.expect_true(request_attempted,
                  "Server request should have been attempted");

  // Force NetworkSystem to check immediately to detect disconnection
  app.force_network_check();

  // Wait a moment for disconnection to be detected
  app.wait_for_frames(30);

  // Verify we're still on Battle screen (should not navigate)
  // With new behavior, we show toast but don't navigate on Battle screen
  // even when there was a pending request
  app.expect_screen_is(GameStateManager::Screen::Battle);

  log_info("PENDING_REQUEST_TEST: ✅ Test passed - stayed on Battle screen "
           "after server failure during pending request");
}

