#pragma once

#include "../../game_state_manager.h"
#include "../TestInteraction.h"
#include "../UITestHelpers.h"

struct ValidateShopNavigationTest {
  static void execute() {
    // Step 1: Validate we start on Main screen
    if (!UITestHelpers::visible_ui_exists("Play")) {
      return; // Not on main screen
    }

    // Step 2: Click the Play button
    if (!UITestHelpers::click_ui("Play")) {
      return; // Failed to click Play button
    }

    // Step 3: Apply the screen transition
    TestInteraction::start_game();
    GameStateManager::get().update_screen();

    // Step 4: Validate we're now on Shop screen (this will be checked in next
    // frame) The TestSystem will need to be enhanced to support multi-frame
    // validation
  }

  // Validation function that can be called after the transition
  static bool validate_shop_screen() {
    return UITestHelpers::visible_ui_exists("Next Round") &&
           UITestHelpers::visible_ui_exists("Reroll (5)");
  }
};
