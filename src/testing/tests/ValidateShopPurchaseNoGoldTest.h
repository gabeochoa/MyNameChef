#pragma once

#include "../../dish_types.h"
#include "../test_macros.h"

// Test that purchasing fails when wallet has 0 gold
TEST(validate_shop_purchase_no_gold) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();

  // Get shop items
  const auto shop_items = app.read_store_options();
  if (shop_items.empty()) {
    app.fail("No items available in shop to test purchase failure");
  }

  // Set gold to 0 to test purchase failure
  app.set_wallet_gold(0);
  app.expect_wallet_has(0);

  // Store the shop item ID and type before attempting purchase
  const auto item_to_try = shop_items[0];
  const DishType item_type = item_to_try.type;

  // Verify we can't afford it
  if (app.can_afford_purchase(item_type)) {
    app.fail(
        "Test setup failed: should not be able to afford item with 0 gold");
  }

  // Attempt purchase - should fail without throwing
  bool purchase_succeeded = app.try_purchase_item(item_type);
  if (purchase_succeeded) {
    app.fail("Purchase should have failed due to insufficient funds (0 gold)");
  }

  // Verify gold is still 0 (wasn't charged)
  app.expect_wallet_has(0);

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

