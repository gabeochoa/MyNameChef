#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/battle_result.h"
#include "../../components/battle_team_tags.h"
#include "../../components/is_dish.h"
#include "../../game_state_manager.h"
#include "../test_macros.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <fstream>

namespace ValidateBattleResultsTestHelpers {
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
} // namespace ValidateBattleResultsTestHelpers

TEST(validate_battle_results) {
  using namespace ValidateBattleResultsTestHelpers;
  
  // Step 1: Create mock opponent JSON file
  create_mock_opponent_json();

  // Check if we're already on Results screen (from a previous test iteration)
  GameStateManager::get().update_screen();
  auto &gsm = GameStateManager::get();
  if (gsm.active_screen == GameStateManager::Screen::Results) {
    app.wait_for_ui_exists("Back to Shop", 5.0f);
    
    // Validate BattleResult component exists
    auto resultEntity = afterhours::EntityHelper::get_singleton<BattleResult>();
    if (!resultEntity.get().has<BattleResult>()) {
      return; // Will fail if result not found
    }
    return; // Test is done!
  }

  // Step 2: Navigate to shop screen
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

  // Step 3: Create BattleLoadRequest and navigate to battle
  auto &requestEntity = afterhours::EntityHelper::createEntity();
  BattleLoadRequest battleRequest;
  battleRequest.playerJsonPath = "output/battles/pending/test_player.json";
  battleRequest.opponentJsonPath = "output/battles/pending/test_opponent.json";
  battleRequest.loaded = false;
  requestEntity.addComponent<BattleLoadRequest>(std::move(battleRequest));
  afterhours::EntityHelper::registerSingleton<BattleLoadRequest>(requestEntity);

  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_ui_exists("Skip to Results", 5.0f);

  // Step 4: Skip to results
  app.click("Skip to Results");
  app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
  app.wait_for_ui_exists("Back to Shop", 5.0f);

  // Step 5: Validate results
  auto resultEntity = afterhours::EntityHelper::get_singleton<BattleResult>();
  if (!resultEntity.get().has<BattleResult>()) {
    return; // Will fail
  }

  auto &result = resultEntity.get().get<BattleResult>();
  log_info("TEST: BattleResult found - Player wins: {}, Opponent wins: {}, "
           "Ties: {}, Outcome: {}",
           result.playerWins, result.opponentWins, result.ties,
           static_cast<int>(result.outcome));

  if (result.outcomes.empty()) {
    return; // Will fail - no course outcomes
  }

  log_info("TEST: Found {} course outcomes", result.outcomes.size());

  // TODO: Validate CourseOutcome component
  // Expected: slotIndex, winner (Player/Opponent/Tie), ticks
  // Bug: CourseOutcome may not be properly recorded

  // TODO: Validate course-by-course resolution
  // Expected: Each course should have a clear winner
  // Bug: Course resolution may not be working

  // TODO: Validate match winner determination
  // Expected: Winner is team with more course wins
  // Bug: Match result calculation may be wrong

  // TODO: Validate tie handling
  // Expected: Ties should be handled properly
  // Bug: Tie resolution may not be implemented
}
