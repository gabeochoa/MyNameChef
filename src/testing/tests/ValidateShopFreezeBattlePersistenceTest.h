#pragma once

#include "../../components/is_dish.h"
#include "../../components/is_shop_item.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../query.h"
#include "../../shop.h"
#include "../test_macros.h"
#include <cstddef>
#include <map>
#include <string>
#include <vector>

TEST(validate_shop_freeze_battle_persistence) {
  (void)app;
  // Test that frozen shop items persist after going to battle and returning

  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Reroll (1 gold)");
  app.wait_for_frames(10);

  // Get initial shop items and freeze some
  std::map<int, DishType> frozen_items; // slot -> dish type
  std::map<int, int> frozen_entity_ids; // slot -> entity id

  int frozen_count = 0;
  const int target_frozen = 2;

  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsShopItem>()
           .whereHasComponent<IsDish>()
           .whereHasComponent<Freezeable>()
           .gen()) {
    if (frozen_count >= target_frozen) {
      break;
    }

    const IsShopItem &shop_item = entity.get<IsShopItem>();
    const IsDish &dish = entity.get<IsDish>();
    Freezeable &freezeable = entity.get<Freezeable>();

    freezeable.isFrozen = true;
    frozen_items[shop_item.slot] = dish.type;
    frozen_entity_ids[shop_item.slot] = entity.id;
    frozen_count++;
  }

  app.expect_count_gte(frozen_count, 1, "at least one item was frozen");

  // Ensure we have inventory items to proceed to battle
  const auto inventory = app.read_player_inventory();
  if (inventory.empty()) {
    app.create_inventory_item(DishType::Potato, 0);
    app.wait_for_frames(2);
  }

  // Navigate to battle
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_frames(5);

  // Skip battle and return to shop
  app.wait_for_ui_exists("Skip to Results", 5.0f);
  app.click("Skip to Results");
  app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
  app.wait_for_frames(5);

  app.wait_for_ui_exists("Back to Shop", 5.0f);
  app.click("Back to Shop");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Reroll (1 gold)");
  app.wait_for_frames(15);

  // Wait for shop items to be restored using read_store_options
  auto shop_items_after = app.read_store_options();
  for (int attempt = 0; attempt < 30 && shop_items_after.empty(); attempt++) {
    app.wait_for_frames(1);
    shop_items_after = app.read_store_options();
  }

  app.expect_not_empty(shop_items_after, "shop items restored after battle");

  // Wait a bit more for systems to fully process
  app.wait_for_frames(5);

  // Verify frozen items still exist - check for any frozen items first
  int frozen_items_found = 0;
  std::map<int, DishType> found_frozen_items; // slot -> dish type

  for (int attempt = 0; attempt < 20; attempt++) {
    if (attempt > 0) {
      app.wait_for_frames(1);
    }
    for (afterhours::Entity &entity :
         afterhours::EntityQuery({.force_merge = true})
             .whereHasComponent<IsShopItem>()
             .whereHasComponent<IsDish>()
             .whereHasComponent<Freezeable>()
             .gen()) {
      const Freezeable &freezeable = entity.get<Freezeable>();
      if (freezeable.isFrozen) {
        const IsShopItem &shop_item = entity.get<IsShopItem>();
        const IsDish &dish = entity.get<IsDish>();
        if (found_frozen_items.find(shop_item.slot) ==
            found_frozen_items.end()) {
          found_frozen_items[shop_item.slot] = dish.type;
          frozen_items_found++;
        }
      }
    }
    if (frozen_items_found >= frozen_count) {
      break;
    }
  }

  app.expect_count_gte(frozen_items_found, frozen_count,
                       "frozen items still exist after battle");

  // Verify the frozen items match what we froze
  for (const auto &[slot, expected_type] : frozen_items) {
    auto it = found_frozen_items.find(slot);
    app.expect_true(it != found_frozen_items.end(),
                    "frozen item at slot " + std::to_string(slot) +
                        " still exists after battle");
    if (it != found_frozen_items.end()) {
      app.expect_eq(static_cast<int>(it->second),
                    static_cast<int>(expected_type),
                    "frozen item at slot " + std::to_string(slot) +
                        " kept same dish type after battle");
    }
  }
}
