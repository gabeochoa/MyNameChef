#pragma once

#include "../../game_state_manager.h"
#include "../test_macros.h"

TEST(validate_full_game_flow) {
  // Check if we're already on Battle screen (from a previous test iteration)
  GameStateManager::get().update_screen();
  auto &gsm = GameStateManager::get();
  if (gsm.active_screen == GameStateManager::Screen::Battle) {
    app.wait_for_ui_exists("Skip to Results", 5.0f);
    return; // Test is done!
  }

  // Step 1: Start from main menu
  app.launch_game();
  app.wait_for_ui_exists("Play");

  // Step 2: Navigate to shop
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);

  // TODO: Validate shop screen loaded
  // Expected: Shop screen with slots and inventory
  // Bug: Shop screen may not be loading properly

  // Step 3: Validate shop functionality
  // TODO: Shop should have items to buy
  // Expected: Shop slots populated with dishes
  // Bug: Shop generation may not be working
  app.wait_for_ui_exists("Next Round");
  app.wait_for_ui_exists("Reroll (5)");

  // Step 4: Navigate to battle
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);

  // TODO: Validate battle screen loaded
  // Expected: Battle screen with combat display
  // Bug: Battle screen may not be loading properly
  app.wait_for_ui_exists("Skip to Results");

  // Step 5: Validate battle progression
  // TODO: Battle should progress through courses
  // Expected: 7 courses with alternating bites
  // Bug: Battle progression may not be working

  // Step 6: Validate battle completion
  // TODO: Battle should complete and show results
  // Expected: Results screen with match outcome
  // Bug: Battle completion may not be working

  // TODO: Validate game state is consistent throughout flow
  // Expected: No crashes, proper state transitions
  // Bug: Game state may become inconsistent

  // TODO: Validate entity cleanup
  // Expected: Entities should be properly cleaned up between screens
  // Bug: Entity cleanup may not be working

  // TODO: Validate singleton state
  // Expected: Singletons should maintain proper state
  // Bug: Singleton state may be corrupted

  // TODO: Validate game performance
  // Expected: Smooth 60fps, no stuttering
  // Bug: Performance may be poor

  // TODO: Validate memory usage
  // Expected: No memory leaks
  // Bug: Memory leaks may exist

  // TODO: Validate rendering performance
  // Expected: Efficient rendering pipeline
  // Bug: Rendering may be inefficient
}
