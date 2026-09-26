#pragma once

#include "../components/battle_load_request.h"
#include "../components/dish_level.h"
#include "../components/drink_pairing.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../components/replay_state.h"
#include "../components/user_id.h"
#include "../game_state_manager.h"
#include "../shop.h"
#include "../log.h"
#include "../server/file_storage.h"
#include "../systems/GameStateSaveSystem.h"
#include "../utils/code_hash_generated.h"
#include "../utils/http_helpers.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <fstream>
#include <future>
#include <httplib.h>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <vector>

struct ServerBattleRequestSystem : afterhours::System<BattleLoadRequest> {
  int failures = 0; // issue 25: bounded retries
  // Async HTTP (settled): battle POST runs on a worker thread; the game loop
  // polls and processes the response on the main thread.
  std::future<std::pair<int, std::string>> battle_future;
  bool in_flight = false;
  float retry_delay = 0.0f; // Backoff between attempts (server restarts)
  nlohmann::json pending_team;
  http_helpers::ServerUrlParts pending_url;
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &, BattleLoadRequest &request,
                     float dt) override {
    if (request.serverUrl.empty()) {
      return;
    }

    if (retry_delay > 0.0f) {
      retry_delay -= dt;
      return;
    }

    if (in_flight) {
      if (battle_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
      auto result = battle_future.get(); in_flight = false;
      process_battle_response(request, result.first, result.second);
      return;
    }

    if (request.serverRequestPending) {
      return;
    }

    if (!request.playerJsonPath.empty() || !request.opponentJsonPath.empty()) {
      return;
    }
    if (failures >= 3) { log_warn("SERVER_BATTLE_REQUEST: terminal failure after 3 attempts - set a new serverUrl/request to retry"); return; } // issue 25

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
      log_warn("SERVER_BATTLE_REQUEST: Failed to build player team JSON"); failures++; retry_delay = 1.0f;
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
    if (!url_parts.success || url_parts.is_https) { // issue 28: no TLS client - fail closed, never downgrade
      log_warn("SERVER_BATTLE_REQUEST: Invalid or HTTPS (unsupported) server URL: {}", request.serverUrl); failures++; retry_delay = 1.0f;
      request.serverRequestPending = false;
      return;
    }

    // Pool payload (settled): round + user + version for matching
    player_team_json["round"] = [] { auto e = afterhours::EntityHelper::get_singleton<Round>(); return e.get().has<Round>() ? e.get().get<Round>().current : 1; }();
    player_team_json["userId"] = [] { auto e = afterhours::EntityHelper::get_singleton<UserId>(); return e.get().has<UserId>() ? e.get().get<UserId>().userId : std::string(""); }();
    player_team_json["clientVersion"] = GAME_STATE_CLIENT_VERSION;

    pending_team = player_team_json; pending_url = url_parts;
    std::string request_body = player_team_json.dump();
    std::string host = url_parts.host; int port = url_parts.port;
    battle_future = std::async(std::launch::async, [host, port, request_body]() -> std::pair<int, std::string> {
      httplib::Client client(host, port); client.set_read_timeout(30, 0); client.set_connection_timeout(10, 0);
      auto res = client.Post("/battle", request_body, "application/json");
      if (!res) return {-1, ""};
      return {res->status, res->body};
    });
    in_flight = true;
    log_info("SERVER_BATTLE_REQUEST: Async battle request launched to {}:{}", host, port);
    return;
  }

  void process_battle_response(BattleLoadRequest &request, int status, const std::string &body) {
    BattleLoadRequest &req_ref = request;
    (void)req_ref;
    nlohmann::json player_team_json = pending_team;
    http_helpers::ServerUrlParts url_parts = pending_url;
    GameStateSaveSystem save_system;
    if (status < 0) { log_warn("SERVER_BATTLE_REQUEST: Failed to connect"); failures++; retry_delay = 1.0f; request.serverRequestPending = false; return; }
    if (status != 200) { log_warn("SERVER_BATTLE_REQUEST: Server status {}: {}", status, body); failures++; retry_delay = 1.0f; request.serverRequestPending = false; return; }
    nlohmann::json battle_response; uint64_t seed; std::string opponent_id, checksum;
    try {
      battle_response = nlohmann::json::parse(body);
      if (!battle_response.contains("seed") || !battle_response.contains("opponentId")) throw std::runtime_error("missing seed/opponentId");
      seed = battle_response["seed"].get<uint64_t>(); opponent_id = battle_response["opponentId"].get<std::string>(); checksum = battle_response.value("checksum", std::string(""));
    } catch (const std::exception &e) { log_warn("SERVER_BATTLE_REQUEST: Malformed response: {}", e.what()); failures++; retry_delay = 1.0f; request.serverRequestPending = false; return; }
    failures = 0;
    httplib::Client client(url_parts.host, url_parts.port); client.set_read_timeout(30, 0); client.set_connection_timeout(10, 0);

    log_info("SERVER_BATTLE_REQUEST: Battle request successful");
    log_info("  Seed: {}", seed);
    log_info("  Opponent ID: {}", opponent_id);
    log_info("  Checksum: {}", checksum);

    request.playerJsonPath =
        "output/battles/temp_player_" + std::to_string(seed) + ".json";
    request.opponentJsonPath =
        "output/battles/temp_opponent_" + std::to_string(seed) + ".json";

    if (!std::filesystem::exists(request.playerJsonPath)) {
      std::filesystem::create_directories("output/battles");
      std::ofstream player_out(request.playerJsonPath); player_out << player_team_json.dump(2); player_out.close();
    }
    // Issue 10: write opponent snapshot returned by server (no server temp file).
    if (battle_response.contains("opponentTeam")) {
      std::filesystem::create_directories("output/battles");
      std::ofstream opp_out(request.opponentJsonPath);
      auto ot = battle_response["opponentTeam"]; if (ot.is_array()) ot = nlohmann::json{{"team", ot}};
      opp_out << ot.dump(2); opp_out.close();
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
    replay.from_history = false;
    replay.paused = false;
    replay.timeScale = 1.0f;

    if (!afterhours::EntityHelper::has_singleton<ReplayState>())
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
        nlohmann::json save_response; try { save_response = nlohmann::json::parse(save_res->body); } catch (...) { log_warn("GAME_STATE_SAVE: malformed save response, keeping local"); save_response = {}; }
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
      dish_entry["level"] = entity.has<DishLevel>() ? entity.get<DishLevel>().level : 1; // issue 7
      if (entity.has<DrinkPairing>() && entity.get<DrinkPairing>().drink) dish_entry["drink"] = magic_enum::enum_name(*entity.get<DrinkPairing>().drink); // issue 8
      team.push_back(dish_entry);
    }

    nlohmann::json result;
    result["team"] = team;
    return result;
  }
};
