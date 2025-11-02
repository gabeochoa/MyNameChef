#pragma once

#include "../../game_state_manager.h"
#include "../test_macros.h"

TEST(validate_replay_pause_play) {
  // Navigate to battle screen where replay UI should be visible
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  
  // Wait for replay UI to appear (Play/Pause button should be visible)
  app.wait_for_ui_exists("Pause", 5.0f);
  
  // Verify initial state: not paused
  bool initially_paused = app.read_replay_paused();
  app.expect_false(initially_paused, "replay should not be paused initially");
  
  // Click Pause button - the click should immediately toggle the state
  app.click("Pause");
  
  // Wait a frame for the UI to update and state to propagate
  app.wait_for_frames(1);
  
  // Verify paused state - the button label should now be "Play"
  bool after_pause = app.read_replay_paused();
  app.expect_true(after_pause, "replay should be paused after clicking Pause");
  
  // Wait for UI to update to show "Play" button
  app.wait_for_ui_exists("Play", 5.0f);
  
  // Click Play button (button label should have changed from "Pause" to "Play")
  app.click("Play");
  
  // Wait a frame for the UI to update and state to propagate
  app.wait_for_frames(1);
  
  // Verify unpaused state - the button label should now be "Pause" again
  bool after_play = app.read_replay_paused();
  app.expect_false(after_play, "replay should not be paused after clicking Play");
}

