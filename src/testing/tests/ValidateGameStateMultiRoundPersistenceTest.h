#pragma once

#include "../../components/is_dish.h"
#include "../../components/is_inventory_item.h"
#include "../../components/user_id.h"
#include "../../game_state_manager.h"
#include "../../query.h"
#include "../../server/file_storage.h"
#include "../test_macros.h"
#include "../UITestHelpers.h"
#include <afterhours/ah.h>
#include <filesystem>

// Test that game state persists correctly across multiple rounds
// Verifies accumulated state (inventory, gold, health, round, shop tier) after multiple rounds
TEST(validate_game_state_multi_round_persistence) {
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

  // Round 1: Initial state
  const int round1_gold = app.read_wallet_gold();
  const int round1_health = app.read_player_health();
  const int round1_shop_tier = app.read_shop_tier();
  auto round1_inventory = app.read_player_inventory();
  const int round1_inventory_count = static_cast<int>(round1_inventory.size());

  // Purchase an item in round 1
  const auto shop_items_round1 = app.read_store_options();
  if (!shop_items_round1.empty() && app.can_afford_purchase(shop_items_round1[0].type)) {
    app.purchase_item(shop_items_round1[0].type);
    app.wait_for_frames(5);
  }

  // Go to battle (triggers save)
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_frames(10);

  // Wait for battle to complete and return to shop
  app.wait_for_battle_complete(30.0f);
  app.wait_for_results_screen(10.0f);
  app.wait_for_ui_exists("Back to Shop", 5.0f);
  app.click("Back to Shop");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_frames(20);

  // Round 2: State should have changed
  const int round2_gold = app.read_wallet_gold();
  const int round2_health = app.read_player_health();
  const int round2_round = app.read_round();
  const int round2_shop_tier = app.read_shop_tier();
  auto round2_inventory = app.read_player_inventory();
  const int round2_inventory_count = static_cast<int>(round2_inventory.size());

  // Verify round incremented
  app.expect_count_gt(round2_round, 1, "round should have incremented");

  // Purchase another item in round 2
  const auto shop_items_round2 = app.read_store_options();
  if (!shop_items_round2.empty() && app.can_afford_purchase(shop_items_round2[0].type)) {
    app.purchase_item(shop_items_round2[0].type);
    app.wait_for_frames(5);
  }

  // Modify health to simulate battle damage
  auto health_opt = afterhours::EntityHelper::get_singleton<Health>();
  if (health_opt.get().has<Health>()) {
    health_opt.get().get<Health>().current = round2_health - 1;
  }

  // Go to battle again (triggers save)
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_frames(10);

  // Wait for battle to complete
  app.wait_for_battle_complete(30.0f);
  app.wait_for_results_screen(10.0f);
  app.wait_for_ui_exists("Back to Shop", 5.0f);
  app.click("Back to Shop");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_frames(20);

  // Round 3: Capture final state before save
  const int round3_gold = app.read_wallet_gold();
  const int round3_health = app.read_player_health();
  const int round3_round = app.read_round();
  const int round3_shop_tier = app.read_shop_tier();
  auto round3_inventory = app.read_player_inventory();
  const int round3_inventory_count = static_cast<int>(round3_inventory.size());

  // Manually trigger save to ensure latest state is saved
  app.trigger_game_state_save();
  app.wait_for_frames(5);
  app.expect_true(app.save_file_exists(), "save file should exist");

  // Clear state to simulate fresh start
  // Clear inventory
  for (afterhours::Entity &entity : afterhours::EntityQuery()
                                         .whereHasComponent<IsInventoryItem>()
                                         .whereHasComponent<IsDish>()
                                         .gen()) {
    entity.cleanup = true;
  }
  afterhours::EntityHelper::cleanup();
  app.wait_for_frames(1); // Entities will be merged by system loop
  app.wait_for_frames(5);

  // Reset gold, health, round
  app.set_wallet_gold(0);
  auto health_reset_opt = afterhours::EntityHelper::get_singleton<Health>();
  if (health_reset_opt.get().has<Health>()) {
    health_reset_opt.get().get<Health>().current = 5;
    health_reset_opt.get().get<Health>().max = 5;
  }
  auto round_reset_opt = afterhours::EntityHelper::get_singleton<Round>();
  if (round_reset_opt.get().has<Round>()) {
    round_reset_opt.get().get<Round>().current = 1;
  }

  // Trigger load
  app.trigger_game_state_load();
  app.wait_for_frames(10);

  // Verify all accumulated state is restored
  app.expect_wallet_has(round3_gold, "gold should match round 3 state");
  app.expect_eq(app.read_player_health(), round3_health,
                "health should match round 3 state");
  app.expect_eq(app.read_round(), round3_round,
                "round should match round 3 state");
  app.expect_eq(app.read_shop_tier(), round3_shop_tier,
                "shop tier should match round 3 state");

  // Verify inventory count matches
  const auto restored_inventory = app.read_player_inventory();
  app.expect_count_eq(static_cast<int>(restored_inventory.size()),
                      round3_inventory_count,
                      "inventory count should match round 3 state");
}

