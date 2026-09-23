#pragma once

#include "../../components/is_dish.h"
#include "../../components/is_inventory_item.h"
#include "../../components/is_shop_item.h"
#include "../../dish_types.h"
#include "../../query.h"
#include "../../shop.h"
#include "../test_macros.h"

TEST(validate_shop_freeze_purchase) {
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_ui_exists("Reroll (1 gold)");
  app.wait_for_frames(10);

  app.once([&] {
    afterhours::OptEntity item_opt = afterhours::EntityQuery({.force_merge = true})
                                         .whereHasComponent<IsShopItem>()
                                         .whereHasComponent<IsDish>()
                                         .gen_first();
    app.expect_true(item_opt.has_value(), "found shop item to purchase");
    afterhours::Entity &item = item_opt.asE();
    if (!item.has<Freezeable>()) {
      item.addComponent<Freezeable>(false);
    }
    item.get<Freezeable>().isFrozen = true;
    app.expect_true(item.get<Freezeable>().isFrozen,
                    "item is frozen before purchase");

    const DishType item_type = item.get<IsDish>().type;
    app.set_test_int("item_type", static_cast<int>(item_type));
    app.set_test_int("item_slot", item.get<IsShopItem>().slot);
    app.set_wallet_gold(get_dish_info(item_type).price + 10);
    app.expect_count_lt(static_cast<int>(app.read_player_inventory().size()), 7,
                        "inventory has space for purchase");
  });

  const DishType item_type =
      static_cast<DishType>(app.get_test_int("item_type").value());
  const int item_slot = app.get_test_int("item_slot").value();

  app.purchase_item(item_type);
  app.wait_for_frames(5);

  bool found_in_inventory = false;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsInventoryItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    if (entity.get<IsDish>().type == item_type) {
      found_in_inventory = true;
      app.expect_false(entity.has<Freezeable>(),
                       "purchased item no longer has Freezeable component");
      break;
    }
  }
  app.expect_true(found_in_inventory, "purchased item is in inventory");

  bool found_in_shop = false;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsShopItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    if (entity.get<IsShopItem>().slot == item_slot &&
        entity.get<IsDish>().type == item_type) {
      found_in_shop = true;
      break;
    }
  }
  app.expect_false(found_in_shop, "purchased item no longer in shop");
}
