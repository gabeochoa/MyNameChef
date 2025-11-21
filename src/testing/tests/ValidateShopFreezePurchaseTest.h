#pragma once

#include "../../components/is_dish.h"
#include "../../components/is_inventory_item.h"
#include "../../components/is_shop_item.h"
#include "../../dish_types.h"
#include "../../query.h"
#include "../../shop.h"
#include "../test_macros.h"
#include <string>
#include <vector>

TEST(validate_shop_freeze_purchase) {
  (void)app;
  // Test that frozen shop items lose their freeze state when purchased

  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_ui_exists("Reroll (1 gold)");
  app.wait_for_frames(10);

  // Wait for shop items to be created using read_store_options
  auto shop_items = app.read_store_options();
  for (int attempt = 0; attempt < 20 && shop_items.empty(); attempt++) {
    app.wait_for_frames(1);
    shop_items = app.read_store_options();
  }

  app.expect_not_empty(shop_items, "shop items exist");

  // Find a shop item and freeze it
  afterhours::Entity *item_to_purchase = nullptr;
  int item_slot = -1;
  DishType item_type = DishType::Potato;

  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsShopItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    if (!item_to_purchase) {
      item_to_purchase = &entity;
      const IsShopItem &shop_item = entity.get<IsShopItem>();
      const IsDish &dish = entity.get<IsDish>();
      item_slot = shop_item.slot;
      item_type = dish.type;

      if (!entity.has<Freezeable>()) {
        entity.addComponent<Freezeable>(false);
      }
      Freezeable &freezeable = entity.get<Freezeable>();
      freezeable.isFrozen = true;
      break;
    }
  }

  app.expect_true(item_to_purchase != nullptr, "found shop item to purchase");
  app.expect_true(item_to_purchase->has<Freezeable>(),
                  "item has Freezeable component");
  app.expect_true(item_to_purchase->get<Freezeable>().isFrozen,
                  "item is frozen before purchase");

  // Ensure we have enough gold and an empty inventory slot
  const int price = get_dish_info(item_type).price;
  app.set_wallet_gold(price + 10);

  const auto inventory = app.read_player_inventory();
  app.expect_count_lt(static_cast<int>(inventory.size()), 7,
                      "inventory has space for purchase");

  // Purchase the frozen item
  app.purchase_item(item_type);
  app.wait_for_frames(5);

  // Verify the item is now in inventory and no longer has Freezeable component
  bool found_in_inventory = false;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsInventoryItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    const IsDish &dish = entity.get<IsDish>();
    if (dish.type == item_type) {
      found_in_inventory = true;
      app.expect_false(entity.has<Freezeable>(),
                       "purchased item no longer has Freezeable component");
      break;
    }
  }

  app.expect_true(found_in_inventory, "purchased item is in inventory");

  // Verify the item is no longer in shop
  bool found_in_shop = false;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsShopItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    const IsShopItem &shop_item = entity.get<IsShopItem>();
    const IsDish &dish = entity.get<IsDish>();
    if (shop_item.slot == item_slot && dish.type == item_type) {
      found_in_shop = true;
      break;
    }
  }

  app.expect_false(found_in_shop, "purchased item no longer in shop");
}
