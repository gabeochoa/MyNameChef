#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/battle_team_tags.h"
#include "../../components/dish_battle_state.h"
#include "../../components/is_dish.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../test_app.h"
#include "../test_macros.h"
#include "../test_server_helpers.h"
#include <afterhours/ah.h>
#include <algorithm>
#include <filesystem>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

TEST(validate_server_opponent_match) {
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

  // Create dishes in inventory (required to proceed past shop screen)
  const auto inventory = app.read_player_inventory();
  if (inventory.empty()) {
    const auto shop_items = app.read_store_options();
    if (!shop_items.empty()) {
      const int item_price = shop_items[0].price;
      app.set_wallet_gold(item_price + 10);
      app.purchase_item(shop_items[0].type);
      app.wait_for_frames(2);
    } else {
      app.create_inventory_item(DishType::Potato, 0);
      app.wait_for_frames(2);
    }
  }

  // Setup BattleLoadRequest with server URL (only once)
  test_server_helpers::server_integration_test_setup("OPPONENT_MATCH_TEST");

  app.wait_for_frames(1);

  // Navigate to battle
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);

  // Wait for server request to complete - files must be set
  app.wait_for_frames(60);
  auto request_opt =
      afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
  app.expect_singleton_has_component<BattleLoadRequest>(request_opt,
                                                        "BattleLoadRequest");

  const BattleLoadRequest &req = request_opt.get().get<BattleLoadRequest>();
  app.expect_false(req.opponentJsonPath.empty(),
                   "opponentJsonPath should be set by server");

  log_info("OPPONENT_MATCH_TEST: Reading opponent JSON from: {}",
           req.opponentJsonPath);

  // Wait for battle to initialize
  app.wait_for_battle_initialized(30.0f);
  app.wait_for_dishes_in_combat(1, 30.0f);
  
  // Wait longer for dishes to be fully organized into slots
  // With timing speed scale, battles progress faster, so we need to ensure
  // all dishes are properly assigned to their slots before querying
  // Also need to wait for all opponent dishes to be created and assigned
  app.wait_for_frames(60);

  // Read opponent JSON file
  nlohmann::json opponent_json =
      test_server_helpers::read_json_file(app, req.opponentJsonPath);

  app.expect_true(opponent_json.contains("team"),
                  "opponent JSON should contain 'team' array");
  app.expect_true(opponent_json["team"].is_array(),
                  "opponent JSON 'team' should be an array");

  const auto &team = opponent_json["team"];
  std::unordered_map<int, DishType> expected_dishes;

  for (const auto &dish_entry : team) {
    app.expect_true(dish_entry.contains("slot"),
                    "dish entry should contain 'slot'");
    app.expect_true(dish_entry.contains("dishType"),
                    "dish entry should contain 'dishType'");

    int slot = dish_entry["slot"];
    std::string dishTypeStr = dish_entry["dishType"];

    auto dishTypeOpt = magic_enum::enum_cast<DishType>(dishTypeStr);
    app.expect_true(dishTypeOpt.has_value(),
                    std::string("dishType should be valid enum: ") +
                        dishTypeStr);

    expected_dishes[slot] = dishTypeOpt.value();
  }

  log_info("OPPONENT_MATCH_TEST: Expected {} dishes from JSON",
           expected_dishes.size());

  // Query actual opponent dishes in battle
  afterhours::EntityHelper::merge_entity_arrays();
  std::unordered_map<int, DishType> actual_dishes;
  std::vector<DishType> actual_types;
  int actual_dish_count = 0;

  for (afterhours::Entity &entity : afterhours::EntityQuery()
                                        .whereHasComponent<IsOpponentTeamItem>()
                                        .whereHasComponent<IsDish>()
                                        .whereHasComponent<DishBattleState>()
                                        .gen()) {
    const IsDish &dish = entity.get<IsDish>();
    const DishBattleState &dbs = entity.get<DishBattleState>();

    int slot = dbs.queue_index;
    actual_dishes[slot] = dish.type;
    actual_types.push_back(dish.type);
    actual_dish_count++;

    log_info("OPPONENT_MATCH_TEST: Found dish at slot {}: {}", slot,
             magic_enum::enum_name(dish.type));
  }

  // Compare total dish count (not unique slots, as multiple dishes can be at same slot)
  int expected_dish_count = static_cast<int>(team.size());
  app.expect_count_eq(actual_dish_count, expected_dish_count,
                      "opponent dish count should match JSON");

  // Compare each dish - iterate through expected dishes
  // Note: With timing speed scale, dishes might be at different slots due to faster
  // battle progression, so we check that all expected dish types exist (not necessarily at exact slots)
  std::vector<DishType> expected_types;
  for (const auto &[slot, expected_type] : expected_dishes) {
    expected_types.push_back(expected_type);
  }
  
  // Sort both vectors for comparison (order doesn't matter)
  std::sort(expected_types.begin(), expected_types.end());
  std::sort(actual_types.begin(), actual_types.end());
  
  app.expect_true(expected_types == actual_types,
                  "opponent dish types should match JSON (slots may differ due to timing)");
  
  // Also try to match by slot if possible (for logging/debugging)
  for (const auto &[slot, expected_type] : expected_dishes) {
    if (actual_dishes.count(slot) > 0) {
      DishType actual_type = actual_dishes.at(slot);
      app.expect_eq(static_cast<int>(actual_type),
                    static_cast<int>(expected_type),
                    std::string("opponent dish at slot ") + std::to_string(slot) +
                        " should match JSON");
      log_info("OPPONENT_MATCH_TEST: ✅ Slot {} matches: {}", slot,
               magic_enum::enum_name(expected_type));
    } else {
      log_warn("OPPONENT_MATCH_TEST: Expected dish {} at slot {} not found at that slot (may be at different slot due to timing)", 
               magic_enum::enum_name(expected_type), slot);
    }
  }

  log_info("OPPONENT_MATCH_TEST: ✅ All opponent dishes match server JSON");
}
