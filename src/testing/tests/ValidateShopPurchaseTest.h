#pragma once

#include "../../components/is_dish.h"
#include "../../components/is_inventory_item.h"
#include "../../dish_types.h"
#include "../../query.h"
#include "../test_macros.h"
#include <afterhours/ah.h>
#include <source_location>

// Test purchasing items from the shop
TEST(validate_shop_purchase) {
  // Navigate to shop
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_ui_exists("Next Round");
  // Wait for systems to create inventory slots and shop items
  app.wait_for_frames(10);
  
  const TestOperationID purchase_stage =
      TestApp::generate_operation_id(std::source_location::current(),
                                     "validate_shop_purchase.purchase_stage");
  if (!app.completed_operations.count(purchase_stage)) {
    app.set_wallet_gold(10);
    app.wait_for_shop_items(1, 15.0f);
    std::vector<TestShopItemInfo> shop_items = app.read_store_options();
    app.expect_not_empty(shop_items, "shop items");

    const TestShopItemInfo item_to_buy = shop_items[0];
    app.set_test_shop_item("validate_shop_purchase.first_item", item_to_buy);
    app.set_test_int("validate_shop_purchase.initial_gold",
                     app.read_wallet_gold());

    app.expect_wallet_at_least(item_to_buy.price);
    app.purchase_item(item_to_buy.type);
    // purchase_item is now idempotent and tracks its own completion via completed_operations
    // It will return early if the item is already in inventory
    // Check if purchase completed by verifying item is in inventory
    bool item_in_inventory = false;
    for (afterhours::Entity &entity :
         afterhours::EntityQuery({.force_merge = true})
             .whereHasComponent<IsInventoryItem>()
             .whereHasComponent<IsDish>()
             .gen()) {
      const IsDish &dish = entity.get<IsDish>();
      if (dish.type == item_to_buy.type) {
        item_in_inventory = true;
        break;
      }
    }
    if (item_in_inventory) {
      // Purchase completed successfully - mark stage as complete
      app.completed_operations.insert(purchase_stage);
    }
    // If purchase didn't complete yet, don't mark as complete - test will resume and call purchase_item again
    return;
  }

  const TestOperationID verification_stage =
      TestApp::generate_operation_id(std::source_location::current(),
                                     "validate_shop_purchase.verify_stage");
  if (!app.completed_operations.count(verification_stage)) {
    const auto stored_item =
        app.get_test_shop_item("validate_shop_purchase.first_item");
    const auto initial_gold =
        app.get_test_int("validate_shop_purchase.initial_gold");

    if (!stored_item.has_value() || !initial_gold.has_value()) {
      app.fail("Missing stored data for shop purchase verification");
    }

    const int expected_gold = initial_gold.value() - stored_item->price;
    app.expect_wallet_has(expected_gold);
    app.expect_inventory_contains(stored_item->type);

    app.wait_for_shop_items(1, 15.0f);
    const auto updated_shop_items = app.read_store_options();
    int matching_items = 0;
    for (const auto &shop_item : updated_shop_items) {
      if (shop_item.id == stored_item->id) {
        matching_items++;
      }
    }
    app.expect_count_eq(matching_items, 0, "items with purchased ID in shop");
    app.completed_operations.insert(verification_stage);
    return;
  }
}
