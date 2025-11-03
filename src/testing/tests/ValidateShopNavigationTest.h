#pragma once

#include "../test_macros.h"

TEST(validate_shop_navigation) {
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f); // Longer timeout
  app.wait_for_ui_exists("Next Round");
  // Reroll button starts at cost 1 (base=1, increment=0)
  app.wait_for_ui_exists("Reroll (1)");
}
