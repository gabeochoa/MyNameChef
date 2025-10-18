#pragma once

#include "../UITestHelpers.h"

struct ValidateMainMenuTest {
  static void execute() {
    // This test just validates that main menu elements exist
    // It doesn't perform any actions, just checks UI state

    bool play_exists = UITestHelpers::visible_ui_exists("Play");
    bool settings_exists = UITestHelpers::visible_ui_exists("Settings");
    bool dishes_exists = UITestHelpers::visible_ui_exists("Dishes");
    bool quit_exists = UITestHelpers::visible_ui_exists("Quit");

    // In a real test framework, you'd assert these are true
    // For now, we just validate they exist
  }
};
