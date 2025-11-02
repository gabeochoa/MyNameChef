#pragma once

#include "../../dish_types.h"
#include "../test_macros.h"

// Test that purchasing succeeds with exact gold amount (boundary case)
TEST(validate_shop_purchase_exact_gold) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();

  const auto shop_items = app.read_store_options();
  app.expect_not_empty(shop_items, "shop items");

  const TestShopItemInfo *cheapest = &shop_items[0];
  for (const auto &item : shop_items) {
    if (item.price < cheapest->price) {
      cheapest = &item;
    }
  }

  app.set_wallet_gold(cheapest->price);
  app.expect_wallet_has(cheapest->price);

  app.expect_true(app.can_afford_purchase(cheapest->type),
                  "ability to afford item with exact gold");

  bool purchase_succeeded = app.try_purchase_item(cheapest->type);
  app.expect_true(purchase_succeeded,
                  "purchase success with exact gold amount");

  app.expect_wallet_has(0);

  app.expect_inventory_contains(cheapest->type);
}
