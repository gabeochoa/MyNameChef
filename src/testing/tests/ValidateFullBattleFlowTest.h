#pragma once

#include "../../components/animation_event.h"
#include "../../components/battle_result.h"
#include "../../components/dish_battle_state.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

// Test that runs through a complete battle flow
// Prerequisites for battle to start:
// 1. SlideIn animation must complete (0.27s timer)
// 2. SimplifiedOnServeSystem must fire OnServe for all dishes
// 3. StartCourseSystem can then start the first course
// Validates:
// - Battle starts and loads dishes from JSON
// - Battle progresses through all 7 courses
// - Battle eventually transitions to results screen
// - Dishes are served and combat occurs
TEST(validate_full_battle_flow) {
  log_info("TEST: Starting validate_full_battle_flow test");

  app.launch_game();
  log_info("TEST: Game launched");

  GameStateManager::Screen current_screen = app.read_current_screen();
  if (current_screen != GameStateManager::Screen::Shop) {
    app.wait_for_ui_exists("Play");
    app.click("Play");
    app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  }
  app.expect_screen_is(GameStateManager::Screen::Shop);

  app.wait_for_frames(5);

  const auto inventory = app.read_player_inventory();
  if (inventory.empty()) {
    app.create_inventory_item(DishType::Potato, 0);
    app.wait_for_frames(2);
  }

  app.wait_for_ui_exists("Next Round");

  GameStateManager::Screen battle_screen = app.read_current_screen();
  if (battle_screen != GameStateManager::Screen::Battle) {
    app.click("Next Round");
    app.wait_for_frames(3);
    app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  }
  app.expect_screen_is(GameStateManager::Screen::Battle);

  app.wait_for_frames(10);

  app.wait_for_battle_initialized(10.0f);

  app.expect_screen_is(GameStateManager::Screen::Battle);

  int initial_player_dishes = app.count_active_player_dishes();
  int initial_opponent_dishes = app.count_active_opponent_dishes();

  app.wait_for_battle_complete(60.0f);
  
  // Verify combat ticks occurred
  app.expect_combat_ticks_occurred(1);
  
  app.wait_for_results_screen(10.0f);
  app.expect_screen_is(GameStateManager::Screen::Results);

  if (initial_player_dishes > 0 || initial_opponent_dishes > 0) {
    int final_player = app.count_active_player_dishes();
    int final_opponent = app.count_active_opponent_dishes();
    app.expect_count_lte(final_player, initial_player_dishes, "final player dish count");
    app.expect_count_lte(final_opponent, initial_opponent_dishes, "final opponent dish count");
  }

  auto result_entity = afterhours::EntityHelper::get_singleton<BattleResult>();
  if (result_entity.get().has<BattleResult>()) {
    app.expect_battle_not_tie();
    app.expect_battle_has_outcomes();
  }
}
