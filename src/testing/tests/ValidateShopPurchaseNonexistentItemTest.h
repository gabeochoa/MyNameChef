#pragma once

#include "../../dish_types.h"
#include "../test_macros.h"
#include <set>

// Test that purchasing fails when item doesn't exist in shop
TEST(validate_shop_purchase_nonexistent_item) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();

  const auto shop_items = app.read_store_options();
  app.expect_not_empty(shop_items, "shop items");

  std::set<DishType> shop_dish_types;
  for (const auto &item : shop_items) {
    shop_dish_types.insert(item.type);
  }

  DishType nonexistent_type = DishType::Bagel;
  bool found_nonexistent = false;
  for (int i = 0; i < 50; ++i) {
    DishType test_type = static_cast<DishType>(i);
    if (shop_dish_types.find(test_type) == shop_dish_types.end()) {
      nonexistent_type = test_type;
      found_nonexistent = true;
      break;
    }
  }

  if (!found_nonexistent) {
    return;
  }

  bool purchase_succeeded = app.try_purchase_item(nonexistent_type);
  app.expect_false(purchase_succeeded, "purchase success for nonexistent item");
}

