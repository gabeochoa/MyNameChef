#pragma once

#include "../../components/battle_load_request.h"
#include "../../game_state_manager.h"
#include "../../utils/code_hash_generated.h"
#include "../test_app.h"
#include "../test_macros.h"
#include "../test_server_helpers.h"
#include <afterhours/ah.h>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <string>

TEST(validate_code_hash) {
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

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

  test_server_helpers::server_integration_test_setup("CODE_HASH_TEST");

  app.wait_for_frames(1);

  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);

  app.wait_for_frames(60);

  auto request_opt =
      afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
  app.expect_singleton_has_component<BattleLoadRequest>(request_opt,
                                                         "BattleLoadRequest");

  const BattleLoadRequest &req = request_opt.get().get<BattleLoadRequest>();
  app.expect_false(req.playerJsonPath.empty(),
                   "playerJsonPath should be set by server");
  app.expect_false(req.opponentJsonPath.empty(),
                   "opponentJsonPath should be set by server");

  log_info("CODE_HASH_TEST: Server request completed successfully");
  log_info("  Expected hash: {}", SHARED_CODE_HASH);
  log_info("  ✅ Test passed - hash validation working correctly");
}

TEST(validate_code_hash_mismatch_rejection) {
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

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

  test_server_helpers::server_integration_test_setup("CODE_HASH_MISMATCH_TEST");

  app.wait_for_frames(1);

  httplib::Client client("localhost", 8080);
  client.set_read_timeout(5, 0);
  client.set_connection_timeout(2, 0);

  nlohmann::json test_request;
  test_request["team"] = nlohmann::json::array();
  nlohmann::json dish_entry;
  dish_entry["slot"] = 0;
  dish_entry["dishType"] = "Potato";
  dish_entry["level"] = 1;
  test_request["team"].push_back(dish_entry);
  test_request["codeHash"] = "wrong_hash_value_for_testing";

  auto res = client.Post("/battle", test_request.dump(), "application/json");

  app.expect_true(res != nullptr, "Server should respond");
  app.expect_true(res->status == 400, "Server should reject with 400 for hash mismatch");

  nlohmann::json error_response = nlohmann::json::parse(res->body);
  app.expect_true(error_response.contains("error"), "Error response should contain error field");
  app.expect_true(error_response.contains("clientHash"), "Error response should contain clientHash");
  app.expect_true(error_response.contains("serverHash"), "Error response should contain serverHash");

  log_info("CODE_HASH_MISMATCH_TEST: Server correctly rejected mismatched hash");
  log_info("  Error: {}", error_response["error"].get<std::string>());
  log_info("  Client hash: {}", error_response["clientHash"].get<std::string>());
  log_info("  Server hash: {}", error_response["serverHash"].get<std::string>());
  log_info("  ✅ Test passed - hash mismatch rejection working correctly");
}

