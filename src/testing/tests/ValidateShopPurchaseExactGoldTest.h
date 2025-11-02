#pragma once

#include "../../dish_types.h"
#include "../test_macros.h"

// Test that purchasing succeeds with exact gold amount (boundary case)
TEST(validate_shop_purchase_exact_gold) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();

  // Get shop items
  const auto shop_items = app.read_store_options();
  if (shop_items.empty()) {
    app.fail("No items available in shop to purchase");
  }

  // Find the cheapest item
  const TestShopItemInfo *cheapest = &shop_items[0];
  for (const auto &item : shop_items) {
    if (item.price < cheapest->price) {
      cheapest = &item;
    }
  }

  // Set gold to exactly the price
  app.set_wallet_gold(cheapest->price);
  app.expect_wallet_has(cheapest->price);

  // Verify we can afford it
  if (!app.can_afford_purchase(cheapest->type)) {
    app.fail(
        "Test setup failed: should be able to afford item with exact gold");
  }

  // Purchase should succeed
  bool purchase_succeeded = app.try_purchase_item(cheapest->type);
  if (!purchase_succeeded) {
    app.fail("Purchase should have succeeded with exact gold amount");
  }

  // Verify gold was deducted to 0
  app.expect_wallet_has(0);

  // Verify item is in inventory
  const auto inventory = app.read_player_inventory();
  bool found = false;
  for (const auto &item : inventory) {
    if (item.type == cheapest->type) {
      found = true;
      break;
    }
  }
  if (!found) {
    app.fail("Purchased item should be in inventory");
  }
}

