#pragma once

#include "../components/battle_load_request.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../utils/http_helpers.h"
#include "test_app.h"
#include <afterhours/ah.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

namespace test_server_helpers {

// Format BattleFingerprint result as hex string (matching server format)
inline std::string format_checksum(uint64_t fingerprint) {
  std::stringstream ss;
  ss << std::hex << std::setfill('0') << std::setw(16) << fingerprint;
  return ss.str();
}

// Setup BattleLoadRequest with server URL for integration tests
// Usage: test_server_helpers::server_integration_test_setup("TEST_NAME");
inline void server_integration_test_setup(const std::string &test_name) {
  static bool setup_complete = false;

  if (setup_complete) {
    return;
  }

  log_info("{}: Setting up BattleLoadRequest with server URL...", test_name);

  std::string server_url = http_helpers::get_server_url();
  log_info("{}: Using server URL: {}", test_name, server_url);

  afterhours::Entity &request_entity = afterhours::EntityHelper::createEntity();
  request_entity.addComponent<BattleLoadRequest>();
  BattleLoadRequest &request = request_entity.get<BattleLoadRequest>();

  request.serverUrl = server_url;
  request.playerJsonPath = "";
  request.opponentJsonPath = "";
  request.loaded = false;
  request.serverRequestPending = false;

  afterhours::EntityHelper::registerSingleton<BattleLoadRequest>(
      request_entity);
  afterhours::EntityHelper::merge_entity_arrays();

  log_info("{}: BattleLoadRequest configured with server URL", test_name);
  setup_complete = true;
}

// Read JSON file and return parsed JSON object
// Uses app.expect_* to validate and app.fail() on errors
inline nlohmann::json read_json_file(TestApp &app,
                                     const std::string &filepath) {
  app.expect_true(std::filesystem::exists(filepath),
                  "JSON file should exist: " + filepath);

  std::ifstream file(filepath);
  app.expect_true(file.is_open(), "JSON file should be readable: " + filepath);

  nlohmann::json json_data;
  file >> json_data;
  file.close();

  return json_data;
}

// Convert player inventory team to JSON format for server requests
inline nlohmann::json team_to_json() {
  nlohmann::json player_team_json = nlohmann::json::object();
  nlohmann::json team_array = nlohmann::json::array();

  for (afterhours::Entity &entity : afterhours::EntityQuery()
                                        .whereHasComponent<IsInventoryItem>()
                                        .whereHasComponent<IsDish>()
                                        .gen()) {
    const IsInventoryItem &inv = entity.get<IsInventoryItem>();
    const IsDish &dish = entity.get<IsDish>();

    nlohmann::json dish_entry;
    dish_entry["slot"] = inv.slot;
    dish_entry["dishType"] = std::string(magic_enum::enum_name(dish.type));
    dish_entry["level"] = 1;
    team_array.push_back(dish_entry);
  }

  player_team_json["team"] = team_array;
  player_team_json["debug"] = false;

  return player_team_json;
}
} // namespace test_server_helpers
