#pragma once

#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../test_macros.h"

TEST(validate_combat_system) {
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

  // TODO: Validate that dishes have proper CombatStats
  // Expected: Zing = spice + acidity + umami (0-3), Body = satiety + richness
  // + sweetness + freshness (0-4)
  // Bug: Combat stats calculation may be wrong

  // TODO: Validate level scaling
  // Expected: If DishLevel.level > 1, multiply both Zing and Body by 2
  // Bug: Level scaling may not be applied

  // TODO: Validate DishBattleState phase transitions
  // Expected: InQueue → Entering → InCombat → Finished
  // Bug: Phase transitions may not be working properly
}
