#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/battle_result.h"
#include "../../components/replay_state.h"
#include "../../game_state_manager.h"
#include "../../seeded_rng.h"
#include "../test_app.h"
#include "../test_macros.h"
#include <afterhours/ah.h>
#include <string>
#include <vector>

namespace ValidateServerBattleDeterminismTestHelpers {
static void ensure_battle_load_request_exists(uint64_t seed) {
  const auto componentId =
      afterhours::components::get_type_id<BattleLoadRequest>();
  auto &singletonMap = afterhours::EntityHelper::get().singletonMap;
  if (singletonMap.contains(componentId)) {
    return;
  }
  auto &requestEntity = afterhours::EntityHelper::createEntity();
  BattleLoadRequest request;
  request.playerJsonPath = "";
  request.opponentJsonPath = "";
  request.loaded = false;
  requestEntity.addComponent<BattleLoadRequest>(std::move(request));
  afterhours::EntityHelper::registerSingleton<BattleLoadRequest>(requestEntity);
}

static void ensure_replay_state_exists(uint64_t seed) {
  const auto componentId = afterhours::components::get_type_id<ReplayState>();
  auto &singletonMap = afterhours::EntityHelper::get().singletonMap;
  if (singletonMap.contains(componentId)) {
    auto replayEntity = afterhours::EntityHelper::get_singleton<ReplayState>();
    if (replayEntity.get().has<ReplayState>()) {
      auto &replay = replayEntity.get().get<ReplayState>();
      replay.seed = seed;
      replay.active = true;
      replay.paused = false;
      replay.timeScale = 1.0f;
    }
    return;
  }
  auto &replayEntity = afterhours::EntityHelper::createEntity();
  ReplayState replay;
  replay.seed = seed;
  replay.active = true;
  replay.paused = false;
  replay.timeScale = 1.0f;
  replayEntity.addComponent<ReplayState>(std::move(replay));
  afterhours::EntityHelper::registerSingleton<ReplayState>(replayEntity);
}

struct BattleRunResult {
  BattleResult::Outcome outcome;
  int playerWins;
  int opponentWins;
  int ties;
  std::vector<BattleResult::CourseOutcome> outcomes;
};

static BattleRunResult capture_battle_result() {
  BattleRunResult result;
  auto resultEntity = afterhours::EntityHelper::get_singleton<BattleResult>();
  if (resultEntity.get().has<BattleResult>()) {
    const auto &battleResult = resultEntity.get().get<BattleResult>();
    result.outcome = battleResult.outcome;
    result.playerWins = battleResult.playerWins;
    result.opponentWins = battleResult.opponentWins;
    result.ties = battleResult.ties;
    result.outcomes = battleResult.outcomes;
  }
  return result;
}

static bool compare_battle_results(const BattleRunResult &first,
                                   const BattleRunResult &second) {
  if (first.outcome != second.outcome) {
    return false;
  }
  if (first.playerWins != second.playerWins) {
    return false;
  }
  if (first.opponentWins != second.opponentWins) {
    return false;
  }
  if (first.ties != second.ties) {
    return false;
  }
  if (first.outcomes.size() != second.outcomes.size()) {
    return false;
  }
  for (size_t i = 0; i < first.outcomes.size(); ++i) {
    if (first.outcomes[i].slotIndex != second.outcomes[i].slotIndex) {
      return false;
    }
    if (first.outcomes[i].winner != second.outcomes[i].winner) {
      return false;
    }
    if (first.outcomes[i].ticks != second.outcomes[i].ticks) {
      return false;
    }
  }
  return true;
}
} // namespace ValidateServerBattleDeterminismTestHelpers

