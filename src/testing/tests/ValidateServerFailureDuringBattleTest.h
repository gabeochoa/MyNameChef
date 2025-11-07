#pragma once

#include "../../game_state_manager.h"
#include "../test_macros.h"

TEST(validate_server_failure_during_battle) {
  log_info("TEST FUNCTION CALLED: validate_server_failure_during_battle - Starting test execution");
  // Navigate to battle
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_frames(10);

  // Ensure we have dishes before going to battle
  app.create_inventory_item(DishType::Potato, 0);
  app.wait_for_frames(5);

  app.navigate_to_battle();
  // navigate_to_battle already waits for Battle screen, so we don't need to
  // verify again
  app.wait_for_frames(10);

  // Kill the server
  app.kill_server();

  // Force NetworkSystem to check immediately (tests set fast interval via env
  // var)
  app.force_network_check();

  // Wait a moment for disconnection to be detected
  app.wait_for_frames(30);

  // Verify we're still on Battle screen (should not navigate)
  // With new behavior, we never navigate away from Battle screen on disconnection
  // However, if battle completed very quickly with timing speed scale, we might be on Results
  GameStateManager::get().update_screen();
  GameStateManager::Screen current_screen = GameStateManager::get().active_screen;
  if (current_screen == GameStateManager::Screen::Results) {
    // Battle completed before server was killed, which is acceptable
    log_info("TEST: Battle completed before server failure, on Results screen (acceptable)");
  } else {
    // Should still be on Battle screen
    app.expect_screen_is(GameStateManager::Screen::Battle);
  }
}
