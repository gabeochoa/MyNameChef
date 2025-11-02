#pragma once

#include "../../dish_types.h"
#include "../test_macros.h"

// Test that purchasing fails when insufficient funds
TEST(validate_shop_purchase_insufficient_funds) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();

  // Get shop items
  const auto shop_items = app.read_store_options();
  if (shop_items.empty()) {
    app.fail("No items available in shop to test purchase failure");
  }

  // Find the most expensive item
  const TestShopItemInfo *most_expensive = &shop_items[0];
  for (const auto &item : shop_items) {
    if (item.price > most_expensive->price) {
      most_expensive = &item;
    }
  }

  // Set gold to just below price to test purchase failure
  const int gold_amount = most_expensive->price - 1;
  app.set_wallet_gold(gold_amount);
  app.expect_wallet_has(gold_amount);

  // Store item ID before attempting purchase
  const auto item_to_try = *most_expensive;

  // Verify we can't afford it
  if (app.can_afford_purchase(most_expensive->type)) {
    app.fail(
        "Test setup failed: should not be able to afford most expensive item");
  }

  // Attempt purchase - should fail without throwing
  bool purchase_succeeded = app.try_purchase_item(most_expensive->type);
  if (purchase_succeeded) {
    app.fail("Purchase should have failed due to insufficient funds");
  }

  // Verify gold hasn't changed (wasn't charged)
  app.expect_wallet_has(gold_amount);

  // Verify item is still in shop (purchase failed)
  const auto updated_shop_items = app.read_store_options();
  bool still_in_shop = false;
  for (const auto &shop_item : updated_shop_items) {
    if (shop_item.id == item_to_try.id) {
      still_in_shop = true;
      break;
    }
  }
  if (!still_in_shop) {
    app.fail("Item should still be in shop after failed purchase");
  }
}

