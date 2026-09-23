#pragma once

#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

TEST(validate_three_battles_flow) {
  app.delete_save_file();
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

  app.run_once("inventory1",
               [&app]() { app.create_inventory_item(DishType::Potato, 0); });
  app.wait_for_frames(2);

  log_info("TEST: Starting battle 1");
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_battle_initialized(10.0f);
  app.wait_for_battle_complete(60.0f);
  app.wait_for_results_screen(10.0f);
  app.wait_for_frames(2);
  app.run_once("results1", [&app]() { app.expect_battle_has_outcomes(); });
  app.wait_for_ui_exists("Back to Shop", 5.0f);
  app.click("Back to Shop");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

  app.run_once("inventory2", [&app]() {
    app.create_inventory_item(DishType::Potato, 0);
    app.create_inventory_item(DishType::Burger, 1);
  });
  app.wait_for_frames(2);

  log_info("TEST: Starting battle 2");
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_battle_initialized(10.0f);
  app.wait_for_battle_complete(60.0f);
  app.wait_for_results_screen(10.0f);
  app.wait_for_frames(2);
  app.run_once("results2", [&app]() { app.expect_battle_has_outcomes(); });
  app.wait_for_ui_exists("Back to Shop", 5.0f);
  app.click("Back to Shop");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

  app.run_once("inventory3", [&app]() {
    app.create_inventory_item(DishType::Potato, 0);
    app.create_inventory_item(DishType::Burger, 1);
    app.create_inventory_item(DishType::Pizza, 2);
  });
  app.wait_for_frames(2);

  log_info("TEST: Starting battle 3");
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_battle_initialized(10.0f);
  app.wait_for_battle_complete(60.0f);
  app.wait_for_results_screen(10.0f);
  app.wait_for_frames(2);
  app.run_once("results3", [&app]() {
    app.expect_screen_is(GameStateManager::Screen::Results);
    app.expect_battle_has_outcomes();
  });
  app.wait_for_ui_exists("Back to Shop", 5.0f);

  log_info("TEST: validate_three_battles_flow passed");
}
