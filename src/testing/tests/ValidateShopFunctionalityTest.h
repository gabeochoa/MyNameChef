#pragma once

#include "../../game_state_manager.h"
#include "../TestInteraction.h"
#include "../UITestHelpers.h"

struct ValidateShopFunctionalityTest {
  static void execute() {
    // Step 1: Navigate to shop screen first
    UITestHelpers::assert_ui_exists("Play");
    UITestHelpers::assert_click_ui("Play");
    
    // Apply the screen transition
    TestInteraction::start_game();
    GameStateManager::get().update_screen();

    // Note: UI validation will be done in validate_shop_complete() function
    // which runs on subsequent frames after the transition completes
  }

  static bool validate_shop_complete() {
    // Check UI elements that actually exist
    bool ui_elements_valid = UITestHelpers::check_ui_exists("Next Round") &&
                             UITestHelpers::check_ui_exists("Reroll (5)");

    // Check entity-based validations (visual elements)
    bool shop_slots_valid = UITestHelpers::shop_slots_exist();
    bool inventory_slots_valid = UITestHelpers::inventory_slots_exist();
    bool shop_items_valid = UITestHelpers::shop_items_exist();
    bool inventory_items_valid = UITestHelpers::inventory_items_exist();

    return ui_elements_valid && shop_slots_valid && inventory_slots_valid &&
           shop_items_valid && inventory_items_valid;
  }
};
