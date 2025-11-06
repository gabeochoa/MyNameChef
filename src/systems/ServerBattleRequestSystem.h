#pragma once

#include "../components/battle_load_request.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../components/replay_state.h"
#include "../game_state_manager.h"
#include "../log.h"
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

    nlohmann::json player_team_json = build_player_team_json();
    if (player_team_json.empty()) {
      log_error("SERVER_BATTLE_REQUEST: Failed to build player team JSON");
      request.serverRequestPending = false;
      return;
    }

    std::string host;
    int port = 8080;

    if (!parse_server_url(request.serverUrl, host, port)) {
      log_error("SERVER_BATTLE_REQUEST: Failed to parse server URL: {}",
                request.serverUrl);
      request.serverRequestPending = false;
      return;
    }

    log_info("SERVER_BATTLE_REQUEST: Connecting to server at {}:{}", host,
             port);

    httplib::Client client(host, port);
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

    log_info("SERVER_BATTLE_REQUEST: Battle request successful");
    log_info("  Seed: {}", seed);
    log_info("  Opponent ID: {}", opponent_id);

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
    replay.active = true;
    replay.paused = false;
    replay.timeScale = 1.0f;

    afterhours::EntityHelper::registerSingleton<ReplayState>(replay_entity);
    afterhours::EntityHelper::merge_entity_arrays();

    log_info("SERVER_BATTLE_REQUEST: Server request complete");
    log_info("  Player file: {}", request.playerJsonPath);
    log_info("  Opponent file: {}", request.opponentJsonPath);
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
    for (const auto &entity_ref : inventory_dishes) {
      afterhours::Entity &entity = entity_ref.get();
      IsInventoryItem &inventory_item = entity.get<IsInventoryItem>();
      IsDish &dish = entity.get<IsDish>();

      nlohmann::json dish_entry;
      dish_entry["slot"] = inventory_item.slot - 100;
      dish_entry["dishType"] = magic_enum::enum_name(dish.type);
      dish_entry["level"] = 1;
      team.push_back(dish_entry);
    }

    nlohmann::json result;
    result["team"] = team;
    return result;
  }

  bool parse_server_url(const std::string &server_url, std::string &host,
                        int &port) {
    if (server_url.find("://") == std::string::npos) {
      host = server_url;
      return true;
    }

    size_t protocol_end = server_url.find("://") + 3;
    size_t colon_pos = server_url.find(":", protocol_end);
    size_t slash_pos = server_url.find("/", protocol_end);

    if (colon_pos != std::string::npos &&
        (slash_pos == std::string::npos || colon_pos < slash_pos)) {
      host = server_url.substr(protocol_end, colon_pos - protocol_end);
      size_t port_end =
          (slash_pos != std::string::npos) ? slash_pos : server_url.length();
      try {
        port = std::stoi(
            server_url.substr(colon_pos + 1, port_end - colon_pos - 1));
      } catch (...) {
        return false;
      }
    } else {
      size_t end =
          (slash_pos != std::string::npos) ? slash_pos : server_url.length();
      host = server_url.substr(protocol_end, end - protocol_end);
    }

    return true;
  }
};
