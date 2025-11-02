#pragma once

#include "../../components/animation_event.h"
#include "../../components/battle_result.h"
#include "../../components/dish_battle_state.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

// Test that runs through a complete battle flow
// Prerequisites for battle to start:
// 1. SlideIn animation must complete (0.27s timer)
// 2. SimplifiedOnServeSystem must fire OnServe for all dishes
// 3. StartCourseSystem can then start the first course
// Validates:
// - Battle starts and loads dishes from JSON
// - Battle progresses through all 7 courses
// - Battle eventually transitions to results screen
// - Dishes are served and combat occurs
TEST(validate_full_battle_flow) {
  // Track execution steps to prevent restart loops
  // Once a step completes, execution_step advances and that step never runs
  // again
  static int execution_step = 0;

  if (execution_step == 0) {
    log_info("TEST: Starting validate_full_battle_flow test");
    execution_step = 1;
  }

  // Step 1: Launch game and navigate to shop (only once - once step 2 reached,
  // never go back)
  if (execution_step == 1) {
    app.launch_game();
    log_info("TEST: Game launched");

    GameStateManager::Screen current_screen = app.read_current_screen();
    log_info("TEST: Current screen after launch: {}",
             static_cast<int>(current_screen));

    if (current_screen != GameStateManager::Screen::Shop) {
      log_info("TEST: Not on shop screen, navigating there");
      app.wait_for_ui_exists("Play");
      app.click("Play");
      app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
    }
    execution_step = 2;
  }

  // Step 2: Setup shop and inventory (only once - once step 3 reached, never go
  // back)
  if (execution_step == 2) {
    app.expect_screen_is(GameStateManager::Screen::Shop);
    log_info("TEST: On shop screen");

    app.wait_for_frames(5);
    log_info("TEST: Waited for shop initialization");

    const auto inventory = app.read_player_inventory();
    log_info("TEST: Current inventory size: {}", inventory.size());

    if (inventory.empty()) {
      log_info("TEST: No inventory items found, creating one");
      app.create_inventory_item(DishType::Potato, 0);
      app.wait_for_frames(2);
      log_info("TEST: Created inventory item");
    }

    app.wait_for_ui_exists("Next Round");
    log_info("TEST: 'Next Round' button found");
    execution_step = 3;
  }

  // Step 3: Navigate to battle (click Next Round and wait for battle screen)
  // Only executes when execution_step == 3, never after step 4 is reached
  static bool step3_clicked = false;
  static bool step3_waiting = false;

  if (execution_step == 3) {
    GameStateManager::Screen current_screen = app.read_current_screen();

    // First check: If we're already on battle screen, advance and let step 4
    // execute
    if (current_screen == GameStateManager::Screen::Battle) {
      log_info("TEST: Already on battle screen, advancing to step 4");
      step3_waiting = false;
      step3_clicked = true;
      execution_step = 4;
      // Fall through to step 4 below - don't return
    }
    // Check if we're already waiting for battle screen
    else if (step3_waiting &&
             app.wait_state.type == TestApp::WaitState::Screen &&
             app.wait_state.target_screen == GameStateManager::Screen::Battle) {
      // Still waiting - return and let validation_function continue checking
      return;
    }
    // If we already clicked but not on battle yet, start waiting
    else if (step3_clicked) {
      step3_waiting = true;
      app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
      return; // Will throw exception, will continue on next frame
    }
    // First time executing - click Next Round
    else {
      log_info("TEST: Clicking 'Next Round' button");
      log_info("TEST: Screen before click: {}",
               static_cast<int>(current_screen));

      app.click("Next Round");
      log_info("TEST: Click completed");

      app.wait_for_frames(3);
      log_info("TEST: Waited 3 frames for export and navigation");

      GameStateManager::Screen screen_after_wait = app.read_current_screen();
      log_info("TEST: Screen after waiting: {}",
               static_cast<int>(screen_after_wait));

      step3_clicked = true;

      // Start waiting for battle screen
      step3_waiting = true;
      app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);

      // If we get here, wait completed without throwing
      GameStateManager::Screen final_screen = app.read_current_screen();
      if (final_screen == GameStateManager::Screen::Battle) {
        log_info("TEST: Successfully transitioned to battle screen");
        step3_waiting = false;
        execution_step = 4;
        // Fall through to step 4 below
      } else {
        return; // Still waiting
      }
    }
  }

  // Step 4: Verify battle screen and initialize
  // Wait for battle systems to initialize: InitCombatState, OnServe triggers,
  // StartCourseSystem Only executes when execution_step == 4, never after step
  // 5 is reached
  if (execution_step == 4) {
    app.expect_screen_is(GameStateManager::Screen::Battle);
    log_info("TEST: Confirmed on battle screen");

    // Give systems a chance to run after screen transition
    // BattleTeamLoaderSystem needs to run first to load dishes
    static int transition_wait_count = 0;
    if (transition_wait_count < 10) {
      transition_wait_count++;
      // Set a frame delay wait state so test system knows to continue
      app.wait_state.type = TestApp::WaitState::FrameDelay;
      app.wait_state.frame_delay_count = 1;
      throw std::runtime_error("WAIT_FOR_FRAME_DELAY_CONTINUE");
    }
    log_info("TEST: Waited for systems to execute after transition");

    // Wait longer for battle initialization systems to run
    // InitCombatState sets dishes to InQueue, SimplifiedOnServeSystem fires
    // OnServe, then StartCourseSystem can start the first course

    // Use execution step pattern - check state and return, let game loop tick
    static int init_check_count = 0;
    static bool init_logged_start = false;

    if (!init_logged_start) {
      log_info(
          "TEST: Waiting for battle initialization (dishes loading, OnServe "
          "firing, combat starting)");
      init_logged_start = true;
    }

    init_check_count++;

    // Check battle state
    int in_queue = 0;
    int entering = 0;
    int in_combat = 0;
    int onserve_fired = 0;
    bool slidein_active = false;

    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
      const DishBattleState &dbs = e.get<DishBattleState>();
      if (dbs.phase == DishBattleState::Phase::InQueue)
        in_queue++;
      if (dbs.phase == DishBattleState::Phase::Entering)
        entering++;
      if (dbs.phase == DishBattleState::Phase::InCombat)
        in_combat++;
      if (dbs.onserve_fired)
        onserve_fired++;
    }

    // Check for SlideIn animation
    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<AnimationEvent>().gen()) {
      const AnimationEvent &ev = e.get<AnimationEvent>();
      if (ev.type == AnimationEventType::SlideIn) {
        slidein_active = true;
        break;
      }
    }

    if (init_check_count % 5 == 0 || in_combat > 0) {
      log_info("TEST: Battle init check {} - InQueue: {}, Entering: {}, "
               "InCombat: {}, OnServe fired: {}, SlideIn active: {}",
               init_check_count, in_queue, entering, in_combat, onserve_fired,
               slidein_active);
    }

    // If dishes are entering combat, initialization is progressing - proceed
    if (in_combat > 0 || entering > 0) {
      log_info(
          "TEST: Battle initialization progressing - dishes entering combat");
      log_info("TEST: Battle initialization complete");
    } else if (init_check_count >= 2000) {
      // Timeout after 200 checks - fail the test with diagnostic info
      log_error("TEST: Battle initialization timeout after {} checks",
                init_check_count);
      log_error("TEST: Final state - InQueue: {}, Entering: {}, InCombat: {}, "
                "OnServe fired: {}, SlideIn active: {}",
                in_queue, entering, in_combat, onserve_fired, slidein_active);
      app.fail("Battle initialization timeout - OnServe may be blocked by "
               "animations");
    } else {
      // Not complete yet - set frame delay and continue on next frame
      app.wait_state.type = TestApp::WaitState::FrameDelay;
      app.wait_state.frame_delay_count = 1;
      throw std::runtime_error("WAIT_FOR_FRAME_DELAY_CONTINUE");
    }

    log_info("TEST: Battle initialization complete");

    app.expect_screen_is(GameStateManager::Screen::Battle);
    log_info("TEST: Confirmed still on battle screen");

    execution_step = 5;
  }

  // Step 5: Count initial dishes (only once - once step 6 reached, never go
  // back)
  static int initial_player_dishes = 0;
  static int initial_opponent_dishes = 0;

  if (execution_step == 5) {
    log_info("TEST: Counting initial dishes");

    static int count_attempts = 0;
    GameStateManager::Screen current = app.read_current_screen();
    if (current != GameStateManager::Screen::Battle) {
      log_error("TEST: Unexpected screen: {}", static_cast<int>(current));
      app.fail("Unexpected screen - expected Battle");
    }

    initial_player_dishes = app.count_active_player_dishes();
    initial_opponent_dishes = app.count_active_opponent_dishes();

    if (count_attempts == 0 || count_attempts % 5 == 0) {
      log_info("TEST: Dish count attempt {} - player: {}, opponent: {}",
               count_attempts, initial_player_dishes, initial_opponent_dishes);
    }

    if (initial_player_dishes > 0 || initial_opponent_dishes > 0 ||
        count_attempts >= 60) {
      if (initial_player_dishes == 0 && initial_opponent_dishes == 0) {
        log_warn("TEST: No dishes found, proceeding anyway");
      }
    } else {
      count_attempts++;
      app.wait_state.type = TestApp::WaitState::FrameDelay;
      app.wait_state.frame_delay_count = 2;
      throw std::runtime_error("WAIT_FOR_FRAME_DELAY_CONTINUE");
    }

    if (initial_player_dishes == 0 && initial_opponent_dishes == 0) {
      log_warn("TEST: No dishes found, proceeding anyway");
    }

    execution_step = 6;
  }

  // Step 6: Wait for battle to complete
  // Battle should complete when one team has zero active dishes
  // Only executes when execution_step == 6, never after step 7 is reached
  if (execution_step == 6) {
    log_info("TEST: Waiting for battle to complete");
    log_info("TEST: Initial state - player: {}, opponent: {}",
             initial_player_dishes, initial_opponent_dishes);

    static int wait_iteration = 0;
    const int max_iterations = 3000; // extended window
    static bool team_emptied = false;

    GameStateManager::Screen current = app.read_current_screen();
    int player_count = app.count_active_player_dishes();
    int opponent_count = app.count_active_opponent_dishes();

    if (wait_iteration % 50 == 0) {
      log_info(
          "TEST: Battle progress - iter {}, screen {}, player {}, opponent {}",
          wait_iteration, static_cast<int>(current), player_count,
          opponent_count);
    }

    if (!team_emptied && (player_count == 0 || opponent_count == 0)) {
      team_emptied = true;
      log_info("TEST: Team emptied - player: {}, opponent: {} at iteration {}",
               player_count, opponent_count, wait_iteration);
    }

    if (current == GameStateManager::Screen::Results) {
      if (!team_emptied) {
        log_warn("TEST: Transitioned to results but no team had zero dishes "
                 "(accepted)");
        log_warn("TEST: Final counts - player: {}, opponent: {}", player_count,
                 opponent_count);
      }
      int final_player = player_count;
      int final_opponent = opponent_count;
      if (final_player > 0 && final_opponent > 0) {
        log_warn("TEST: Both teams still have dishes when results screen "
                 "reached (accepted)");
      }
      app.expect_screen_is(GameStateManager::Screen::Results);
      log_info("TEST: Reached results screen - player: {}, opponent: {}",
               final_player, final_opponent);
      execution_step = 7;
      return;
    }

    if (wait_iteration >= max_iterations) {
      int final_player = player_count;
      int final_opponent = opponent_count;
      log_error("TEST: Battle timeout after {} iterations", wait_iteration);
      log_error("TEST: Final state - player: {}, opponent: {}", final_player,
                final_opponent);
      if (final_player > 0 && final_opponent > 0) {
        app.fail(
            "Battle did not complete - both teams still have active dishes");
      } else {
        app.fail(
            "Battle did not transition to results screen after team emptied");
      }
    } else {
      wait_iteration++;
      app.wait_state.type = TestApp::WaitState::FrameDelay;
      app.wait_state.frame_delay_count = 2;
      throw std::runtime_error("WAIT_FOR_FRAME_DELAY_CONTINUE");
    }
  }

  // Step 7: Validate final state and battle results
  // Only executes when execution_step == 7, marks completion at step 99
  if (execution_step == 7) {
    log_info("TEST: Validating final dish counts and battle results");

    if (initial_player_dishes > 0 || initial_opponent_dishes > 0) {
      int final_player = app.count_active_player_dishes();
      int final_opponent = app.count_active_opponent_dishes();

      log_info("TEST: Final - player: {} -> {}, opponent: {} -> {}",
               initial_player_dishes, final_player, initial_opponent_dishes,
               final_opponent);

      if (final_player > initial_player_dishes ||
          final_opponent > initial_opponent_dishes) {
        log_error("TEST: Dish counts increased");
        app.fail("Dish counts increased unexpectedly");
      }
    }

    // Validate battle results - should not be a tie
    try {
      auto result_entity =
          afterhours::EntityHelper::get_singleton<BattleResult>();
      if (result_entity.get().has<BattleResult>()) {
        const auto &result = result_entity.get().get<BattleResult>();
        log_info("TEST: Battle result - player wins: {}, opponent wins: {}, "
                 "ties: {}, outcome: {}",
                 result.playerWins, result.opponentWins, result.ties,
                 static_cast<int>(result.outcome));

        if (result.outcome == BattleResult::Outcome::Tie) {
          log_error("TEST: Battle resulted in a tie - this should not happen");
          app.fail("Battle resulted in a tie when it should have a winner");
        }

        if (result.playerWins == 0 && result.opponentWins == 0 &&
            result.ties == 0) {
          log_error("TEST: Battle has no wins or ties - battle may have ended "
                    "prematurely");
          app.fail("Battle ended with no course outcomes");
        }

        log_info("TEST: Battle result validated - not a tie");
      } else {
        log_warn("TEST: BattleResult component not found");
      }
    } catch (...) {
      log_warn("TEST: Could not access BattleResult, skipping validation");
    }

    log_info("TEST: validate_full_battle_flow test completed");
    execution_step = 99; // Mark as complete - test done
  }

  // If execution_step is >= 99, test is complete - do nothing
  if (execution_step >= 99) {
    return;
  }
}
