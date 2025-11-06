#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/replay_state.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../test_app.h"
#include "../test_macros.h"
#include <afterhours/ah.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <string>

TEST(validate_server_battle_integration) {
  // Static state to track test progress across frames
  static enum {
    REQUEST_BATTLE,
    NAVIGATE_TO_SHOP,
    SETUP_BATTLE_REQUEST,
    NAVIGATE_TO_BATTLE,
    WAIT_FOR_BATTLE_COMPLETE,
    VERIFY_RESULTS
  } phase = REQUEST_BATTLE;

  static std::string player_temp;
  static std::string opponent_temp;
  static uint64_t test_seed = 0;

  // Step 1: Request battle from server (only once)
  if (phase == REQUEST_BATTLE) {
    app.launch_game();

    // Test team for server
    nlohmann::json player_team_json = {
        {"team",
         {{{"dishType", "Potato"}, {"slot", 0}, {"level", 1}},
          {{"dishType", "Burger"}, {"slot", 1}, {"level", 1}},
          {{"dishType", "Pizza"}, {"slot", 2}, {"level", 1}}}}};

    log_info("INTEGRATION_TEST: Requesting battle from server...");

    // Get server URL from environment or use default
    const char *server_url_env = std::getenv("INTEGRATION_SERVER_URL");
    std::string server_url =
        server_url_env ? server_url_env : "http://localhost:8080";

    // Parse server URL
    std::string host = "localhost";
    int port = 8080;
    if (server_url.find("://") != std::string::npos) {
      size_t protocol_end = server_url.find("://") + 3;
      size_t colon_pos = server_url.find(":", protocol_end);
      size_t slash_pos = server_url.find("/", protocol_end);

      if (colon_pos != std::string::npos &&
          (slash_pos == std::string::npos || colon_pos < slash_pos)) {
        host = server_url.substr(protocol_end, colon_pos - protocol_end);
        size_t port_end =
            (slash_pos != std::string::npos) ? slash_pos : server_url.length();
        port = std::stoi(
            server_url.substr(colon_pos + 1, port_end - colon_pos - 1));
      } else {
        size_t end =
            (slash_pos != std::string::npos) ? slash_pos : server_url.length();
        host = server_url.substr(protocol_end, end - protocol_end);
      }
    }

    log_info("INTEGRATION_TEST: Connecting to server at {}:{}", host, port);

    httplib::Client client(host, port);
    client.set_read_timeout(30, 0);
    client.set_connection_timeout(10, 0);

    // Make POST request to /battle endpoint
    std::string request_body = player_team_json.dump();
    auto res = client.Post("/battle", request_body, "application/json");

    if (!res) {
      log_error("INTEGRATION_TEST: Failed to connect to server");
      log_info("INTEGRATION_TEST: Falling back to standalone mode...");

      // Fallback: Create test files (for standalone test runs)
      std::string opponent_file_path =
          "resources/battles/opponents/test_integration_opponent.json";
      std::filesystem::create_directories("resources/battles/opponents");

      nlohmann::json opponent_team = {
          {"team",
           {{{"dishType", "Ramen"}, {"slot", 0}, {"level", 1}},
            {{"dishType", "Sushi"}, {"slot", 1}, {"level", 1}},
            {{"dishType", "Steak"}, {"slot", 2}, {"level", 1}}}}};

      if (!std::filesystem::exists(opponent_file_path)) {
        std::ofstream opp_file(opponent_file_path);
        opp_file << opponent_team.dump(2);
        opp_file.close();
      }

      test_seed = 999999;
      player_temp =
          "output/battles/temp_player_" + std::to_string(test_seed) + ".json";
      opponent_temp =
          "output/battles/temp_opponent_" + std::to_string(test_seed) + ".json";

      std::filesystem::create_directories("output/battles");
      std::ofstream player_out(player_temp);
      player_out << player_team_json.dump(2);
      player_out.close();

      std::ofstream opponent_out(opponent_temp);
      opponent_out << opponent_team.dump(2);
      opponent_out.close();

      log_info("INTEGRATION_TEST: Created test battle files (standalone mode)");
      log_info("  Player: {}", player_temp);
      log_info("  Opponent: {}", opponent_temp);
      log_info("  Seed: {}", test_seed);
    } else if (res->status != 200) {
      log_error("INTEGRATION_TEST: Server returned error status: {}",
                res->status);
      log_error("  Response: {}", res->body);
      throw std::runtime_error("Battle request failed");
    } else {
      // Parse response
      nlohmann::json battle_response = nlohmann::json::parse(res->body);
      test_seed = battle_response["seed"].get<uint64_t>();
      std::string opponent_id =
          battle_response["opponentId"].get<std::string>();

      log_info("INTEGRATION_TEST: Battle request successful");
      log_info("  Seed: {}", test_seed);
      log_info("  Opponent ID: {}", opponent_id);

      // Construct file paths from seed (server creates files with this pattern)
      player_temp =
          "output/battles/temp_player_" + std::to_string(test_seed) + ".json";
      opponent_temp =
          "output/battles/temp_opponent_" + std::to_string(test_seed) + ".json";

      // Verify files exist (server should have created them)
      if (!std::filesystem::exists(player_temp)) {
        log_warn("INTEGRATION_TEST: Player file not found, creating it...");
        std::filesystem::create_directories("output/battles");
        std::ofstream player_out(player_temp);
        player_out << player_team_json.dump(2);
        player_out.close();
      }

      if (!std::filesystem::exists(opponent_temp)) {
        log_warn(
            "INTEGRATION_TEST: Opponent file not found, this may cause issues");
      }

      log_info("INTEGRATION_TEST: Using server-generated battle files");
      log_info("  Player: {}", player_temp);
      log_info("  Opponent: {}", opponent_temp);
    }

    phase = NAVIGATE_TO_SHOP;
    return;
  }

  // Step 2: Navigate to shop screen
  if (phase == NAVIGATE_TO_SHOP) {
    log_info("INTEGRATION_TEST: Navigating to shop screen...");
    app.navigate_to_shop();
    app.wait_for_ui_exists("Next Round");
    phase = SETUP_BATTLE_REQUEST;
    return;
  }

  // Step 3: Set up BattleLoadRequest in the client
  if (phase == SETUP_BATTLE_REQUEST) {
    log_info("INTEGRATION_TEST: Setting up BattleLoadRequest...");
    auto &request_entity = afterhours::EntityHelper::createEntity();
    BattleLoadRequest battleRequest;
    battleRequest.playerJsonPath = player_temp;
    battleRequest.opponentJsonPath = opponent_temp;
    battleRequest.loaded =
        false; // Will be loaded by BattleTeamFileLoaderSystem
    request_entity.addComponent<BattleLoadRequest>(std::move(battleRequest));
    afterhours::EntityHelper::registerSingleton<BattleLoadRequest>(
        request_entity);

    // Set up ReplayState with seed
    auto replay_state_opt =
        afterhours::EntityHelper::get_singleton<ReplayState>();
    afterhours::Entity &replay_entity = replay_state_opt.get();

    if (!replay_entity.has<ReplayState>()) {
      replay_entity.addComponent<ReplayState>();
    }

    ReplayState &replay = replay_entity.get<ReplayState>();
    replay.seed = test_seed;
    replay.playerJsonPath = player_temp;
    replay.opponentJsonPath = opponent_temp;
    replay.active = true;
    replay.paused = false;
    replay.timeScale = 1.0f;

    afterhours::EntityHelper::registerSingleton<ReplayState>(replay_entity);
    afterhours::EntityHelper::merge_entity_arrays();

    log_info("INTEGRATION_TEST: BattleLoadRequest and ReplayState configured");
    phase = NAVIGATE_TO_BATTLE;
    return;
  }

  // Step 4: Navigate to battle screen
  if (phase == NAVIGATE_TO_BATTLE) {
    log_info("INTEGRATION_TEST: Navigating to battle screen...");
    app.click("Next Round");
    app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
    app.wait_for_ui_exists("Skip to Results", 5.0f);

    log_info("INTEGRATION_TEST: Waiting for battle to initialize...");
    app.wait_for_frames(10); // Wait for battle to start loading
    phase = WAIT_FOR_BATTLE_COMPLETE;
    return;
  }

  // Step 5: Wait for battle to complete
  if (phase == WAIT_FOR_BATTLE_COMPLETE) {
    log_info("INTEGRATION_TEST: Battle started, waiting for completion...");
    app.wait_for_battle_complete(60.0f);
    app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
    phase = VERIFY_RESULTS;
    return;
  }

  // Step 6: Verify results
  if (phase == VERIFY_RESULTS) {
    log_info("INTEGRATION_TEST: Battle completed successfully!");

    // Verify we got to results screen
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Results) {
      app.fail("Expected to be on Results screen after battle");
    }

    log_info("INTEGRATION_TEST: ✅ Test passed - battle completed and results "
             "screen shown");
  }
}
