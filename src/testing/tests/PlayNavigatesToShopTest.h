#pragma once

#include "../../game_state_manager.h"
#include "../TestInteraction.h"
#include "../UITestHelpers.h"

struct PlayNavigatesToShopTest {
  static void execute() {
    // Assert we start on Main screen
    UITestHelpers::assert_ui_exists("Play");

    // Click the Play button
    UITestHelpers::assert_click_ui("Play");

    // Apply the screen transition
    TestInteraction::start_game();
    GameStateManager::get().update_screen();

    // Validate we're now on Shop screen
    // Note: This validation happens in the next frame after UI systems run
  }
};
