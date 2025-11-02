#pragma once

#include "../test_macros.h"

TEST(play_navigates_to_shop) {
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
}
