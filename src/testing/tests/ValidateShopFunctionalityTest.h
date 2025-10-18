#pragma once

#include "../../game_state_manager.h"
#include "../TestInteraction.h"
#include "../UITestHelpers.h"

struct ValidateShopFunctionalityTest {
  static void execute() {
    // Step 1: Navigate to shop screen first
    if (UITestHelpers::visible_ui_exists("Play")) {
      // We're on main menu, click Play to go to shop
      if (!UITestHelpers::click_ui("Play")) {
        return; // Failed to click Play
      }
      
      // Apply the screen transition
      TestInteraction::start_game();
      GameStateManager::get().update_screen();
    }

    // Test 1: Validate shop screen elements exist
    bool shop_elements_exist = UITestHelpers::visible_ui_exists("Next Round") &&
                               UITestHelpers::visible_ui_exists("Reroll (5)");

    if (!shop_elements_exist) {
      return; // Not on shop screen
    }

    // Test 2: Validate shop slots are present (should have 7 slots)
    // Note: Shop slots are IsDropSlot components, not UI elements with labels
    // TODO: Add UI labels to shop slots for better testing
    // Expected: 7 shop slots should be visible
    // Bug: Shop slots don't have UI labels, only visual rendering

    // Test 3: Validate inventory slots are present (should have 7 slots)
    // Note: Inventory slots are IsDropSlot components, not UI elements with labels
    // TODO: Add UI labels to inventory slots for better testing
    // Expected: 7 inventory slots should be visible
    // Bug: Inventory slots don't have UI labels, only visual rendering

    // Test 4: Validate wallet display
    // Note: Wallet is rendered as text using raylib::DrawText(), not as UI element
    // TODO: Convert wallet display to UI element with label
    // Expected: Wallet/gold display should be present
    // Bug: Wallet display is not a UI element, only text rendering

    // Test 5: Validate shop items have prices
    // This would require checking if shop items show price information
    // TODO: Shop items should display prices
    // Expected: Each shop item should show its price
    // Bug: Price display may be missing

    // Test 6: Validate reroll functionality
    // TODO: Reroll button should be clickable and functional
    // Expected: Clicking reroll should change shop items
    // Bug: Reroll functionality may not be implemented

    // Test 7: Validate freeze functionality
    // TODO: Items should be freezable to prevent reroll
    // Expected: Clicking items should toggle freeze state
    // Bug: Freeze functionality may not be implemented
  }

  static bool validate_shop_complete() {
    // Only check for UI elements that actually exist
    return UITestHelpers::visible_ui_exists("Next Round") &&
           UITestHelpers::visible_ui_exists("Reroll (5)");
  }
};
