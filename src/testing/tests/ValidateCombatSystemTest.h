#pragma once

#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../test_macros.h"
#include <source_location>

TEST(validate_combat_system) {
  app.wait_for_frames(1);
  auto &gsm = GameStateManager::get();
  gsm.update_screen();
  
  // Handle Results screen if we're already on it
  if (gsm.active_screen == GameStateManager::Screen::Results) {
    app.wait_for_ui_exists("Next Round", 10.0f);
    app.wait_for_frames(2);
    // Don't click yet - let the stage logic handle it
    return;
  }
  
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

  const TestOperationID enter_battle_stage =
      TestApp::generate_operation_id(std::source_location::current(),
                                     "validate_combat_system.enter_battle");
  if (!app.completed_operations.count(enter_battle_stage)) {
    const auto inventory = app.read_player_inventory();
    if (inventory.empty()) {
      app.create_inventory_item(DishType::Potato, 0);
      app.wait_for_frames(2);
    }

    app.click("Next Round");
    app.wait_for_battle_complete(60.0f);
    app.wait_for_results_screen(10.0f);
    app.wait_for_ui_exists("Next Round", 10.0f);
    app.completed_operations.insert(enter_battle_stage);
    return;
  }

  const TestOperationID back_to_shop_stage =
      TestApp::generate_operation_id(std::source_location::current(),
                                     "validate_combat_system.back_to_shop");
  if (!app.completed_operations.count(back_to_shop_stage)) {
    app.wait_for_results_screen(10.0f);
    app.wait_for_ui_exists("Next Round", 10.0f);
    app.wait_for_frames(2);
    app.click("Next Round");
    app.wait_for_screen(GameStateManager::Screen::Shop, 15.0f);
    app.wait_for_ui_exists("Next Round", 5.0f);
    app.completed_operations.insert(back_to_shop_stage);
    return;
  }

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
