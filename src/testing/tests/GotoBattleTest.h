#pragma once

#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../test_macros.h"

TEST(goto_battle) {
  GameStateManager::get().update_screen();
  auto &gsm = GameStateManager::get();
  if (gsm.active_screen == GameStateManager::Screen::Battle) {
    app.wait_for_ui_exists("Skip to Results", 5.0f);
  } else {
    app.launch_game();
    app.wait_for_ui_exists("Play");
    app.click("Play");
    app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
    app.wait_for_ui_exists("Next Round");

    const auto inventory = app.read_player_inventory();
    if (inventory.empty()) {
      app.create_inventory_item(DishType::Potato, 0);
      app.wait_for_frames(2);
    }

    app.click("Next Round");
    app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
    app.wait_for_ui_exists("Skip to Results", 5.0f);
  }
}
