#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/battle_team_tags.h"
#include "../../components/dish_battle_state.h"
#include "../../components/is_dish.h"
#include "../../game_state_manager.h"
#include "../test_app.h"
#include "../test_macros.h"
#include "../test_server_helpers.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

TEST(validate_server_opponent_match) {
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

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

  for (afterhours::Entity &entity : afterhours::EntityQuery()
                                        .whereHasComponent<IsOpponentTeamItem>()
                                        .whereHasComponent<IsDish>()
                                        .whereHasComponent<DishBattleState>()
                                        .gen()) {
    const IsDish &dish = entity.get<IsDish>();
    const DishBattleState &dbs = entity.get<DishBattleState>();

    int slot = dbs.queue_index;
    actual_dishes[slot] = dish.type;

    log_info("OPPONENT_MATCH_TEST: Found dish at slot {}: {}", slot,
             magic_enum::enum_name(dish.type));
  }

  app.expect_count_eq(static_cast<int>(actual_dishes.size()),
                      static_cast<int>(expected_dishes.size()),
                      "opponent dish count should match JSON");

  // Compare each dish - iterate through expected dishes
  for (const auto &[slot, expected_type] : expected_dishes) {
    app.expect_true(actual_dishes.count(slot) > 0,
                    std::string("opponent dish should exist at slot ") +
                        std::to_string(slot));

    DishType actual_type = actual_dishes.at(slot);
    app.expect_eq(static_cast<int>(actual_type),
                  static_cast<int>(expected_type),
                  std::string("opponent dish at slot ") + std::to_string(slot) +
                      " should match JSON");

    log_info("OPPONENT_MATCH_TEST: ✅ Slot {} matches: {}", slot,
             magic_enum::enum_name(expected_type));
  }

  log_info("OPPONENT_MATCH_TEST: ✅ All opponent dishes match server JSON");
}
