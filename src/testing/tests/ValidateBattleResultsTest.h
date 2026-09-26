#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/battle_result.h"
#include "../../components/battle_team_tags.h"
#include "../../components/is_dish.h"
#include "../../game_state_manager.h"
#include "../test_app.h"
#include "../test_macros.h"
#include <afterhours/ah.h>


TEST(validate_battle_results) {
  app.wait_for_frames(1); // Ensure screen state is synced
  auto &gsm = GameStateManager::get();
  if (gsm.active_screen == GameStateManager::Screen::Results) {
    app.wait_for_ui_exists("Back to Shop", 5.0f);
    auto resultEntity = afterhours::EntityHelper::get_singleton<BattleResult>();
    app.expect_singleton_has_component<BattleResult>(resultEntity, "BattleResult");
    return;
  }

  // Step 2: Navigate to shop screen
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

  // Step 2.5: Create dishes in inventory (required to proceed past shop screen)
  const auto inventory = app.read_player_inventory();
  if (inventory.empty()) {
    app.create_inventory_item(DishType::Potato, 0);
    app.wait_for_frames(2);
  }

  // Step 3: Navigate to battle. Server-only flow (settled): Next Round
  // uploads the team and the server matches an opponent; local mock battle
  // files are no longer used.
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_battle_initialized(30.0f);
  app.wait_for_ui_exists("Skip to Results", 5.0f);

  // Step 4: Skip to results
  app.click("Skip to Results");
  app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
  app.wait_for_ui_exists("Back to Shop", 5.0f);

  auto resultEntity = afterhours::EntityHelper::get_singleton<BattleResult>();
  app.expect_singleton_has_component<BattleResult>(resultEntity, "BattleResult");

  auto &result = resultEntity.get().get<BattleResult>();
  log_info("TEST: BattleResult found - Player wins: {}, Opponent wins: {}, "
           "Ties: {}, Outcome: {}",
           result.playerWins, result.opponentWins, result.ties,
           static_cast<int>(result.outcome));

  app.expect_not_empty(result.outcomes, "course outcomes");

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
