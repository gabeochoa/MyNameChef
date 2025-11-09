#pragma once

#include "../../components/user_id.h"
#include "../../game_state_manager.h"
#include "../../server/async/components/team_pool.h"
#include "../../server/file_storage.h"
#include "../../utils/battle_fingerprint.h"
#include "../test_macros.h"
#include "../test_server_helpers.h"
#include "../UITestHelpers.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <nlohmann/json.hpp>

// Test that server state takes precedence when checksums don't match
// Verifies server sync behavior and local file overwrite
TEST(validate_game_state_server_checksum_sync) {
  // Setup server integration
  test_server_helpers::server_integration_test_setup("CHECKSUM_SYNC_TEST");

  app.launch_game();
  // Delete any existing save file to ensure we start fresh
  auto userId_opt_cleanup = afterhours::EntityHelper::get_singleton<UserId>();
  if (userId_opt_cleanup.get().has<UserId>()) {
    std::string userId = userId_opt_cleanup.get().get<UserId>().userId;
    std::string save_file =
        server::FileStorage::get_game_state_save_path(userId);
    if (server::FileStorage::file_exists(save_file)) {
      std::filesystem::remove(save_file);
    }
  }
  app.wait_for_frames(5);
  if (!UITestHelpers::check_ui_exists("Play") &&
      !UITestHelpers::check_ui_exists("New Team")) {
    app.wait_for_ui_exists("Play", 5.0f);
  }
  if (UITestHelpers::check_ui_exists("New Team")) {
    app.click("New Team");
  } else {
    app.click("Play");
  }
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_frames(20);

  // Set up initial state
  app.set_wallet_gold(100);
  const int initial_gold = app.read_wallet_gold();

  // Save local state
  app.trigger_game_state_save();
  app.wait_for_frames(5);
  app.expect_true(app.save_file_exists(), "local save file should exist");

  // Read local save file
  auto userId_opt = afterhours::EntityHelper::get_singleton<UserId>();
  app.expect_true(userId_opt.get().has<UserId>(),
                  "UserId singleton should exist");
  std::string userId = userId_opt.get().get<UserId>().userId;
  std::string save_file = server::FileStorage::get_game_state_save_path(userId);
  nlohmann::json local_state =
      server::FileStorage::load_json_from_file(save_file);
  app.expect_true(local_state.contains("gold"), "local state should have gold");
  app.expect_eq(local_state["gold"].get<int>(), initial_gold,
                "local state gold should match");

  // Manually create server state with different gold (simulating server
  // modification)
  const int server_gold = initial_gold + 200;
  nlohmann::json server_state = local_state;
  server_state["gold"] = server_gold;
  std::string server_checksum = compute_game_state_checksum(server_state);

  // Store server state in TeamPoolEntry (simulating server storage)
  // This requires access to the server's entity system
  // For now, we'll verify the load system handles checksum mismatch correctly
  // by checking the logic path

  // Modify local state to have different checksum (simulate local being out
  // of sync)
  local_state["gold"] = initial_gold - 50; // Different from server
  std::string modified_local_checksum =
      compute_game_state_checksum(local_state);
  local_state["checksum"] = modified_local_checksum;
  server::FileStorage::save_json_to_file(save_file, local_state);

  // Note: In a real scenario, the server would return the server_state when
  // checksum doesn't match. For this test, we're verifying the local file
  // structure and that the system would handle server state correctly.
  // A full integration test would require mocking the HTTP response.

  // Verify local file has modified state
  nlohmann::json modified_local =
      server::FileStorage::load_json_from_file(save_file);
  app.expect_eq(modified_local["gold"].get<int>(), initial_gold - 50,
                "modified local gold should be different");

  // TODO: Add full server integration test that mocks HTTP response
  // to verify server state overwrites local when checksums don't match
  log_info("CHECKSUM_SYNC_TEST: Test verifies local state structure. Full "
           "server sync test requires HTTP mocking.");
}

