#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/battle_result.h"
#include "../../components/battle_team_tags.h"
#include "../../components/is_dish.h"
#include "../../game_state_manager.h"
#include "../TestInteraction.h"
#include "../UITestHelpers.h"
#include <filesystem>
#include <fstream>

struct ValidateBattleResultsTest {
  static void execute() {
    log_info("TEST: ValidateBattleResultsTest::execute() called!");
    log_info("TEST: Starting ValidateBattleResultsTest");

    // Step 1: Create mock opponent JSON file
    create_mock_opponent_json();

    // Step 2: Navigate to shop screen
    log_info("TEST: Navigating to shop screen");
    UITestHelpers::assert_click_ui("Play");

    // Manually trigger screen transition
    log_info("TEST: Manually triggering screen transition to shop");
    TestInteraction::start_game();
    GameStateManager::get().update_screen();

    log_info("TEST: Shop navigation complete, validation will run on "
             "subsequent frames");
  }

  static void create_mock_opponent_json() {
    // Create output directory if it doesn't exist
    std::filesystem::create_directories("output/battles/pending");

    // Create a simple opponent JSON file
    std::string opponentJson = R"({
    "team": [
        {
            "slot": 0,
            "dishType": "GarlicBread"
        },
        {
            "slot": 1,
            "dishType": "Potato"
        },
        {
            "slot": 2,
            "dishType": "Potato"
        }
    ],
    "seed": 1234567890,
    "meta": {
        "timestampIso": "20250111_000000",
        "gameVersion": "0.1.0"
    }
})";

    std::ofstream opponentFile("output/battles/pending/test_opponent.json");
    if (opponentFile.is_open()) {
      opponentFile << opponentJson;
      opponentFile.close();
      log_info("TEST: Created mock opponent JSON file");
    } else {
      log_error("TEST: Failed to create mock opponent JSON file");
    }

    // Create a simple player JSON file (will be populated by
    // ExportMenuSnapshotSystem)
    std::string playerJson = R"({
    "team": [
        {
            "slot": 0,
            "dishType": "Bagel"
        },
        {
            "slot": 1,
            "dishType": "Salmon"
        },
        {
            "slot": 2,
            "dishType": "Salmon"
        }
    ],
    "seed": 1234567890,
    "meta": {
        "timestampIso": "20250111_000000",
        "gameVersion": "0.1.0"
    }
})";

    std::ofstream playerFile("output/battles/pending/test_player.json");
    if (playerFile.is_open()) {
      playerFile << playerJson;
      playerFile.close();
      log_info("TEST: Created mock player JSON file");
    } else {
      log_error("TEST: Failed to create mock player JSON file");
    }
  }

  static bool validate_results_screen() {
    log_info("TEST: Validating results screen");

    // Check current screen and navigate accordingly
    auto &gsm = GameStateManager::get();
    log_info("TEST: Current screen: {}", static_cast<int>(gsm.active_screen));

    // If we're on the shop screen, navigate to battle
    if (gsm.active_screen == GameStateManager::Screen::Shop) {
      log_info("TEST: On shop screen, navigating to battle");

      // Wait for shop UI to be ready
      if (!UITestHelpers::check_ui_exists("Next Round")) {
        log_info("TEST: Next Round button not ready yet, waiting...");
        return false;
      }

      // Click Next Round to start battle
      UITestHelpers::assert_click_ui("Next Round");

      // Create BattleLoadRequest singleton for battle
      auto &requestEntity = afterhours::EntityHelper::createEntity();
      BattleLoadRequest battleRequest;
      battleRequest.playerJsonPath = "output/battles/pending/test_player.json";
      battleRequest.opponentJsonPath =
          "output/battles/pending/test_opponent.json";
      battleRequest.loaded =
          false; // Not loaded yet, will be loaded by battle systems
      requestEntity.addComponent<BattleLoadRequest>(std::move(battleRequest));
      afterhours::EntityHelper::registerSingleton<BattleLoadRequest>(
          requestEntity);

      // Manually trigger screen transition to battle
      GameStateManager::get().set_next_screen(GameStateManager::Screen::Battle);
      GameStateManager::get().update_screen();
      return false; // Not done yet, need to wait for battle screen
    }

    // If we're on the battle screen, wait for battle to complete or skip to
    // results
    if (gsm.active_screen == GameStateManager::Screen::Battle) {
      log_info("TEST: On battle screen");

      // Check if we can skip to results (for faster testing)
      if (UITestHelpers::check_ui_exists("Skip to Results")) {
        log_info("TEST: Skipping to results for faster testing");
        UITestHelpers::assert_click_ui("Skip to Results");
        GameStateManager::get().set_next_screen(
            GameStateManager::Screen::Results);
        GameStateManager::get().update_screen();
        return false; // Not done yet, need to wait for results screen
      } else {
        log_info("TEST: No Skip to Results button, waiting for battle to "
                 "complete naturally...");
        return false; // Wait for battle to complete
      }
    }

    // If we're on the results screen, validate it
    if (gsm.active_screen == GameStateManager::Screen::Results) {
      log_info("TEST: On results screen, validating");

      // Debug: Check if battle teams exist
      int playerTeamCount = 0;
      int opponentTeamCount = 0;
      for (auto &ref : afterhours::EntityQuery()
                           .template whereHasComponent<IsPlayerTeamItem>()
                           .template whereHasComponent<IsDish>()
                           .gen()) {
        playerTeamCount++;
      }
      for (auto &ref : afterhours::EntityQuery()
                           .template whereHasComponent<IsOpponentTeamItem>()
                           .template whereHasComponent<IsDish>()
                           .gen()) {
        opponentTeamCount++;
      }
      log_info("TEST: Found {} player team entities, {} opponent team entities",
               playerTeamCount, opponentTeamCount);

      // Test 1: Validate BattleResult component exists
      auto resultEntity =
          afterhours::EntityHelper::get_singleton<BattleResult>();
      if (!resultEntity.get().has<BattleResult>()) {
        log_info("TEST: BattleResult component not found");
        return false;
      }

      auto &result = resultEntity.get().get<BattleResult>();
      log_info("TEST: BattleResult found - Player wins: {}, Opponent wins: {}, "
               "Ties: {}, Outcome: {}",
               result.playerWins, result.opponentWins, result.ties,
               static_cast<int>(result.outcome));

      // Test 2: Validate that we have course outcomes
      if (result.outcomes.empty()) {
        log_info("TEST: No course outcomes found");
        return false;
      }

      log_info("TEST: Found {} course outcomes", result.outcomes.size());

      // Test 3: Validate UI elements exist (Back to Shop button)
      if (!UITestHelpers::check_ui_exists("Back to Shop")) {
        log_info("TEST: Back to Shop button not found");
        return false;
      }

      log_info("TEST: All validation checks passed");
      return true;
    }

    // If we're not on any expected screen, something went wrong
    log_error("TEST: Unexpected screen: {}",
              static_cast<int>(gsm.active_screen));
    return false;
  }

  static bool validate_course_outcomes() {
    // TODO: Validate CourseOutcome component
    // Expected: slotIndex, winner (Player/Opponent/Tie), ticks
    // Bug: CourseOutcome may not be properly recorded

    // TODO: Validate course-by-course resolution
    // Expected: Each course should have a clear winner
    // Bug: Course resolution may not be working

    return true; // Placeholder
  }

  static bool validate_match_results() {
    // TODO: Validate match winner determination
    // Expected: Winner is team with more course wins
    // Bug: Match result calculation may be wrong

    // TODO: Validate tie handling
    // Expected: Ties should be handled properly
    // Bug: Tie resolution may not be implemented

    return true; // Placeholder
  }
};
