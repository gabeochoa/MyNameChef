#pragma once

#include "../../components/is_dish.h"
#include "../../components/is_inventory_item.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../query.h"
#include "../../shop.h"
#include "../test_macros.h"
#include <afterhours/ah.h>
#include <algorithm>
#include <vector>

TEST(validate_game_state_save_resume_basic) {
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_frames(20);

  app.once([&] {
    app.set_test_int("initial_round", app.read_round());
    app.set_test_int("initial_tier", app.read_shop_tier());
    TestApp::RerollCostInfo reroll = app.read_reroll_cost();
    app.set_test_int("reroll_base", reroll.base);
    app.set_test_int("reroll_increment", reroll.increment);
    app.set_test_int("reroll_current", reroll.current);

    std::vector<TestShopItemInfo> shop_items = app.read_store_options();
    app.expect_not_empty(shop_items, "shop items should exist");
    app.set_wallet_gold(100);
    app.set_test_int("purchased_type", static_cast<int>(shop_items[0].type));
    app.expect_true(app.try_purchase_item(shop_items[0].type),
                    "purchase should succeed");
  });
  app.wait_for_frames(5);

  app.once([&] {
    app.set_wallet_gold(app.read_wallet_gold() + 50);
    Health &health =
        afterhours::EntityHelper::get_singleton<Health>().get().get<Health>();
    health.current = std::max(1, health.current - 1);
    app.set_test_int("saved_gold", app.read_wallet_gold());
    app.set_test_int("saved_health", app.read_player_health());
    app.set_test_int("saved_inventory_size",
                     static_cast<int>(app.read_player_inventory().size()));
  });
  app.wait_for_frames(2);

  app.trigger_game_state_save();
  app.wait_for_frames(5);
  app.once([&] {
    app.expect_true(app.save_file_exists(),
                    "save file should exist after save");
  });

  app.once([&] {
    for (afterhours::Entity &entity :
         afterhours::EntityQuery({.force_merge = true})
             .whereHasComponent<IsInventoryItem>()
             .whereHasComponent<IsDish>()
             .gen()) {
      entity.cleanup = true;
    }
    app.set_wallet_gold(0);
    Health &health =
        afterhours::EntityHelper::get_singleton<Health>().get().get<Health>();
    health.current = 5;
    health.max = 5;
  });
  app.wait_for_frames(5);
  app.once([&] {
    app.expect_wallet_has(0, "gold should be reset before load");
    app.expect_true(app.read_player_inventory().empty(),
                    "inventory should be cleared before load");
  });

  app.trigger_game_state_load();
  app.wait_for_frames(10);

  app.expect_wallet_has(app.get_test_int("saved_gold").value(),
                        "gold should be restored");
  app.expect_eq(app.read_player_health(),
                app.get_test_int("saved_health").value(),
                "health should be restored");
  app.expect_eq(app.read_round(), app.get_test_int("initial_round").value(),
                "round should be restored");
  app.expect_eq(app.read_shop_tier(), app.get_test_int("initial_tier").value(),
                "shop tier should be restored");

  TestApp::RerollCostInfo restored_reroll = app.read_reroll_cost();
  app.expect_eq(restored_reroll.base, app.get_test_int("reroll_base").value(),
                "reroll cost base should be restored");
  app.expect_eq(restored_reroll.increment,
                app.get_test_int("reroll_increment").value(),
                "reroll cost increment should be restored");
  app.expect_eq(restored_reroll.current,
                app.get_test_int("reroll_current").value(),
                "reroll cost current should be restored");

  std::vector<TestDishInfo> restored_inventory = app.read_player_inventory();
  app.expect_count_eq(static_cast<int>(restored_inventory.size()),
                      app.get_test_int("saved_inventory_size").value(),
                      "inventory size should match");

  DishType purchased_type =
      static_cast<DishType>(app.get_test_int("purchased_type").value());
  bool found_purchased_item = std::any_of(
      restored_inventory.begin(), restored_inventory.end(),
      [purchased_type](const TestDishInfo &d) { return d.type == purchased_type; });
  app.expect_true(found_purchased_item,
                  "purchased item should be in restored inventory");
}
