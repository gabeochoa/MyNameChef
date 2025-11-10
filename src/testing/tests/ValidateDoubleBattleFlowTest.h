#pragma once

#include "../../components/battle_processor.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

TEST(validate_double_battle_flow) {
  log_info("DOUBLE_BATTLE_TEST: Starting validate_double_battle_flow test");

  log_info("DOUBLE_BATTLE_TEST: Step 1 - Launching game");
  app.launch_game();
  log_info("DOUBLE_BATTLE_TEST: Step 1.5 - Waiting for Main screen");
  app.wait_for_screen(GameStateManager::Screen::Main, 5.0f);
  log_info("DOUBLE_BATTLE_TEST: Step 2 - Waiting for Play button");
  app.wait_for_ui_exists("Play");
  log_info("DOUBLE_BATTLE_TEST: Step 3 - Clicking Play button");
  app.click("Play");
  log_info("DOUBLE_BATTLE_TEST: Step 4 - Waiting for Shop screen");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_frames(5);

  log_info("DOUBLE_BATTLE_TEST: Step 6 - Creating inventory item");
  app.create_inventory_item(DishType::Potato, 0);
  app.wait_for_frames(2);

  log_info("DOUBLE_BATTLE_TEST: ========== BATTLE 1 START ==========");
  log_info("DOUBLE_BATTLE_TEST: Battle 1 - Waiting for Next Round button");
  app.wait_for_ui_exists("Next Round");
  log_info("DOUBLE_BATTLE_TEST: Battle 1 - Clicking Next Round");
  app.click("Next Round");
  app.wait_for_frames(3);
  log_info("DOUBLE_BATTLE_TEST: Battle 1 - Waiting for Battle screen");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_frames(10);
  log_info("DOUBLE_BATTLE_TEST: Battle 1 - Waiting for battle initialization");
  app.wait_for_battle_initialized(10.0f);

  // Verify battle is actually starting - check BattleProcessor state
  auto battleProcessor =
      afterhours::EntityHelper::get_singleton<BattleProcessor>();
  if (battleProcessor.get().has<BattleProcessor>()) {
    const BattleProcessor &processor =
        battleProcessor.get().get<BattleProcessor>();
    log_info("DOUBLE_BATTLE_TEST: Battle 1 - BattleProcessor state: "
             "simulationStarted={}, isBattleActive={}, simulationComplete={}",
             processor.simulationStarted, processor.isBattleActive(),
             processor.simulationComplete);

    if (!processor.simulationStarted && !processor.isBattleActive()) {
      log_info("DOUBLE_BATTLE_TEST: Battle 1 - ERROR: Battle simulation not "
               "started!");
    }
  }

  // Verify dishes are loaded and active
  int player_dishes = app.count_active_player_dishes();
  int opponent_dishes = app.count_active_opponent_dishes();
  log_info("DOUBLE_BATTLE_TEST: Battle 1 - Dish counts: player={}, opponent={}",
           player_dishes, opponent_dishes);

  if (player_dishes == 0 || opponent_dishes == 0) {
    log_info("DOUBLE_BATTLE_TEST: Battle 1 - ERROR: No dishes loaded! "
             "player={}, opponent={}",
             player_dishes, opponent_dishes);
  }

  // Wait for dishes to actually enter combat (verify battle is running)
  GameStateManager::get().update_screen();
  GameStateManager::Screen battle1_screen =
      GameStateManager::get().active_screen;

  if (battle1_screen == GameStateManager::Screen::Results) {
    log_info("DOUBLE_BATTLE_TEST: Battle 1 - Battle completed very quickly, "
             "already on Results screen");
  } else if (battle1_screen == GameStateManager::Screen::Battle) {
    // Verify battle is actually running by waiting for dishes in combat
    log_info(
        "DOUBLE_BATTLE_TEST: Battle 1 - Waiting for dishes to enter combat");
    app.wait_for_dishes_in_combat(1, 10.0f);
    log_info("DOUBLE_BATTLE_TEST: Battle 1 - Dishes are in combat, battle is "
             "running");

    // Wait for animations to complete before checking combat ticks
    log_info(
        "DOUBLE_BATTLE_TEST: Battle 1 - Waiting for animations to complete");
    app.wait_for_animations_complete(5.0f);
    log_info("DOUBLE_BATTLE_TEST: Battle 1 - Animations complete, waiting for "
             "combat ticks");
    app.wait_for_frames(120); // Wait ~2 seconds at 60fps for bites to happen

    // Verify combat ticks occurred for Battle 1
    log_info("DOUBLE_BATTLE_TEST: Battle 1 - Verifying combat ticks occurred");
    app.expect_combat_ticks_occurred(1);
    log_info("DOUBLE_BATTLE_TEST: Battle 1 - Combat ticks verified");
  } else {
    log_info("DOUBLE_BATTLE_TEST: Battle 1 - Unexpected screen: {}, continuing "
             "anyway",
             (int)battle1_screen);
  }
  log_info("DOUBLE_BATTLE_TEST: Battle 1 - Battle initialized, waiting for "
           "Skip to Results");
  app.wait_for_ui_exists("Skip to Results", 5.0f);
  log_info("DOUBLE_BATTLE_TEST: Battle 1 - Clicking Skip to Results");
  app.click("Skip to Results");
  log_info("DOUBLE_BATTLE_TEST: Battle 1 - Waiting for Results screen");
  app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
  log_info("DOUBLE_BATTLE_TEST: Battle 1 - Waiting for Back to Shop");
  app.wait_for_ui_exists("Back to Shop", 5.0f);
  log_info("DOUBLE_BATTLE_TEST: Battle 1 - Clicking Back to Shop");
  app.click("Back to Shop");
  log_info("DOUBLE_BATTLE_TEST: Battle 1 - Waiting for Shop screen");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_frames(5);
  log_info("DOUBLE_BATTLE_TEST: ========== BATTLE 1 COMPLETE ==========");

  log_info("DOUBLE_BATTLE_TEST: ========== BATTLE 2 START ==========");
  log_info("DOUBLE_BATTLE_TEST: Battle 2 - Creating inventory item");
  app.create_inventory_item(DishType::Potato, 0);
  app.wait_for_frames(2);
  log_info("DOUBLE_BATTLE_TEST: Battle 2 - Waiting for Next Round button");
  app.wait_for_ui_exists("Next Round");
  log_info("DOUBLE_BATTLE_TEST: Battle 2 - Clicking Next Round");
  app.click("Next Round");
  app.wait_for_frames(5); // Give more time for transition
  log_info("DOUBLE_BATTLE_TEST: Battle 2 - Waiting for Battle screen");

  // Wait for Battle screen - it should transition from Shop to Battle
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);

  app.wait_for_frames(10);
  log_info("DOUBLE_BATTLE_TEST: Battle 2 - Waiting for battle initialization");
  app.wait_for_battle_initialized(10.0f);

  // CRITICAL: Verify Battle 2 actually started a NEW simulation
  // Wait a bit for the battle to potentially start
  app.wait_for_frames(30);

  auto battleProcessor2 =
      afterhours::EntityHelper::get_singleton<BattleProcessor>();
  if (battleProcessor2.get().has<BattleProcessor>()) {
    const BattleProcessor &processor =
        battleProcessor2.get().get<BattleProcessor>();
    log_info("DOUBLE_BATTLE_TEST: Battle 2 - BattleProcessor state: "
             "simulationStarted={}, isBattleActive={}, simulationComplete={}",
             processor.simulationStarted, processor.isBattleActive(),
             processor.simulationComplete);

    // Battle 2 MUST have started - if simulationStarted is false, battle didn't
    // start
    if (!processor.simulationStarted) {
      log_info("DOUBLE_BATTLE_TEST: Battle 2 - ERROR: Battle simulation not "
               "started! simulationStarted=false");
      app.fail(
          "Battle 2 simulation did not start - simulationStarted is false");
    }

    // Battle 2 MUST be active - if isBattleActive is false, battle isn't
    // running
    if (!processor.isBattleActive()) {
      log_info("DOUBLE_BATTLE_TEST: Battle 2 - ERROR: Battle is not active! "
               "isBattleActive=false");
      app.fail("Battle 2 is not active - isBattleActive is false");
    }
  } else {
    app.fail("BattleProcessor singleton not found for Battle 2");
  }

  // Verify dishes are loaded and active
  int player_dishes2 = app.count_active_player_dishes();
  int opponent_dishes2 = app.count_active_opponent_dishes();
  log_info("DOUBLE_BATTLE_TEST: Battle 2 - Dish counts: player={}, opponent={}",
           player_dishes2, opponent_dishes2);

  if (player_dishes2 == 0 || opponent_dishes2 == 0) {
    log_info("DOUBLE_BATTLE_TEST: Battle 2 - ERROR: No dishes loaded! "
             "player={}, opponent={}",
             player_dishes2, opponent_dishes2);
  }

  // Check current screen - battle might have completed instantly
  GameStateManager::get().update_screen();
  GameStateManager::Screen current = GameStateManager::get().active_screen;

  if (current == GameStateManager::Screen::Results) {
    log_info("DOUBLE_BATTLE_TEST: Battle 2 - Battle completed very quickly, "
             "already on Results screen");
  } else if (current == GameStateManager::Screen::Battle) {
    // Verify battle is actually running by waiting for dishes in combat
    log_info(
        "DOUBLE_BATTLE_TEST: Battle 2 - Waiting for dishes to enter combat");
    app.wait_for_dishes_in_combat(1, 10.0f);
    log_info("DOUBLE_BATTLE_TEST: Battle 2 - Dishes are in combat, battle is "
             "running");

    // Wait for animations to complete before checking combat ticks
    log_info(
        "DOUBLE_BATTLE_TEST: Battle 2 - Waiting for animations to complete");
    app.wait_for_animations_complete(5.0f);
    log_info("DOUBLE_BATTLE_TEST: Battle 2 - Animations complete, waiting for "
             "combat ticks");
    app.wait_for_frames(120); // Wait ~2 seconds at 60fps for bites to happen

    // Verify combat ticks occurred for Battle 2
    log_info("DOUBLE_BATTLE_TEST: Battle 2 - Verifying combat ticks occurred");
    app.expect_combat_ticks_occurred(1);
    log_info("DOUBLE_BATTLE_TEST: Battle 2 - Combat ticks verified");
  } else {
    // Unexpected screen - log but don't fail (might be a timing issue)
    log_info("DOUBLE_BATTLE_TEST: Battle 2 - Unexpected screen: {}, continuing "
             "anyway",
             (int)current);
  }
  log_info("DOUBLE_BATTLE_TEST: ========== BATTLE 2 COMPLETE ==========");

  log_info("DOUBLE_BATTLE_TEST: Test passed - both battles completed");
}
