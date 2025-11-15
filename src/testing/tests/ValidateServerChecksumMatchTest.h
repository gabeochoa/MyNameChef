#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/replay_state.h"
#include "../../game_state_manager.h"
#include "../../utils/battle_fingerprint.h"
#include "../../utils/http_helpers.h"
#include "../test_app.h"
#include "../test_macros.h"
#include "../test_server_helpers.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <string>

TEST(validate_server_checksum_match) {
  static std::string server_checksum;

  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

  // Setup BattleLoadRequest with server URL (only once)
  test_server_helpers::server_integration_test_setup("CHECKSUM_TEST");

  app.wait_for_frames(1);

  // Navigate to battle
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);

  // Wait for server request to complete and capture checksum
  app.wait_for_frames(60);
  auto request_opt =
      afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
  app.expect_singleton_has_component<BattleLoadRequest>(request_opt,
                                                        "BattleLoadRequest");

  const BattleLoadRequest &req = request_opt.get().get<BattleLoadRequest>();
  app.expect_false(req.serverUrl.empty(), "serverUrl should be set");

  // Make HTTP request to get server checksum (only once)
  static bool checksum_fetched = false;
  app.expect_false(checksum_fetched,
                   "server checksum should only be fetched once");

  std::string url = http_helpers::get_server_url();
  http_helpers::ServerUrlParts url_parts = http_helpers::parse_server_url(url);
  app.expect_true(url_parts.success, "Server URL should parse successfully");

  // Build player team JSON
  nlohmann::json player_team_json = test_server_helpers::team_to_json();

  httplib::Client client(url_parts.host, url_parts.port);
  client.set_read_timeout(30, 0);
  client.set_connection_timeout(10, 0);

  std::string request_body = player_team_json.dump();
  auto res = client.Post("/battle", request_body, "application/json");

  app.expect_true(res != nullptr, "server HTTP request should succeed");
  app.expect_eq(res->status, 200, "server should return 200 OK");

  nlohmann::json battle_response = nlohmann::json::parse(res->body);
  app.expect_true(battle_response.contains("checksum"),
                  "server response should contain checksum");
  app.expect_true(battle_response.contains("seed"),
                  "server response should contain seed");

  server_checksum = battle_response["checksum"].get<std::string>();
  uint64_t server_seed = battle_response["seed"].get<uint64_t>();

  log_info("CHECKSUM_TEST: Server checksum: {}", server_checksum);
  log_info("CHECKSUM_TEST: Server seed: {}", server_seed);

  checksum_fetched = true;

  // Wait for battle to initialize and complete
  app.wait_for_battle_initialized(30.0f);
  app.wait_for_dishes_in_combat(1, 30.0f);

  app.wait_for_ui_exists("Skip to Results", 5.0f);

  // Wait for battle to complete naturally
  log_info("CHECKSUM_TEST: Waiting for battle to complete...");
  app.wait_for_battle_complete(60.0f);
  app.wait_for_results_screen(10.0f);

  // Compute client checksum
  app.wait_for_frames(1); // Entities will be merged by system loop
  uint64_t client_fingerprint = BattleFingerprint::compute();
  std::string client_checksum =
      test_server_helpers::format_checksum(client_fingerprint);

  log_info("CHECKSUM_TEST: Client checksum: {}", client_checksum);
  log_info("CHECKSUM_TEST: Server checksum: {}", server_checksum);

  // Compare checksums
  app.expect_eq(client_checksum, server_checksum,
                "client checksum should match server checksum");

  log_info(
      "CHECKSUM_TEST: ✅ Checksums match - client replay is deterministic");
}
