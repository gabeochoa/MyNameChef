#pragma once

#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

TEST(validate_three_battles_flow) {
  log_info("TEST: Starting validate_three_battles_flow test");

  log_info("TEST: Step 1 - Launching game");
  app.launch_game();
  log_info("TEST: Step 1.5 - Waiting for Main screen");
  app.wait_for_screen(GameStateManager::Screen::Main, 5.0f);
  log_info("TEST: Step 2 - Waiting for Play button");
  app.wait_for_ui_exists("Play");
  log_info("TEST: Step 3 - Clicking Play button");
  app.click("Play");
  log_info("TEST: Step 4 - Waiting for Shop screen");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  log_info("TEST: Step 5 - Verifying Shop screen");
  app.expect_screen_is(GameStateManager::Screen::Shop);
  app.wait_for_frames(5);

  log_info("TEST: Step 6 - Creating inventory item");
  app.create_inventory_item(DishType::Potato, 0);
  app.wait_for_frames(2);

  log_info("TEST: Starting battle 1");
  log_info("TEST: Battle 1 - Waiting for Next Round button");
  app.wait_for_ui_exists("Next Round");
  log_info("TEST: Battle 1 - Clicking Next Round");
  app.click("Next Round");
  app.wait_for_frames(3);
  log_info("TEST: Battle 1 - Waiting for Battle screen");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_frames(10);
  log_info("TEST: Battle 1 - Waiting for battle initialization");
  app.wait_for_battle_initialized(10.0f);
  app.expect_screen_is(GameStateManager::Screen::Battle);

  // Verify combat ticks occurred for Battle 1
  app.expect_combat_ticks_occurred(1);

  log_info("TEST: Battle 1 - Waiting for Skip to Results");
  app.wait_for_ui_exists("Skip to Results", 5.0f);
  log_info("TEST: Battle 1 - Clicking Skip to Results");
  app.click("Skip to Results");
  log_info("TEST: Battle 1 - Waiting for Results screen");
  app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
  log_info("TEST: Battle 1 - Waiting for Back to Shop");
  app.wait_for_ui_exists("Back to Shop", 5.0f);

  log_info("TEST: Battle 1 - Clicking Back to Shop");
  app.click("Back to Shop");
  log_info("TEST: Battle 1 - Waiting for Shop screen");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_frames(5);

  const auto shop_items1 = app.read_store_options();
  app.expect_not_empty(shop_items1, "shop items should exist");
  const int price1 = shop_items1[0].price;
  app.set_wallet_gold(price1 + 10);
  app.purchase_item(shop_items1[0].type);
  app.wait_for_frames(5);

  log_info("TEST: Starting battle 2");
  app.create_inventory_item(DishType::Potato, 0);
  app.wait_for_frames(2);
  app.wait_for_ui_exists("Next Round");
  app.click("Next Round");
  app.wait_for_frames(3);
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_frames(10);
  app.wait_for_battle_initialized(10.0f);
  app.expect_screen_is(GameStateManager::Screen::Battle);

  // Verify combat ticks occurred for Battle 2
  app.expect_combat_ticks_occurred(1);

  app.wait_for_ui_exists("Skip to Results", 5.0f);
  app.click("Skip to Results");
  app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
  app.wait_for_ui_exists("Back to Shop", 5.0f);

  app.click("Back to Shop");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_frames(5);

  const auto shop_items2 = app.read_store_options();
  app.expect_not_empty(shop_items2, "shop items should exist");
  const int price2 = shop_items2[0].price;
  app.set_wallet_gold(price2 + 10);
  app.purchase_item(shop_items2[0].type);
  app.wait_for_frames(5);

  log_info("TEST: Starting battle 3");
  app.create_inventory_item(DishType::Potato, 0);
  app.wait_for_frames(2);
  app.wait_for_ui_exists("Next Round");
  app.click("Next Round");
  app.wait_for_frames(3);
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_frames(10);
  app.wait_for_battle_initialized(10.0f);
  app.expect_screen_is(GameStateManager::Screen::Battle);

  // Verify combat ticks occurred for Battle 3
  app.expect_combat_ticks_occurred(1);

  app.wait_for_ui_exists("Skip to Results", 5.0f);
  app.click("Skip to Results");
  app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
  app.wait_for_ui_exists("Back to Shop", 5.0f);

  log_info("TEST: Completed all three battles successfully");
  app.expect_screen_is(GameStateManager::Screen::Results);
  log_info("TEST: validate_three_battles_flow test passed");
}
