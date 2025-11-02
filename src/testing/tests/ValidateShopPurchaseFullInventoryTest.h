#pragma once

#include "../../dish_types.h"
#include "../../shop.h"
#include "../test_macros.h"

// Test that purchasing fails when inventory is full
TEST(validate_shop_purchase_full_inventory) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();

  const auto shop_items = app.read_store_options();
  app.expect_not_empty(shop_items, "shop items");

  app.wait_for_frames(2);

  const int MAX_INVENTORY_SLOTS = 7;

  for (int i = 0; i < MAX_INVENTORY_SLOTS; ++i) {
    app.create_inventory_item(DishType::Potato, i);
  }

  app.wait_for_frames(2);

  const auto inventory = app.read_player_inventory();
  const int inventory_count = static_cast<int>(inventory.size());
  app.expect_count_gte(inventory_count, MAX_INVENTORY_SLOTS, "inventory items after filling slots");

  const DishType item_type = shop_items[0].type;
  const int item_price = shop_items[0].price;
  int current_gold = app.read_wallet_gold();

  if (current_gold < item_price) {
    app.set_wallet_gold(item_price);
    current_gold = item_price;
  }

  bool purchase_succeeded = app.try_purchase_item(item_type);
  app.expect_false(purchase_succeeded, "purchase success with full inventory");

  app.expect_wallet_has(current_gold);

  const auto updated_inventory = app.read_player_inventory();
  const int updated_count = static_cast<int>(updated_inventory.size());
  app.expect_count_lte(updated_count, inventory_count, "inventory count after failed purchase");
}

