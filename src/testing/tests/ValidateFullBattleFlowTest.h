#pragma once

#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

TEST(validate_full_battle_flow) {
  app.delete_save_file();
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

  app.run_once("inventory", [&app]() {
    app.create_inventory_item(DishType::Potato, 0);
    app.create_inventory_item(DishType::Burger, 1);
    app.create_inventory_item(DishType::Pizza, 2);
  });
  app.wait_for_frames(2);

  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_battle_initialized(10.0f);

  app.run_once("dishes_loaded", [&app]() {
    app.expect_count_gte(app.count_active_player_dishes(), 1,
                         "player dishes at battle start");
    app.expect_count_gte(app.count_active_opponent_dishes(), 1,
                         "opponent dishes at battle start");
  });

  app.wait_for_battle_complete(60.0f);
  app.wait_for_results_screen(10.0f);
  app.wait_for_frames(2);

  app.run_once("results", [&app]() {
    app.expect_screen_is(GameStateManager::Screen::Results);
    app.expect_battle_has_outcomes();
  });

  log_info("TEST: validate_full_battle_flow passed");
}
