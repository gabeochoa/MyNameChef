#pragma once

#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../test_macros.h"

// Test that purchasing fails when not on shop screen
TEST(validate_shop_purchase_wrong_screen) {
  // Start game - launch_game resets to main menu
  app.launch_game();

  // Wait frames for initialization and ensure we're on main screen
  app.wait_for_frames(2);

  // Check current screen - if we're already on shop, navigate to battle to test
  GameStateManager::Screen current = app.read_current_screen();

  if (current == GameStateManager::Screen::Shop) {
    // Navigate to battle screen to test purchase failure
    app.navigate_to_battle();
    app.wait_for_frames(1);
  }

  // Get a dish type to try (any will do)
  const DishType test_type = DishType::Potato;

  // Attempt purchase - should fail because we're not on shop screen
  bool purchase_succeeded = app.try_purchase_item(test_type);
  if (purchase_succeeded) {
    app.fail("Purchase should have failed when not on shop screen");
  }

  // Navigate to shop and verify purchase works there
  app.navigate_to_shop();
  app.expect_screen_is(GameStateManager::Screen::Shop);

  // Set up gold
  const auto shop_items = app.read_store_options();
  if (!shop_items.empty()) {
    const int price = shop_items[0].price;
    app.set_wallet_gold(price + 10); // Extra gold to be safe

    // Now purchase should work
    bool purchase_works = app.try_purchase_item(shop_items[0].type);
    if (!purchase_works) {
      app.fail("Purchase should work when on shop screen");
    }
  }
}

