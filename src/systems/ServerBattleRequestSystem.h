#pragma once

#include "../components/battle_load_request.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../components/replay_state.h"
#include "../game_state_manager.h"
#include "../log.h"
#include "../server/file_storage.h"
#include "../systems/GameStateSaveSystem.h"
#include "../utils/code_hash_generated.h"
#include "../utils/http_helpers.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <fstream>
#include <httplib.h>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <vector>

struct ServerBattleRequestSystem : afterhours::System<BattleLoadRequest> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &, BattleLoadRequest &request,
                     float) override {
    if (request.serverUrl.empty()) {
      return;
    }

    if (request.serverRequestPending) {
      return;
    }

    if (!request.playerJsonPath.empty() || !request.opponentJsonPath.empty()) {
      return;
    }

    request.serverRequestPending = true;
    log_info("SERVER_BATTLE_REQUEST: Starting server battle request to {}",
             request.serverUrl);

    GameStateSaveSystem save_system;
    auto save_result = save_system.save_game_state();
    if (save_result.success) {
      log_info("GAME_STATE_SAVE: Saved game state before battle start");
    } else {
      log_error(
          "GAME_STATE_SAVE: Failed to save game state before battle start");
    }

    nlohmann::json player_team_json = build_player_team_json();
    if (player_team_json.empty()) {
      log_error("SERVER_BATTLE_REQUEST: Failed to build player team JSON");
      request.serverRequestPending = false;
      return;
    }

    if (save_result.success) {
      player_team_json["checksum"] = save_result.checksum;
    }

    player_team_json["codeHash"] = SHARED_CODE_HASH;
    log_info("SERVER_BATTLE_REQUEST: Client code hash: {}", SHARED_CODE_HASH);

    http_helpers::ServerUrlParts url_parts =
        http_helpers::parse_server_url(request.serverUrl);
    if (!url_parts.success) {
      log_error("SERVER_BATTLE_REQUEST: Failed to parse server URL: {}",
                request.serverUrl);
      request.serverRequestPending = false;
      return;
    }

    log_info("SERVER_BATTLE_REQUEST: Connecting to server at {}:{}",
             url_parts.host, url_parts.port);

    httplib::Client client(url_parts.host, url_parts.port);
    client.set_read_timeout(30, 0);
    client.set_connection_timeout(10, 0);

    std::string request_body = player_team_json.dump();
    auto res = client.Post("/battle", request_body, "application/json");

    if (!res) {
      log_error("SERVER_BATTLE_REQUEST: Failed to connect to server");
      request.serverRequestPending = false;
      return;
    }

    if (res->status != 200) {
      log_error("SERVER_BATTLE_REQUEST: Server returned error status: {}",
                res->status);
      log_error("  Response: {}", res->body);
      request.serverRequestPending = false;
      return;
    }

    nlohmann::json battle_response = nlohmann::json::parse(res->body);
    uint64_t seed = battle_response["seed"].get<uint64_t>();
    std::string opponent_id = battle_response["opponentId"].get<std::string>();
    std::string checksum = battle_response.value("checksum", std::string(""));

    log_info("SERVER_BATTLE_REQUEST: Battle request successful");
    log_info("  Seed: {}", seed);
    log_info("  Opponent ID: {}", opponent_id);
    log_info("  Checksum: {}", checksum);

    request.playerJsonPath =
        "output/battles/temp_player_" + std::to_string(seed) + ".json";
    request.opponentJsonPath =
        "output/battles/temp_opponent_" + std::to_string(seed) + ".json";

    if (!std::filesystem::exists(request.playerJsonPath)) {
      log_warn("SERVER_BATTLE_REQUEST: Player file not found, creating it...");
      std::filesystem::create_directories("output/battles");
      std::ofstream player_out(request.playerJsonPath);
      player_out << player_team_json.dump(2);
      player_out.close();
    }

    auto replay_state_opt =
        afterhours::EntityHelper::get_singleton<ReplayState>();
    afterhours::Entity &replay_entity = replay_state_opt.get();

    if (!replay_entity.has<ReplayState>()) {
      replay_entity.addComponent<ReplayState>();
    }

    ReplayState &replay = replay_entity.get<ReplayState>();
    replay.seed = seed;
    replay.playerJsonPath = request.playerJsonPath;
    replay.opponentJsonPath = request.opponentJsonPath;
    replay.serverChecksum = checksum;
    replay.active = true;
    replay.paused = false;
    replay.timeScale = 1.0f;

    afterhours::EntityHelper::registerSingleton<ReplayState>(replay_entity);

    log_info("SERVER_BATTLE_REQUEST: Server request complete");
    log_info("  Player file: {}", request.playerJsonPath);
    log_info("  Opponent file: {}", request.opponentJsonPath);

    auto save_result_after = save_system.save_game_state();
    if (save_result_after.success) {
      log_info("GAME_STATE_SAVE: Saving game state after battle response");

      nlohmann::json save_request;
      save_request["userId"] = save_result_after.gameState["userId"];
      save_request["checksum"] = save_result_after.checksum;
      save_request["gameState"] = save_result_after.gameState;
      save_request["timestamp"] = save_result_after.gameState["timestamp"];

      auto save_res = client.Post("/save-game-state", save_request.dump(),
                                  "application/json");
      if (save_res && save_res->status == 200) {
        nlohmann::json save_response = nlohmann::json::parse(save_res->body);
        bool match = save_response.value("match", false);
        if (!match && save_response.contains("gameState")) {
          log_info("GAME_STATE_SAVE: Server returned updated state, "
                   "overwriting local save");
          std::string userId =
              save_result_after.gameState["userId"].get<std::string>();
          server::FileStorage::save_json_to_file(
              server::FileStorage::get_game_state_save_path(userId),
              save_response["gameState"]);
        } else {
          log_info("GAME_STATE_SAVE: Server save successful, checksum match");
        }
      } else {
        log_error("GAME_STATE_SAVE: Server save failed, continuing "
                  "with local save only");
      }
    } else {
      log_error(
          "GAME_STATE_SAVE: Failed to save game state after battle response");
    }
  }

private:
  nlohmann::json build_player_team_json() {
    std::vector<std::reference_wrapper<afterhours::Entity>> inventory_dishes;
    for (afterhours::Entity &entity : afterhours::EntityQuery()
                                          .whereHasComponent<IsInventoryItem>()
                                          .whereHasComponent<IsDish>()
                                          .gen()) {
      inventory_dishes.push_back(entity);
    }

    if (inventory_dishes.empty()) {
      return nlohmann::json();
    }

    std::sort(inventory_dishes.begin(), inventory_dishes.end(),
              [](const std::reference_wrapper<afterhours::Entity> &a,
                 const std::reference_wrapper<afterhours::Entity> &b) {
                return a.get().get<IsInventoryItem>().slot <
                       b.get().get<IsInventoryItem>().slot;
              });

    nlohmann::json team = nlohmann::json::array();
    int slot_index = 0;
    for (const auto &entity_ref : inventory_dishes) {
      afterhours::Entity &entity = entity_ref.get();
      IsDish &dish = entity.get<IsDish>();

      nlohmann::json dish_entry;
      dish_entry["slot"] = slot_index++;
      dish_entry["dishType"] = magic_enum::enum_name(dish.type);
      dish_entry["level"] = 1;
      team.push_back(dish_entry);
    }

    nlohmann::json result;
    result["team"] = team;
    return result;
  }
};