TEST(validate_server_battle_determinism) {
  using namespace ValidateServerBattleDeterminismTestHelpers;

  const uint64_t test_seed = 9876543210987654321ULL;

  static const TestOperationID first_run_op = TestApp::generate_operation_id(
      std::source_location::current(),
      "validate_server_battle_determinism.first_run");
  static const TestOperationID second_run_op = TestApp::generate_operation_id(
      std::source_location::current(),
      "validate_server_battle_determinism.second_run");
  static BattleRunResult first_run_result;

  if (app.completed_operations.count(first_run_op) == 0) {
    app.launch_game();
    app.wait_for_frames(1);
    app.clear_battle_dishes();
    ValidateServerBattleDeterminismTestHelpers::
        ensure_battle_load_request_exists(test_seed);
    app.setup_battle();
    app.wait_for_frames(1);

    SeededRng::get().set_seed(test_seed);
    ValidateServerBattleDeterminismTestHelpers::ensure_replay_state_exists(
        test_seed);

    app.create_dish(DishType::Potato)
        .on_team(DishBattleState::TeamSide::Player)
        .at_slot(0)
        .with_combat_stats()
        .commit();

    app.create_dish(DishType::GarlicBread)
        .on_team(DishBattleState::TeamSide::Opponent)
        .at_slot(0)
        .with_combat_stats()
        .commit();

    app.wait_for_frames(1);
    app.wait_for_battle_initialized(10.0f);
    app.wait_for_frames(30);

    app.wait_for_ui_exists("Skip to Results", 5.0f);
    app.click("Skip to Results");
    app.wait_for_results_screen(10.0f);
    app.wait_for_frames(5);

    first_run_result = capture_battle_result();
    app.expect_not_empty(first_run_result.outcomes,
                         "first run should have outcomes");
    app.completed_operations.insert(first_run_op);
    log_info("DETERMINISM_TEST: First run complete - {} outcomes",
             first_run_result.outcomes.size());
    return;
  }

  if (app.completed_operations.count(second_run_op) == 0) {
    app.launch_game();
    app.wait_for_frames(1);
    app.clear_battle_dishes();
    ValidateServerBattleDeterminismTestHelpers::
        ensure_battle_load_request_exists(test_seed);
    app.setup_battle();
    app.wait_for_frames(1);

    SeededRng::get().set_seed(test_seed);
    ValidateServerBattleDeterminismTestHelpers::ensure_replay_state_exists(
        test_seed);

    app.create_dish(DishType::Potato)
        .on_team(DishBattleState::TeamSide::Player)
        .at_slot(0)
        .with_combat_stats()
        .commit();

    app.create_dish(DishType::GarlicBread)
        .on_team(DishBattleState::TeamSide::Opponent)
        .at_slot(0)
        .with_combat_stats()
        .commit();

    app.wait_for_frames(1);
    app.wait_for_battle_initialized(10.0f);
    app.wait_for_frames(30);

    app.wait_for_ui_exists("Skip to Results", 5.0f);
    app.click("Skip to Results");
    app.wait_for_results_screen(10.0f);
    app.wait_for_frames(5);

    BattleRunResult second_run_result = capture_battle_result();
    app.expect_not_empty(second_run_result.outcomes,
                         "second run should have outcomes");

    app.expect_eq(static_cast<int>(first_run_result.outcomes.size()),
                  static_cast<int>(second_run_result.outcomes.size()),
                  "outcome counts should match");

    app.expect_eq(static_cast<int>(first_run_result.outcome),
                  static_cast<int>(second_run_result.outcome),
                  "overall outcomes should match");

    app.expect_eq(first_run_result.playerWins, second_run_result.playerWins,
                  "player wins should match");

    app.expect_eq(first_run_result.opponentWins, second_run_result.opponentWins,
                  "opponent wins should match");

    app.expect_eq(first_run_result.ties, second_run_result.ties,
                  "ties should match");

    for (size_t i = 0; i < first_run_result.outcomes.size(); ++i) {
      app.expect_eq(first_run_result.outcomes[i].slotIndex,
                    second_run_result.outcomes[i].slotIndex,
                    "outcome " + std::to_string(i) + " slotIndex should match");

      app.expect_eq(static_cast<int>(first_run_result.outcomes[i].winner),
                    static_cast<int>(second_run_result.outcomes[i].winner),
                    "outcome " + std::to_string(i) + " winner should match");

      app.expect_eq(first_run_result.outcomes[i].ticks,
                    second_run_result.outcomes[i].ticks,
                    "outcome " + std::to_string(i) + " ticks should match");
    }

    app.completed_operations.insert(second_run_op);
    log_info("DETERMINISM_TEST: ✅ Both runs produced identical results - "
             "server battle determinism verified");
  }
}
