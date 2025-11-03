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
  // Reroll button starts at cost 1 (base=1, increment=0)
  app.wait_for_ui_exists("Reroll (1)");

  int shop_slot_count = 0;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery().whereHasComponent<IsDropSlot>().gen()) {
    const IsDropSlot &slot = entity.get<IsDropSlot>();
    if (slot.accepts_shop_items && !slot.accepts_inventory_items) {
      shop_slot_count++;
    }
  }
  app.expect_count_gt(shop_slot_count, 0, "shop slots");

  int inventory_slot_count = 0;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery().whereHasComponent<IsDropSlot>().gen()) {
    const IsDropSlot &slot = entity.get<IsDropSlot>();
    if (slot.accepts_inventory_items) {
      inventory_slot_count++;
    }
  }
  app.expect_count_gt(inventory_slot_count, 0, "inventory slots");

  int shop_item_count = static_cast<int>(
      afterhours::EntityQuery().whereHasComponent<IsShopItem>().gen_count());
  app.expect_count_gt(shop_item_count, 0, "shop items");

  // Verify inventory items can exist (may be 0 initially)
  // We just check that the query works, not that items exist
  (void)afterhours::EntityQuery().whereHasComponent<IsInventoryItem>().gen_count();
}
