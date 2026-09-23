#pragma once

#include "../../components/is_dish.h"
#include "../../components/is_drop_slot.h"
#include "../../components/is_inventory_item.h"
#include "../../components/is_shop_item.h"
#include "../../dish_types.h"
#include "../../query.h"
#include "../../shop.h"
#include "../test_app.h"
#include "../test_macros.h"

TEST(validate_dish_selling) {
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_ui_exists("Next Round");
  app.wait_for_frames(10);

  app.once([&] { app.set_wallet_gold(0); });

  int test_slot = app.remember_int("test_slot", app.find_free_inventory_slot());
  app.expect_true(test_slot >= 0, "found free inventory slot");

  app.create_inventory_item(DishType::Potato, test_slot);
  app.wait_for_frames(5);

  app.once([&] {
    afterhours::OptEntity item_opt = app.find_inventory_item_by_slot(test_slot);
    app.expect_true(item_opt.has_value(), "created inventory item found");
    app.set_test_int("item_id", item_opt.asE().id);
    app.expect_eq(app.read_wallet_gold(), 0, "gold starts at 0");
    app.expect_true(app.simulate_sell(item_opt.asE()),
                    "sell simulation succeeded");
  });
  app.wait_for_frames(5);

  app.once([&] {
    afterhours::OptEntity sold_item_opt =
        EQ({.force_merge = true})
            .whereID(app.get_test_int("item_id").value())
            .gen_first();
    bool item_removed =
        !sold_item_opt.has_value() || sold_item_opt.asE().cleanup;
    app.expect_true(item_removed, "sold item was removed or marked for cleanup");
    app.expect_eq(app.read_wallet_gold(), 1, "gold increased by 1 after sell");

    afterhours::OptEntity original_slot_opt = app.find_drop_slot(test_slot);
    app.expect_true(original_slot_opt.has_value(), "original slot found");
    app.expect_false(original_slot_opt.asE().get<IsDropSlot>().occupied,
                     "original slot freed after sell");
  });

  app.create_inventory_item(DishType::Potato, test_slot);
  int free_slot_2 =
      app.remember_int("free_slot_2", app.find_free_inventory_slot());
  app.expect_true(free_slot_2 >= 0, "found second free inventory slot");
  app.create_inventory_item(DishType::Salmon, free_slot_2);
  app.wait_for_frames(5);

  app.once([&] {
    afterhours::OptEntity item1_opt = app.find_inventory_item_by_slot(test_slot);
    afterhours::OptEntity item2_opt =
        app.find_inventory_item_by_slot(free_slot_2);
    app.expect_true(item1_opt.has_value(), "first item for multiple sell found");
    app.expect_true(item2_opt.has_value(),
                    "second item for multiple sell found");
    app.expect_true(app.simulate_sell(item1_opt.asE()), "first sell succeeded");
  });
  app.wait_for_frames(5);

  app.once([&] {
    app.expect_eq(app.read_wallet_gold(), 2,
                  "gold increased by 1 after first sell");
    afterhours::OptEntity item2_opt =
        app.find_inventory_item_by_slot(free_slot_2);
    app.expect_true(item2_opt.has_value(), "second item still present");
    app.expect_true(app.simulate_sell(item2_opt.asE()), "second sell succeeded");
  });
  app.wait_for_frames(5);

  app.once([&] {
    app.expect_eq(app.read_wallet_gold(), 3,
                  "gold increased by 2 after two sells");
    app.set_wallet_gold(100);
    afterhours::OptEntity shop_item_opt = EQ({.force_merge = true})
                                              .whereHasComponent<IsShopItem>()
                                              .whereHasComponent<IsDish>()
                                              .gen_first();
    app.expect_true(shop_item_opt.has_value(), "found shop item");
    app.set_test_int("shop_item_id", shop_item_opt.asE().id);
    app.expect_false(app.simulate_sell(shop_item_opt.asE()),
                     "shop items cannot be sold (should return false)");
  });
  app.wait_for_frames(5);

  afterhours::OptEntity shop_item_after_opt =
      EQ({.force_merge = true})
          .whereID(app.get_test_int("shop_item_id").value())
          .gen_first();
  app.expect_true(shop_item_after_opt.has_value(),
                  "shop item still exists after sell attempt");
  app.expect_eq(app.read_wallet_gold(), 100,
                "gold unchanged after shop sell attempt");
}
