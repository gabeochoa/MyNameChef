#pragma once

#include "../../log.h"
#include "../UITestHelpers.h"

struct ValidateMainMenuTest {
  static void execute() {
    // This test validates that main menu elements exist
    // It uses assertions - if any element doesn't exist, the test fails
    // immediately

    log_info("TEST DEBUG: Starting ValidateMainMenuTest::execute()");

    // Assert that all main menu elements exist
    UITestHelpers::assert_ui_exists("Play");
    log_info("TEST DEBUG: Play button assertion passed");

    UITestHelpers::assert_ui_exists("Settings");
    log_info("TEST DEBUG: Settings button assertion passed");

    UITestHelpers::assert_ui_exists("Dishes");
    log_info("TEST DEBUG: Dishes button assertion passed");

    UITestHelpers::assert_ui_exists("Quit");
    log_info("TEST DEBUG: Quit button assertion passed");

    log_info(
        "TEST DEBUG: ValidateMainMenuTest::execute() completed successfully");
  }
};
