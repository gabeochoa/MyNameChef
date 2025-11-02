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

  GameStateManager::Screen current = app.read_current_screen();

  if (current == GameStateManager::Screen::Shop) {
    app.navigate_to_battle();
    app.wait_for_frames(1);
  }

  const DishType test_type = DishType::Potato;

  bool purchase_succeeded = app.try_purchase_item(test_type);
  app.expect_false(purchase_succeeded, "purchase success when not on shop screen");

  app.navigate_to_shop();
  app.expect_screen_is(GameStateManager::Screen::Shop);

  const auto shop_items = app.read_store_options();
  if (!shop_items.empty()) {
    const int price = shop_items[0].price;
    app.set_wallet_gold(price + 10);

    bool purchase_works = app.try_purchase_item(shop_items[0].type);
    app.expect_true(purchase_works, "purchase success when on shop screen");
  }
}

