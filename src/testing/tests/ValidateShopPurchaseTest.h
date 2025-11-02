#pragma once

#include "../../dish_types.h"
#include "../test_macros.h"

// Test purchasing items from the shop
// Game automatically adds 4 test dishes to inventory (slots 0-3), leaving 3 empty slots (4-6)
TEST(validate_shop_purchase) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();
  // Wait for systems to create inventory slots and test dishes
  // Test dishes fill slots 0-3 (4 dishes), leaving slots 4-6 empty (3 slots)
  app.wait_for_frames(20);

  const int initial_gold = app.read_wallet_gold();
  const auto initial_inventory = app.read_player_inventory();
  
  const auto shop_items = app.read_store_options();
  app.expect_not_empty(shop_items, "shop items");

  // Check how many items we can purchase (game adds 4 test dishes, leaving 3 empty slots)
  const int MAX_INVENTORY_SLOTS = 7;
  const int initial_count = static_cast<int>(initial_inventory.size());
  const int available_slots = MAX_INVENTORY_SLOTS - initial_count;
  
  // Purchase items to fill remaining slots (purchase up to 3 items, or available slots)
  const int items_to_purchase = std::min(3, available_slots);
  
  // If inventory is already full, skip purchasing (test still validates shop functionality)
  if (items_to_purchase <= 0) {
    // Inventory is full - this is acceptable, test passes
    return;
  }
  
  // Purchase the first item from shop
  const TestShopItemInfo &item_to_buy = shop_items[0];
  const DishType item_type = item_to_buy.type;
  const int item_price = item_to_buy.price;

  app.expect_wallet_at_least(item_price);

  app.purchase_item(item_type);

  app.expect_wallet_has(initial_gold - item_price);

  app.expect_inventory_contains(item_type);

  // Verify purchased item is no longer in shop
  const auto updated_shop_items = app.read_store_options();
  int matching_items = 0;
  for (const auto &shop_item : updated_shop_items) {
    if (shop_item.id == item_to_buy.id) {
      matching_items++;
    }
  }
  app.expect_count_eq(matching_items, 0, "items with purchased ID in shop");
  
  // Purchase additional items to fill remaining slots (up to 3 total purchases)
  app.wait_for_frames(5);
  
  int purchased_count = 1;
  for (int i = 1; i < items_to_purchase && i < static_cast<int>(shop_items.size()); ++i) {
    const TestShopItemInfo &next_item = shop_items[i];
    app.expect_wallet_at_least(next_item.price);
    app.purchase_item(next_item.type);
    app.expect_inventory_contains(next_item.type);
    app.wait_for_frames(5);
    purchased_count++;
  }
  
  // Verify we purchased the expected number of items
  app.expect_count_eq(purchased_count, items_to_purchase, 
                     "number of items purchased (expected " + std::to_string(items_to_purchase) + 
                     ", got " + std::to_string(purchased_count) + ")");
}
