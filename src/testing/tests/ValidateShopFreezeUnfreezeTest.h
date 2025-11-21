#pragma once

#include "../../components/is_dish.h"
#include "../../components/is_shop_item.h"
#include "../../dish_types.h"
#include "../../query.h"
#include "../../shop.h"
#include "../test_macros.h"
#include <string>
#include <vector>

TEST(validate_shop_freeze_unfreeze) {
  (void)app;
  // Test that frozen shop items can be manually unfrozen by toggling freeze state

  app.navigate_to_shop();
  app.wait_for_ui_exists("Reroll (1 gold)");
  app.wait_for_frames(5);

  // Find a shop item and freeze it
  afterhours::Entity *item_to_toggle = nullptr;

  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsShopItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    if (!item_to_toggle) {
      item_to_toggle = &entity;

      if (!entity.has<Freezeable>()) {
        entity.addComponent<Freezeable>(false);
      }
      Freezeable &freezeable = entity.get<Freezeable>();
      freezeable.isFrozen = true;
      break;
    }
  }

  app.expect_true(item_to_toggle != nullptr, "found shop item to toggle");
  app.expect_true(item_to_toggle->has<Freezeable>(),
                 "item has Freezeable component");
  app.expect_true(item_to_toggle->get<Freezeable>().isFrozen,
                 "item is frozen initially");

  // Manually toggle freeze state (simulating click on freeze icon)
  Freezeable &freezeable = item_to_toggle->get<Freezeable>();
  freezeable.isFrozen = !freezeable.isFrozen;

  app.expect_false(freezeable.isFrozen, "item is unfrozen after toggle");

  // Toggle again to verify it can be frozen again
  freezeable.isFrozen = !freezeable.isFrozen;
  app.expect_true(freezeable.isFrozen, "item is frozen again after second toggle");

  // Verify the state persists
  app.wait_for_frames(5);
  if (item_to_toggle->has<Freezeable>()) {
    const Freezeable &check_freezeable = item_to_toggle->get<Freezeable>();
    app.expect_true(check_freezeable.isFrozen,
                   "freeze state persists after frames");
  }
}

