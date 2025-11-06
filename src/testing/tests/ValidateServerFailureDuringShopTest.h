#pragma once

#include "../../game_state_manager.h"
#include "../test_macros.h"

TEST(validate_server_failure_during_shop) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();
  // navigate_to_shop already waits for Shop screen, so we don't need to verify
  // again
  app.wait_for_frames(10);

  // Kill the server
  app.kill_server();

  // Force NetworkSystem to check immediately (tests set fast interval via env
  // var)
  app.force_network_check();

  // Wait a moment for disconnection to be detected
  app.wait_for_frames(30);

  // Verify we're still on Shop screen (should not navigate)
  // With new behavior, we only navigate if there's a pending request,
  // and even then we stay on Shop screen
  app.expect_screen_is(GameStateManager::Screen::Shop);
}
