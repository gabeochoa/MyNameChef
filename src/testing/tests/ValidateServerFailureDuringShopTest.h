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

  // Wait for navigation back to main menu
  // With forced check and fast timeouts, this should happen quickly
  app.wait_for_screen(GameStateManager::Screen::Main, 5.0f);

  // Verify we're back on main menu
  app.expect_screen_is(GameStateManager::Screen::Main);
}
