#pragma once

#include "../../dish_types.h"
#include "../../shop.h"
#include "../test_macros.h"

// Test that purchasing fails when inventory is full
TEST(validate_shop_purchase_full_inventory) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();

  // Get shop items
  const auto shop_items = app.read_store_options();
  if (shop_items.empty()) {
    app.fail("No items available in shop to test purchase failure");
  }

  // Wait a frame to ensure shop systems have initialized
  app.wait_for_frames(2);

  // Fill all inventory slots (7 slots)
  // GenerateInventorySlots may create some items, so we need to fill ALL slots
  // by creating items in each slot (will replace existing if needed)
  const int MAX_INVENTORY_SLOTS = 7;

  // Fill all slots to reach MAX_INVENTORY_SLOTS
  for (int i = 0; i < MAX_INVENTORY_SLOTS; ++i) {
    // Create a dish in each inventory slot (will replace existing if needed)
    app.create_inventory_item(DishType::Potato, i);
  }

  // Wait frames to ensure all items are merged and slots are marked occupied
  app.wait_for_frames(2);

  // Verify inventory has at least MAX_INVENTORY_SLOTS items (may have more if
  // duplicates)
  const auto inventory = app.read_player_inventory();
  const int inventory_count = static_cast<int>(inventory.size());
  if (inventory_count < MAX_INVENTORY_SLOTS) {
    app.fail("Test setup failed: inventory should have at least " +
             std::to_string(MAX_INVENTORY_SLOTS) + " items, but got " +
             std::to_string(inventory_count));
  }

  // If we have more than MAX_INVENTORY_SLOTS, that's actually fine for the test
  // The key is that all slots should be occupied, preventing new purchases

  // Find an affordable item
  const DishType item_type = shop_items[0].type;
  const int item_price = shop_items[0].price;
  int current_gold = app.read_wallet_gold();

  // Ensure we have enough gold
  if (current_gold < item_price) {
    app.set_wallet_gold(item_price);
    current_gold = item_price;
  }

  // Attempt purchase - should fail because inventory is full
  bool purchase_succeeded = app.try_purchase_item(item_type);
  if (purchase_succeeded) {
    // If purchase succeeded, verify it actually added an item (shouldn't
    // happen)
    const auto updated_inventory = app.read_player_inventory();
    if (static_cast<int>(updated_inventory.size()) > MAX_INVENTORY_SLOTS) {
      app.fail("Purchase succeeded and inventory grew beyond capacity (got " +
               std::to_string(updated_inventory.size()) + " items, max is " +
               std::to_string(MAX_INVENTORY_SLOTS) + ")");
    }
    // Even if purchase succeeded, it shouldn't have
    app.fail("Purchase should have failed because inventory is full");
  }

  // Verify gold wasn't charged
  app.expect_wallet_has(current_gold);

  // Verify inventory count didn't increase (purchase should have failed)
  const auto updated_inventory = app.read_player_inventory();
  const int updated_count = static_cast<int>(updated_inventory.size());
  if (updated_count > inventory_count) {
    app.fail("Inventory grew after failed purchase: got " +
             std::to_string(updated_count) + " items (was " +
             std::to_string(inventory_count) + ")");
  }
}

