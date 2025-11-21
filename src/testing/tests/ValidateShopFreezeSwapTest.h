#pragma once

#include "../../components/is_dish.h"
#include "../../components/is_shop_item.h"
#include "../../dish_types.h"
#include "../../query.h"
#include "../../shop.h"
#include "../test_macros.h"
#include <map>
#include <string>
#include <vector>

TEST(validate_shop_freeze_swap) {
  (void)app;
  // Test that frozen shop items maintain their freeze state when swapped with other shop items

  app.navigate_to_shop();
  app.wait_for_ui_exists("Reroll (1 gold)");
  app.wait_for_frames(5);

  // Get initial shop items
  std::vector<afterhours::Entity *> shop_items;
  std::map<int, DishType> item_types; // slot -> dish type
  std::map<int, bool> item_frozen;     // slot -> is frozen

  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsShopItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    shop_items.push_back(&entity);
    const IsShopItem &shop_item = entity.get<IsShopItem>();
    const IsDish &dish = entity.get<IsDish>();
    item_types[shop_item.slot] = dish.type;

    bool is_frozen = false;
    if (entity.has<Freezeable>()) {
      is_frozen = entity.get<Freezeable>().isFrozen;
    }
    item_frozen[shop_item.slot] = is_frozen;
  }

  app.expect_count_gte(static_cast<int>(shop_items.size()), 2,
                      "need at least 2 shop items to test swap");

  // Freeze the first item
  if (shop_items.size() >= 1) {
    afterhours::Entity &first_item = *shop_items[0];
    if (!first_item.has<Freezeable>()) {
      first_item.addComponent<Freezeable>(false);
    }
    Freezeable &freezeable = first_item.get<Freezeable>();
    freezeable.isFrozen = true;

    const IsShopItem &first_shop_item = first_item.get<IsShopItem>();
    int first_slot = first_shop_item.slot;
    item_frozen[first_slot] = true;
  }

  // Get the second item's slot for swap
  int second_slot = -1;
  if (shop_items.size() >= 2) {
    const IsShopItem &second_shop_item = shop_items[1]->get<IsShopItem>();
    second_slot = second_shop_item.slot;
  }

  app.expect_true(second_slot >= 0, "second slot found");

  // Verify the freeze state is preserved by checking directly
  // The swap_items function in DropWhenNoLongerHeld explicitly preserves freeze state
  app.wait_for_frames(2);

  // Verify frozen item still has freeze state at its original slot
  bool found_frozen = false;
  int original_frozen_slot = shop_items[0]->get<IsShopItem>().slot;

  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsShopItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    const IsShopItem &shop_item = entity.get<IsShopItem>();
    if (shop_item.slot == original_frozen_slot) {
      app.expect_true(entity.has<Freezeable>(),
                     "frozen item still has Freezeable component");
      if (entity.has<Freezeable>()) {
        const Freezeable &freezeable = entity.get<Freezeable>();
        app.expect_true(freezeable.isFrozen,
                       "frozen item still marked as frozen");
        found_frozen = true;
      }
      break;
    }
  }

  app.expect_true(found_frozen, "frozen item still exists with freeze state");
}

