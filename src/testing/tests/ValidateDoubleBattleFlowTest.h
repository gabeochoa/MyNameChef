#pragma once

#include "../../components/battle_processor.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../test_macros.h"
#include <afterhours/ah.h>
#include <functional>
#include <string>

static int battle_path_hash(const BattleProcessor &processor) {
  if (!processor.activeBattle.has_value()) {
    return 0;
  }
  return static_cast<int>(
      std::hash<std::string>{}(processor.activeBattle->playerJsonPath));
}

static void expect_battle_simulation_running(TestApp &app,
                                             const std::string &label) {
  afterhours::RefEntity processor_ref =
      afterhours::EntityHelper::get_singleton<BattleProcessor>();
  app.expect_true(processor_ref.get().has<BattleProcessor>(),
                  label + " BattleProcessor singleton exists");
  const BattleProcessor &processor =
      processor_ref.get().get<BattleProcessor>();
  log_info("DOUBLE_BATTLE_TEST: {} - simulationStarted={}, isBattleActive={}, "
           "simulationComplete={}",
           label, processor.simulationStarted, processor.isBattleActive(),
           processor.simulationComplete);
  app.expect_true(processor.simulationStarted, label + " simulationStarted");
  app.expect_true(processor.isBattleActive(), label + " isBattleActive");
  app.expect_count_gte(app.count_active_player_dishes(), 1,
                       label + " player dishes");
  app.expect_count_gte(app.count_active_opponent_dishes(), 1,
                       label + " opponent dishes");
}

TEST(validate_double_battle_flow) {
  app.delete_save_file();
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

  app.run_once("inventory1",
               [&app]() { app.create_inventory_item(DishType::Potato, 0); });
  app.wait_for_frames(2);

  log_info("DOUBLE_BATTLE_TEST: Battle 1 start");
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_battle_initialized(10.0f);
  app.run_once("battle1_running", [&app]() {
    expect_battle_simulation_running(app, "Battle 1");
    const BattleProcessor &processor =
        afterhours::EntityHelper::get_singleton<BattleProcessor>()
            .get()
            .get<BattleProcessor>();
    app.set_test_int("battle1_path", battle_path_hash(processor));
  });
  app.wait_for_battle_complete(60.0f);
  app.wait_for_results_screen(10.0f);
  app.wait_for_frames(2);
  app.run_once("results1", [&app]() { app.expect_battle_has_outcomes(); });
  app.wait_for_ui_exists("Back to Shop", 5.0f);
  app.click("Back to Shop");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

  app.run_once("inventory2", [&app]() {
    app.create_inventory_item(DishType::Potato, 0);
    app.create_inventory_item(DishType::Burger, 1);
  });
  app.wait_for_frames(2);

  log_info("DOUBLE_BATTLE_TEST: Battle 2 start");
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_battle_initialized(10.0f);
  app.run_once("battle2_running", [&app]() {
    expect_battle_simulation_running(app, "Battle 2");
    const BattleProcessor &processor =
        afterhours::EntityHelper::get_singleton<BattleProcessor>()
            .get()
            .get<BattleProcessor>();
    app.expect_true(battle_path_hash(processor) !=
                        app.get_test_int("battle1_path").value_or(0),
                    "Battle 2 runs a new simulation with a new snapshot");
  });
  app.wait_for_battle_complete(60.0f);
  app.wait_for_results_screen(10.0f);
  app.wait_for_frames(2);
  app.run_once("results2", [&app]() {
    app.expect_screen_is(GameStateManager::Screen::Results);
    app.expect_battle_has_outcomes();
  });

  log_info("DOUBLE_BATTLE_TEST: both battles completed");
}
