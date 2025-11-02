#pragma once

#include "../../dish_types.h"
#include "../test_macros.h"

// Test that purchasing fails when wallet has 0 gold
TEST(validate_shop_purchase_no_gold) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();

  const auto shop_items = app.read_store_options();
  app.expect_not_empty(shop_items, "shop items");

  app.set_wallet_gold(0);
  app.expect_wallet_has(0);

  const auto item_to_try = shop_items[0];
  const DishType item_type = item_to_try.type;

  app.expect_false(app.can_afford_purchase(item_type), "ability to afford item with 0 gold");

  bool purchase_succeeded = app.try_purchase_item(item_type);
  app.expect_false(purchase_succeeded, "purchase success with 0 gold");

  app.expect_wallet_has(0);

  const auto updated_shop_items = app.read_store_options();
  int matching_items = 0;
  for (const auto &shop_item : updated_shop_items) {
    if (shop_item.id == item_to_try.id) {
      matching_items++;
    }
  }
  app.expect_count_gt(matching_items, 0, "items with original ID in shop");
}


