#pragma once

#include "../../game_state_manager.h"
#include "../TestInteraction.h"
#include "../UITestHelpers.h"

struct ValidateShopNavigationTest {
  static void execute() {
    // Step 1: Assert we start on Main screen
    UITestHelpers::assert_ui_exists("Play");

    // Step 2: Click the Play button
    UITestHelpers::assert_click_ui("Play");

    // Step 3: Apply the screen transition
    TestInteraction::start_game();
    GameStateManager::get().update_screen();

    // Step 4: Validate we're now on Shop screen (this will be checked in next
    // frame)
  }

  // Validation function that can be called after the transition
  static bool validate_shop_screen() {
    return UITestHelpers::check_ui_exists("Next Round") &&
           UITestHelpers::check_ui_exists("Reroll (5)");
  }
};
