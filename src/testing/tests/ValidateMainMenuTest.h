#pragma once

#include "../test_macros.h"

TEST(validate_main_menu) {
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.wait_for_ui_exists("Settings");
  app.wait_for_ui_exists("Dishes");
  app.wait_for_ui_exists("Quit");
}
