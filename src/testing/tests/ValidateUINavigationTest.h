#pragma once

#include "../../game_state_manager.h"
#include "../test_macros.h"

TEST(validate_ui_navigation) {
  GameStateManager::get().update_screen();
  auto &gsm = GameStateManager::get();
  if (gsm.active_screen == GameStateManager::Screen::Battle) {
    app.wait_for_ui_exists("Skip to Results", 5.0f);
    return;
  }

  // Test 1: Validate main menu navigation
  // TODO: Validate main menu elements
  // Expected: Play, Settings, Dishes, Quit buttons
  // Bug: Some main menu buttons may not be visible
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.wait_for_ui_exists("Settings");
  app.wait_for_ui_exists("Dishes");
  app.wait_for_ui_exists("Quit");

  // Test 2: Validate shop navigation
  // TODO: Validate shop screen elements
  // Expected: Shop slots, inventory slots, reroll button
  // Bug: Shop UI elements may not be rendering
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");
  app.wait_for_ui_exists("Reroll (5)");

  // Test 3: Navigate to battle and validate battle UI
  // TODO: Validate battle screen elements
  // Expected: Combat display, dish stats, progress indicators
  // Bug: Battle UI may not be complete
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_ui_exists("Skip to Results", 5.0f);

  // Test 4: Validate results navigation
  // TODO: Validate results screen elements
  // Expected: Match results, course outcomes, replay button
  // Bug: Results screen may not be implemented

  // Test 5: Validate input handling
  // Test 1: Validate button clicks
  // TODO: UI buttons should respond to clicks
  // Expected: Click handlers should be registered
  // Bug: Button click handling may not be working

  // Test 2: Validate drag and drop
  // TODO: Drag and drop should work for inventory/shop
  // Expected: Items can be moved between slots
  // Bug: Drag and drop may not be implemented

  // Test 3: Validate keyboard input
  // TODO: Keyboard shortcuts should work
  // Expected: ESC for back, Enter for confirm
  // Bug: Keyboard input may not be handled
}
