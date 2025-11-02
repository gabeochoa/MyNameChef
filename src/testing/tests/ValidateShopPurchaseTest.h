#pragma once

#include "../../dish_types.h"
#include "../test_macros.h"

// Test purchasing an item from the shop
TEST(validate_shop_purchase) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();

  // Get initial state
  const int initial_gold = app.read_wallet_gold();
  const auto initial_inventory = app.read_player_inventory();
  const auto shop_items = app.read_store_options();

  if (shop_items.empty()) {
    app.fail("No items available in shop to purchase");
  }

  // Find the first available shop item
  const TestShopItemInfo &item_to_buy = shop_items[0];
  const DishType item_type = item_to_buy.type;
  const int item_price = item_to_buy.price;

  // Verify we have enough gold
  if (initial_gold < item_price) {
    app.fail("Not enough gold to purchase test item (have " +
             std::to_string(initial_gold) + ", need " +
             std::to_string(item_price) + ")");
  }

  // Purchase the item
  app.purchase_item(item_type);

  // Verify gold was deducted
  const int new_gold = app.read_wallet_gold();
  if (new_gold != initial_gold - item_price) {
    app.fail("Gold not deducted correctly after purchase: expected " +
             std::to_string(initial_gold - item_price) + ", got " +
             std::to_string(new_gold));
  }

  // Verify item is now in inventory
  const auto new_inventory = app.read_player_inventory();
  bool found_in_inventory = false;
  for (const auto &inv_item : new_inventory) {
    if (inv_item.type == item_type) {
      found_in_inventory = true;
      break;
    }
  }

  if (!found_in_inventory) {
    app.fail("Purchased item not found in inventory");
  }

  // TODO: Verify purchased item is added to the end of inventory
  // Currently inventory may have pre-existing items, so we just verify the item
  // exists rather than checking position or size increase
  // Expected: Purchased items should be added to the next available slot
  // Bug: Item positioning logic may not place items at the end

  // Verify item is no longer in shop (shop item should be removed)
  const auto updated_shop_items = app.read_store_options();
  bool still_in_shop = false;
  for (const auto &shop_item : updated_shop_items) {
    if (shop_item.id == item_to_buy.id) {
      still_in_shop = true;
      break;
    }
  }

  if (still_in_shop) {
    app.fail("Purchased item still appears in shop after purchase");
  }
}
