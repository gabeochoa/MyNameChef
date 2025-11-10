#pragma once

#include "../../dish_types.h"
#include "../test_macros.h"

// Test purchasing items from the shop
TEST(validate_shop_purchase) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_ui_exists("Next Round");
  // Wait for systems to create inventory slots and shop items
  app.wait_for_frames(10);
  
  const auto shop_items = app.read_store_options();
  app.expect_not_empty(shop_items, "shop items");
  
  const int initial_gold = app.read_wallet_gold();
  const auto initial_inventory = app.read_player_inventory();

  // Purchase at least one item (required to proceed past shop screen)
  const TestShopItemInfo &item_to_buy = shop_items[0];
  const DishType item_type = item_to_buy.type;
  const int item_price = item_to_buy.price;

  app.expect_wallet_at_least(item_price);
  app.purchase_item(item_type);
  app.expect_wallet_has(initial_gold - item_price);
  app.expect_inventory_contains(item_type);
  
  // Verify purchased item is no longer in shop
  app.wait_for_frames(5);
  const auto updated_shop_items = app.read_store_options();
  int matching_items = 0;
  for (const auto &shop_item : updated_shop_items) {
    if (shop_item.id == item_to_buy.id) {
      matching_items++;
    }
  }
  app.expect_count_eq(matching_items, 0, "items with purchased ID in shop");
  
  // Try to purchase additional items if we have slots and gold available
  const int MAX_INVENTORY_SLOTS = 7;
  const auto final_inventory = app.read_player_inventory();
  const int final_count = static_cast<int>(final_inventory.size());
  const int remaining_slots = MAX_INVENTORY_SLOTS - final_count;
  
  app.expect_count_gt(remaining_slots, 0, "should have at least one slot available after first purchase");
  
  // Purchase one more item if possible
  if (remaining_slots > 0 && !updated_shop_items.empty()) {
    const TestShopItemInfo &second_item = updated_shop_items[0];
    const int second_price = second_item.price;
    const int current_gold = app.read_wallet_gold();
    
    app.expect_true(current_gold >= second_price || remaining_slots > 1, 
                    "should have gold for second purchase or slots available");
    
    if (current_gold >= second_price) {
      app.purchase_item(second_item.type);
      app.expect_inventory_contains(second_item.type);
    }
  }
}
