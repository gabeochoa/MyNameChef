#pragma once

#include "../../components/user_id.h"
#include "../../game_state_manager.h"
#include "../../server/file_storage.h"
#include "../../shop.h"
#include "../test_macros.h"
#include "../UITestHelpers.h"
#include <afterhours/ah.h>
#include <filesystem>

// Test New Team vs Continue button behavior
// Verifies that New Team deletes save and starts fresh, while Continue loads state
TEST(validate_game_state_new_team_vs_continue) {
  app.launch_game();
  // Delete any existing save file to ensure we start fresh
  auto userId_opt = afterhours::EntityHelper::get_singleton<UserId>();
  if (userId_opt.get().has<UserId>()) {
    std::string userId = userId_opt.get().get<UserId>().userId;
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

  // Set up state to save
  app.set_wallet_gold(150);
  const int saved_gold = app.read_wallet_gold();
  const int saved_round = app.read_round();

  // Create save file
  app.trigger_game_state_save();
  app.wait_for_frames(5);
  app.expect_true(app.save_file_exists(), "save file should exist");

  // Navigate back to main menu
  GameStateManager::get().set_next_screen(GameStateManager::Screen::Main);
  app.wait_for_screen(GameStateManager::Screen::Main, 5.0f);
  app.wait_for_frames(10);

  // Verify "New Team" and "Continue" buttons exist
  app.wait_for_ui_exists("New Team", 5.0f);
  app.wait_for_ui_exists("Continue", 5.0f);

  // Test "New Team" button - should delete save and start fresh
  app.click("New Team");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_frames(10);

  // Verify save file is deleted
  app.expect_false(app.save_file_exists(),
                   "save file should be deleted after New Team");

  // Verify fresh game state (default values)
  app.expect_eq(app.read_round(), 1, "round should be reset to 1");
  // Gold might be default or from fresh game start
  const int fresh_gold = app.read_wallet_gold();
  app.expect_true(fresh_gold != saved_gold || fresh_gold == 0,
                  "gold should be different from saved state or default");

  // Create save again for Continue test
  app.set_wallet_gold(200);
  const int new_saved_gold = app.read_wallet_gold();
  app.trigger_game_state_save();
  app.wait_for_frames(5);
  app.expect_true(app.save_file_exists(), "save file should exist again");

  // Navigate back to main menu
  GameStateManager::get().set_next_screen(GameStateManager::Screen::Main);
  app.wait_for_screen(GameStateManager::Screen::Main, 5.0f);
  app.wait_for_frames(10);

  // Clear current state to simulate fresh start
  app.set_wallet_gold(0);
  auto round_opt = afterhours::EntityHelper::get_singleton<Round>();
  if (round_opt.get().has<Round>()) {
    round_opt.get().get<Round>().current = 1;
  }

  // Test "Continue" button - should load saved state
  app.click("Continue");
  app.wait_for_frames(20); // Give load system time to process

  // Verify state is restored
  app.expect_wallet_has(new_saved_gold, "gold should be restored from save");
  app.expect_eq(app.read_round(), saved_round,
                "round should be restored from save");
}

