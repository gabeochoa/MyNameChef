#pragma once

#include "../../dish_types.h"
#include "../test_macros.h"

// Test that purchasing fails when insufficient funds
TEST(validate_shop_purchase_insufficient_funds) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();

  const auto shop_items = app.read_store_options();
  app.expect_not_empty(shop_items, "shop items");

  const TestShopItemInfo *most_expensive = &shop_items[0];
  for (const auto &item : shop_items) {
    if (item.price > most_expensive->price) {
      most_expensive = &item;
    }
  }

  const int gold_amount = most_expensive->price - 1;
  app.set_wallet_gold(gold_amount);
  app.expect_wallet_has(gold_amount);

  const auto item_to_try = *most_expensive;

  app.expect_false(app.can_afford_purchase(most_expensive->type), "ability to afford most expensive item");

  bool purchase_succeeded = app.try_purchase_item(most_expensive->type);
  app.expect_false(purchase_succeeded, "purchase success with insufficient funds");

  app.expect_wallet_has(gold_amount);

  const auto updated_shop_items = app.read_store_options();
  int matching_items = 0;
  for (const auto &shop_item : updated_shop_items) {
    if (shop_item.id == item_to_try.id) {
      matching_items++;
    }
  }
  app.expect_count_gt(matching_items, 0, "items with original ID in shop");
}


