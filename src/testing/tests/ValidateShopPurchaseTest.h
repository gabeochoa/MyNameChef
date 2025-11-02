#pragma once

#include "../../dish_types.h"
#include "../test_macros.h"

// Test purchasing an item from the shop
TEST(validate_shop_purchase) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();

  const int initial_gold = app.read_wallet_gold();
  const auto initial_inventory = app.read_player_inventory();
  const auto shop_items = app.read_store_options();

  app.expect_not_empty(shop_items, "shop items");

  const TestShopItemInfo &item_to_buy = shop_items[0];
  const DishType item_type = item_to_buy.type;
  const int item_price = item_to_buy.price;

  app.expect_wallet_at_least(item_price);

  app.purchase_item(item_type);

  app.expect_wallet_has(initial_gold - item_price);

  app.expect_inventory_contains(item_type);

  // TODO: Verify purchased item is added to the end of inventory
  // Currently inventory may have pre-existing items, so we just verify the item
  // exists rather than checking position or size increase
  // Expected: Purchased items should be added to the next available slot
  // Bug: Item positioning logic may not place items at the end

  const auto updated_shop_items = app.read_store_options();
  int matching_items = 0;
  for (const auto &shop_item : updated_shop_items) {
    if (shop_item.id == item_to_buy.id) {
      matching_items++;
    }
  }
  app.expect_count_eq(matching_items, 0, "items with purchased ID in shop");
}
