#pragma once

#include "../../components/is_drop_slot.h"
#include "../../components/is_inventory_item.h"
#include "../../components/is_shop_item.h"
#include "../../query.h"
#include "../test_macros.h"

TEST(validate_shop_functionality) {
  // Navigate to shop screen
  app.navigate_to_shop();

  // Verify UI elements exist
  app.wait_for_ui_exists("Next Round");
  app.wait_for_ui_exists("Reroll (5)");

  // Verify shop slots exist
  int shop_slot_count = 0;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery().whereHasComponent<IsDropSlot>().gen()) {
    const IsDropSlot &slot = entity.get<IsDropSlot>();
    // Shop slots accept shop items but not inventory items initially
    if (slot.accepts_shop_items && !slot.accepts_inventory_items) {
      shop_slot_count++;
    }
  }
  if (shop_slot_count == 0) {
    app.fail("No shop slots found");
  }

  // Verify inventory slots exist
  int inventory_slot_count = 0;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery().whereHasComponent<IsDropSlot>().gen()) {
    const IsDropSlot &slot = entity.get<IsDropSlot>();
    // Inventory slots accept inventory items
    if (slot.accepts_inventory_items) {
      inventory_slot_count++;
    }
  }
  if (inventory_slot_count == 0) {
    app.fail("No inventory slots found");
  }

  // Verify shop items exist
  int shop_item_count =
      afterhours::EntityQuery().whereHasComponent<IsShopItem>().gen_count();
  if (shop_item_count == 0) {
    app.fail("No shop items found");
  }

  // Verify inventory items can exist (may be 0 initially)
  // We just check that the query works, not that items exist
  afterhours::EntityQuery().whereHasComponent<IsInventoryItem>().gen_count();
}
