#pragma once

#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../test_macros.h"

TEST(goto_battle) {
  app.launch_game();
  app.navigate_to_shop();

  const auto inventory = app.read_player_inventory();
  if (inventory.empty()) {
    app.create_inventory_item(DishType::Potato, 0);
    app.wait_for_frames(2);
  }

  app.wait_for_ui_exists("Next Round", 5.0f);
  app.click("Next Round");
  app.wait_for_battle_initialized(30.0f);
  app.wait_for_dishes_in_combat(1, 30.0f);
  app.wait_for_frames(5);
  app.wait_for_ui_exists("Skip to Results", 5.0f);
}
