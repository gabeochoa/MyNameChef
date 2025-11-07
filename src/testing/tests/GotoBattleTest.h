#pragma once

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

    // TODO: Add dish creation back once inventory setup changes
    // Currently relying on existing inventory from game setup for export menu
    // snapshot

    app.click("Next Round");
    app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
    app.wait_for_ui_exists("Skip to Results", 5.0f);
  }
}
