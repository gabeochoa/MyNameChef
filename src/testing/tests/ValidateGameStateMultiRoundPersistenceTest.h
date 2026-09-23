#pragma once

#include "../../components/is_dish.h"
#include "../../components/is_inventory_item.h"
#include "../../game_state_manager.h"
#include "../../query.h"
#include "../../shop.h"
#include "../test_macros.h"
#include <afterhours/ah.h>
#include <vector>

namespace ValidateGameStateMultiRoundPersistenceTestHelpers {

static void buy_first_affordable_item(TestApp &app) {
  std::vector<TestShopItemInfo> shop_items = app.read_store_options();
  if (shop_items.empty() || !app.can_afford_purchase(shop_items[0].type)) {
    return;
  }
  app.try_purchase_item(shop_items[0].type);
}

} // namespace ValidateGameStateMultiRoundPersistenceTestHelpers

TEST(validate_game_state_multi_round_persistence) {
  using namespace ValidateGameStateMultiRoundPersistenceTestHelpers;
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_frames(20);

  app.once([&] { buy_first_affordable_item(app); });
  app.wait_for_frames(5);

  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_frames(10);

  app.wait_for_battle_complete(30.0f);
  app.wait_for_ui_exists("Back to Shop", 10.0f);
  app.click("Back to Shop");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_frames(20);

  app.once([&] {
    app.expect_count_gt(app.read_round(), 1, "round should have incremented");
    buy_first_affordable_item(app);
    Health &health =
        afterhours::EntityHelper::get_singleton<Health>().get().get<Health>();
    health.current = std::max(1, health.current - 1);
  });
  app.wait_for_frames(5);

  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_frames(10);

  app.wait_for_battle_complete(30.0f);
  app.wait_for_ui_exists("Back to Shop", 10.0f);
  app.click("Back to Shop");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_frames(20);

  app.once([&] {
    app.set_test_int("round3_gold", app.read_wallet_gold());
    app.set_test_int("round3_health", app.read_player_health());
    app.set_test_int("round3_round", app.read_round());
    app.set_test_int("round3_tier", app.read_shop_tier());
    app.set_test_int("round3_inventory",
                     static_cast<int>(app.read_player_inventory().size()));
  });

  app.trigger_game_state_save();
  app.wait_for_frames(5);
  app.expect_true(app.save_file_exists(), "save file should exist");

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
    afterhours::Entity &round_entity =
        afterhours::EntityHelper::get_singleton<Round>().get();
    if (round_entity.has<Round>()) {
      round_entity.get<Round>().current = 1;
    }
  });
  app.wait_for_frames(5);

  app.trigger_game_state_load();
  app.wait_for_frames(10);

  app.expect_wallet_has(app.get_test_int("round3_gold").value(),
                        "gold should match round 3 state");
  app.expect_eq(app.read_player_health(),
                app.get_test_int("round3_health").value(),
                "health should match round 3 state");
  app.expect_eq(app.read_round(), app.get_test_int("round3_round").value(),
                "round should match round 3 state");
  app.expect_eq(app.read_shop_tier(), app.get_test_int("round3_tier").value(),
                "shop tier should match round 3 state");
  app.expect_count_eq(static_cast<int>(app.read_player_inventory().size()),
                      app.get_test_int("round3_inventory").value(),
                      "inventory count should match round 3 state");
}
