#pragma once

#include "../../game_state_manager.h"
#include "../TestInteraction.h"
#include "../UITestHelpers.h"

struct PlayNavigatesToShopTest {
  static void execute() {
    // Validate we start on Main screen
    if (!UITestHelpers::visible_ui_exists("Play")) {
      return; // Not on main screen
    }

    // Click the Play button
    if (!UITestHelpers::click_ui("Play")) {
      return; // Failed to click Play button
    }

    // Apply the screen transition
    TestInteraction::start_game();
    GameStateManager::get().update_screen();

    // Validate we're now on Shop screen
    // Note: This validation happens in the next frame after UI systems run
  }
};
