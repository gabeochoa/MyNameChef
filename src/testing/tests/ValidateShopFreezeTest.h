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

TEST(validate_shop_freeze) {
  (void)app;
  // Test that frozen shop items persist through rerolls

  // Navigate to shop screen
  app.navigate_to_shop();
  app.wait_for_ui_exists("Reroll (1 gold)");
  app.wait_for_frames(5); // Allow shop systems to fully initialize

  // Get initial shop items with their IDs and dish types
  std::map<int, DishType> initial_items; // entity ID -> dish type
  std::map<int, int> item_slots;         // entity ID -> slot number
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsShopItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    const IsDish &dish = entity.get<IsDish>();
    const IsShopItem &shop_item = entity.get<IsShopItem>();
    initial_items[entity.id] = dish.type;
    item_slots[entity.id] = shop_item.slot;
  }
  app.expect_count_gt(static_cast<int>(initial_items.size()), 0,
                      "initial shop items exist");

  // Freeze the first two items (if we have at least 2 items)
  int frozen_count = 0;
  const int target_frozen = 2;
  std::vector<int> frozen_entity_ids;

  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsShopItem>()
           .whereHasComponent<IsDish>()
           .whereHasComponent<Freezeable>()
           .gen()) {
    if (frozen_count >= target_frozen) {
      break;
    }

    Freezeable &freezeable = entity.get<Freezeable>();
    freezeable.isFrozen = true;
    frozen_entity_ids.push_back(entity.id);
    frozen_count++;
  }

  app.expect_count_gte(frozen_count, 1, "at least one item was frozen");

  // Set wallet to have enough gold for reroll
  afterhours::RefEntity wallet_ref =
      afterhours::EntityHelper::get_singleton<Wallet>();
  app.expect_true(wallet_ref.get().has<Wallet>(), "Wallet singleton exists");
  Wallet &wallet = wallet_ref.get().get<Wallet>();
  wallet.gold = 200;

  // Click reroll
  app.click("Reroll (1 gold)");
  app.wait_for_frames(5); // Wait for reroll to process

  // Verify frozen items still exist with same dish types
  for (int frozen_id : frozen_entity_ids) {
    afterhours::OptEntity frozen_entity_opt =
        afterhours::EntityQuery({.force_merge = true})
            .whereID(frozen_id)
            .gen_first();
    app.expect_true(frozen_entity_opt.has_value(),
                    "frozen entity " + std::to_string(frozen_id) +
                        " still exists");

    if (frozen_entity_opt.has_value()) {
      afterhours::Entity &frozen_entity = frozen_entity_opt.asE();
      app.expect_true(frozen_entity.has<IsDish>(),
                      "frozen entity has IsDish component");
      app.expect_true(frozen_entity.has<IsShopItem>(),
                      "frozen entity has IsShopItem component");
      app.expect_true(frozen_entity.has<Freezeable>(),
                      "frozen entity has Freezeable component");

      const IsDish &dish = frozen_entity.get<IsDish>();
      DishType expected_type = initial_items[frozen_id];
      app.expect_eq(static_cast<int>(dish.type),
                    static_cast<int>(expected_type),
                    "frozen item " + std::to_string(frozen_id) +
                        " kept same dish type after reroll");

      const Freezeable &freezeable = frozen_entity.get<Freezeable>();
      app.expect_true(freezeable.isFrozen, "frozen item " +
                                               std::to_string(frozen_id) +
                                               " still marked as frozen");

      const IsShopItem &shop_item = frozen_entity.get<IsShopItem>();
      int expected_slot = item_slots[frozen_id];
      app.expect_eq(shop_item.slot, expected_slot,
                    "frozen item " + std::to_string(frozen_id) +
                        " kept same slot after reroll");
    }
  }

  // Verify that there are unfrozen items (proves reroll happened and didn't
  // freeze everything)
  int total_items =
      static_cast<int>(afterhours::EntityQuery({.force_merge = true})
                           .whereHasComponent<IsShopItem>()
                           .gen_count());
  int unfrozen_count = total_items - frozen_count;
  app.expect_count_gt(unfrozen_count, 0, "there are unfrozen items");
}
