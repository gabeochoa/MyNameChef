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
  (void)app;

  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_ui_exists("Next Round");
  app.wait_for_frames(10);

  app.set_wallet_gold(0);

  int test_slot = app.find_free_inventory_slot();
  app.expect_true(test_slot >= 0, "found free inventory slot");

  app.create_inventory_item(DishType::Potato, test_slot);
  app.wait_for_frames(5);

  auto item_opt = app.find_inventory_item_by_slot(test_slot);
  app.expect_true(item_opt.has_value(), "created inventory item found");

  afterhours::Entity &item = item_opt.asE();
  afterhours::EntityID item_id = item.id;
  int gold_before = app.read_wallet_gold();
  app.expect_eq(gold_before, 0, "gold starts at 0");

  bool sell_succeeded = app.simulate_sell(item);
  app.expect_true(sell_succeeded, "sell simulation succeeded");

  app.wait_for_frames(5);

  // Entities merged by system loop, regular query is sufficient
  auto sold_item_opt = EQ().whereID(item_id).gen_first();
  bool item_removed =
      !sold_item_opt.has_value() ||
      (sold_item_opt.has_value() && sold_item_opt.asE().cleanup);
  app.expect_true(item_removed, "sold item was removed or marked for cleanup");

  int gold_after = app.read_wallet_gold();
  app.expect_eq(gold_after, gold_before + 1, "gold increased by 1 after sell");

  auto original_slot_opt = app.find_drop_slot(test_slot);
  app.expect_true(original_slot_opt.has_value(), "original slot found");
  afterhours::Entity &original_slot = original_slot_opt.asE();
  app.expect_false(original_slot.get<IsDropSlot>().occupied,
                   "original slot freed after sell");

  int gold_before_multiple = app.read_wallet_gold();

  app.create_inventory_item(DishType::Potato, test_slot);
  app.wait_for_frames(5);

  int free_slot_2 = app.find_free_inventory_slot();
  app.expect_true(free_slot_2 >= 0, "found second free inventory slot");
  app.create_inventory_item(DishType::Salmon, free_slot_2);
  app.wait_for_frames(5);

  auto item1_opt = app.find_inventory_item_by_slot(test_slot);
  auto item2_opt = app.find_inventory_item_by_slot(free_slot_2);
  app.expect_true(item1_opt.has_value(), "first item for multiple sell found");
  app.expect_true(item2_opt.has_value(), "second item for multiple sell found");

  afterhours::Entity &item1 = item1_opt.asE();
  afterhours::Entity &item2 = item2_opt.asE();

  bool sell1_succeeded = app.simulate_sell(item1);
  app.expect_true(sell1_succeeded, "first sell succeeded");

  app.wait_for_frames(5);

  int gold_after_first = app.read_wallet_gold();
  app.expect_eq(gold_after_first, gold_before_multiple + 1,
                "gold increased by 1 after first sell");

  bool sell2_succeeded = app.simulate_sell(item2);
  app.expect_true(sell2_succeeded, "second sell succeeded");

  app.wait_for_frames(5);

  int gold_after_second = app.read_wallet_gold();
  app.expect_eq(gold_after_second, gold_before_multiple + 2,
                "gold increased by 2 after two sells");

  app.set_wallet_gold(100);
  int free_shop_slot = app.find_free_shop_slot();
  app.expect_true(free_shop_slot >= 0, "found free shop slot");

  afterhours::Entity &shop_item =
      make_shop_item(free_shop_slot, DishType::Potato);
  afterhours::EntityID shop_item_id = shop_item.id;

  bool shop_sell_attempted = app.simulate_sell(shop_item);
  app.expect_false(shop_sell_attempted,
                   "shop items cannot be sold (should return false)");

  app.wait_for_frames(5);

  // Entities merged by system loop, regular query is sufficient
  auto shop_item_after_opt = EQ().whereID(shop_item_id).gen_first();
  app.expect_true(shop_item_after_opt.has_value(),
                  "shop item still exists after sell attempt");
}
