#pragma once

#include "../../components/battle_result.h"
#include "../../components/dish_battle_state.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

TEST(validate_full_game_flow) {
  log_info("TEST: Starting validate_full_game_flow test");

  // Step 1: Start from main menu
  app.launch_game();
  app.wait_for_ui_exists("Play");

  // Step 2: Navigate to shop
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);

  // Step 3: Validate shop functionality
  app.wait_for_ui_exists("Next Round");
  app.wait_for_ui_exists("Reroll (1)");

  // Step 3.5: Create dishes if inventory is empty
  const auto inventory = app.read_player_inventory();
  if (inventory.empty()) {
    app.create_inventory_item(DishType::Potato, 0);
    app.wait_for_frames(2);
  }

  // Step 4: Navigate to battle
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  // With timing speed scale, battle might complete very quickly, so check
  // current screen
  GameStateManager::get().update_screen();
  GameStateManager::Screen current_screen =
      GameStateManager::get().active_screen;
  if (current_screen != GameStateManager::Screen::Battle &&
      current_screen != GameStateManager::Screen::Results) {
    app.expect_screen_is(GameStateManager::Screen::Battle);
  }

  app.wait_for_frames(10);

  app.wait_for_battle_initialized(10.0f);
  log_info("TEST: Battle initialized");

  // Check screen - might be Battle or Results if battle completed very quickly
  GameStateManager::get().update_screen();
  GameStateManager::Screen screen_after_init =
      GameStateManager::get().active_screen;
  if (screen_after_init == GameStateManager::Screen::Results) {
    // Battle completed very quickly, skip to results validation
    log_info("TEST: Battle completed very quickly, already on Results screen");
  } else {
    app.expect_screen_is(GameStateManager::Screen::Battle);
  }

  int initial_player_dishes = 0;
  int initial_opponent_dishes = 0;
  // Only count dishes if we're still on Battle screen
  GameStateManager::get().update_screen();
  if (GameStateManager::get().active_screen ==
      GameStateManager::Screen::Battle) {
    initial_player_dishes = app.count_active_player_dishes();
    initial_opponent_dishes = app.count_active_opponent_dishes();
    log_info("TEST: Initial dish counts - Player: {}, Opponent: {}",
             initial_player_dishes, initial_opponent_dishes);
  } else {
    // Battle already completed - skip dish count validation since we can't get
    // accurate initial counts
    log_info("TEST: Battle already completed, skipping dish count validation");
    initial_player_dishes = -1; // Use -1 as sentinel to skip validation
    initial_opponent_dishes = -1;
  }

  // Step 5: Wait for battle to complete naturally or skip to results
  GameStateManager::get().update_screen();
  if (GameStateManager::get().active_screen !=
      GameStateManager::Screen::Results) {
    log_info("TEST: Waiting for Skip to Results button...");
    app.wait_for_ui_exists("Skip to Results", 10.0f);
    log_info("TEST: Clicking Skip to Results");
    app.click("Skip to Results");
    log_info("TEST: Waiting for Results screen");
    app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
    log_info("TEST: Battle completed (skipped to results)");
  } else {
    log_info("TEST: Battle already completed, skipping wait");
  }

  // Step 6: Validate battle completion and results
  app.wait_for_results_screen(10.0f);
  app.expect_screen_is(GameStateManager::Screen::Results);
  log_info("TEST: Results screen reached");

  int final_player = app.count_active_player_dishes();
  int final_opponent = app.count_active_opponent_dishes();
  log_info("TEST: Final dish counts - Player: {}, Opponent: {}", final_player,
           final_opponent);

  // Only validate dish counts if we got initial counts (battle was still in
  // progress)
  if (initial_player_dishes >= 0 && initial_opponent_dishes >= 0) {
    app.expect_true(initial_player_dishes > 0 || initial_opponent_dishes > 0,
                    "should have initial dishes");
    app.expect_count_lte(final_player, initial_player_dishes,
                         "final player dish count");
    app.expect_count_lte(final_opponent, initial_opponent_dishes,
                         "final opponent dish count");
  } else {
    log_info("TEST: Skipping dish count validation (battle already completed "
             "when test started)");
  }

  // Wait a moment for BattleResult to be created if battle just completed
  app.wait_for_frames(10);
  auto result_entity = afterhours::EntityHelper::get_singleton<BattleResult>();
  // Check if entity is valid before accessing it
  if (result_entity.get().id == 0) {
    log_warn("TEST: BattleResult not found yet, waiting more...");
    // Try waiting a bit more
    app.wait_for_frames(20);
    result_entity = afterhours::EntityHelper::get_singleton<BattleResult>();
  }
  // Now check if it has the component (this will fail gracefully if entity is
  // invalid)
  app.expect_singleton_has_component<BattleResult>(result_entity,
                                                   "BattleResult");
  app.expect_battle_not_tie();
  app.expect_battle_has_outcomes();
  log_info("TEST: Battle result validated");
}
