#pragma once

#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

TEST(validate_three_battles_flow) {
  log_info("TEST: Starting validate_three_battles_flow test");

  app.launch_game();

  GameStateManager::Screen current_screen = app.read_current_screen();
  app.expect_true(current_screen == GameStateManager::Screen::Main ||
                      current_screen == GameStateManager::Screen::Shop,
                  "should start on Main or Shop");
  if (current_screen == GameStateManager::Screen::Main) {
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

  log_info("TEST: Starting battle 1");
  app.wait_for_ui_exists("Next Round");
  app.click("Next Round");
  app.wait_for_frames(3);
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_frames(10);
  app.wait_for_battle_initialized(10.0f);
  app.expect_screen_is(GameStateManager::Screen::Battle);

  app.wait_for_ui_exists("Skip to Results", 5.0f);
  app.click("Skip to Results");
  app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
  app.wait_for_ui_exists("Back to Shop", 5.0f);

  app.click("Back to Shop");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_frames(5);

  const auto shop_items1 = app.read_store_options();
  app.expect_not_empty(shop_items1, "shop items should exist");
  const int price1 = shop_items1[0].price;
  const int current_gold1 = app.read_wallet_gold();
  if (current_gold1 < price1) {
    app.set_wallet_gold(price1 + 10);
  }
  app.purchase_item(shop_items1[0].type);
  app.wait_for_frames(5);

  log_info("TEST: Starting battle 2");
  const auto inventory2 = app.read_player_inventory();
  if (inventory2.empty()) {
    app.create_inventory_item(DishType::Potato, 0);
    app.wait_for_frames(2);
  }
  app.wait_for_ui_exists("Next Round");
  app.click("Next Round");
  app.wait_for_frames(3);
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_frames(10);
  app.wait_for_battle_initialized(10.0f);
  app.expect_screen_is(GameStateManager::Screen::Battle);

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
  const int current_gold2 = app.read_wallet_gold();
  if (current_gold2 < price2) {
    app.set_wallet_gold(price2 + 10);
  }
  app.purchase_item(shop_items2[0].type);
  app.wait_for_frames(5);

  log_info("TEST: Starting battle 3");
  const auto inventory3 = app.read_player_inventory();
  if (inventory3.empty()) {
    app.create_inventory_item(DishType::Potato, 0);
    app.wait_for_frames(2);
  }
  app.wait_for_ui_exists("Next Round");
  app.click("Next Round");
  app.wait_for_frames(3);
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_frames(10);
  app.wait_for_battle_initialized(10.0f);
  app.expect_screen_is(GameStateManager::Screen::Battle);

  app.wait_for_ui_exists("Skip to Results", 5.0f);
  app.click("Skip to Results");
  app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
  app.wait_for_ui_exists("Back to Shop", 5.0f);

  log_info("TEST: Completed all three battles successfully");
  app.expect_screen_is(GameStateManager::Screen::Results);
  log_info("TEST: validate_three_battles_flow test passed");
}
