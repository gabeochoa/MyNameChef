#pragma once

#include "../../dish_types.h"
#include "../test_macros.h"
#include <set>

// Test that purchasing fails when item doesn't exist in shop
TEST(validate_shop_purchase_nonexistent_item) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();

  // Get all shop items
  const auto shop_items = app.read_store_options();
  if (shop_items.empty()) {
    app.fail("No items in shop to test with");
  }

  // Find a dish type that's NOT in the shop
  std::set<DishType> shop_dish_types;
  for (const auto &item : shop_items) {
    shop_dish_types.insert(item.type);
  }

  // Try some dish types until we find one not in shop
  DishType nonexistent_type = DishType::Bagel;
  bool found_nonexistent = false;
  for (int i = 0; i < 50; ++i) { // Try up to 50 dish types
    DishType test_type = static_cast<DishType>(i);
    if (shop_dish_types.find(test_type) == shop_dish_types.end()) {
      nonexistent_type = test_type;
      found_nonexistent = true;
      break;
    }
  }

  if (!found_nonexistent) {
    // Skip test if we can't find a nonexistent type (shop might be very full)
    return;
  }

  // Attempt purchase - should fail because item doesn't exist
  bool purchase_succeeded = app.try_purchase_item(nonexistent_type);
  if (purchase_succeeded) {
    app.fail("Purchase should have failed for nonexistent item");
  }
}

