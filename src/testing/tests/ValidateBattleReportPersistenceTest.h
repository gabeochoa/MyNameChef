#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/battle_processor.h"
#include "../../components/battle_result.h"
#include "../../components/replay_state.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../../query.h"
#include "../../seeded_rng.h"
#include "../../server/file_storage.h"
#include "../test_macros.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace ValidateBattleReportPersistenceTestHelpers {
static void ensure_battle_load_request_exists() {
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
    // Update existing ReplayState with seed
    auto replay_entity = afterhours::EntityHelper::get_singleton<ReplayState>();
    if (replay_entity.get().has<ReplayState>()) {
      replay_entity.get().get<ReplayState>().seed = seed;
    }
    return;
  }
  auto &replayEntity = afterhours::EntityHelper::createEntity();
  ReplayState replay;
  replay.seed = seed;
  replay.opponentJsonPath =
      "output/battles/temp_opponent_" + std::to_string(seed) + ".json";
  replay.active = true;
  replayEntity.addComponent<ReplayState>(std::move(replay));
  afterhours::EntityHelper::registerSingleton<ReplayState>(replayEntity);
}
} // namespace ValidateBattleReportPersistenceTestHelpers

TEST(validate_battle_report_persistence) {
  // Use global static flag to prevent test from running multiple times
  static bool test_completed = false;
  static bool test_started = false;
  static const TestOperationID main_test_op =
      TestApp::generate_operation_id(std::source_location::current(),
                                     "validate_battle_report_persistence.main");

  log_info(
      "TEST: validate_battle_report_persistence - Entry: test_completed={}, "
      "test_started={}, test_in_progress={}, main_test_op_completed={}",
      test_completed, test_started, app.test_in_progress,
      app.completed_operations.count(main_test_op) > 0 ? 1 : 0);

  // If test already completed, don't run again
  if (test_completed) {
    log_info("TEST: validate_battle_report_persistence - Exiting early: "
             "test_completed=true");
    return; // Test already completed, exit early
  }

  // Use operation ID to ensure main test logic only runs once
  // Only check, don't insert until test completes - this allows the test to
  // yield and resume properly
  if (app.completed_operations.count(main_test_op) > 0) {
    log_info("TEST: validate_battle_report_persistence - Exiting early: "
             "main_test_op already completed");
    return; // Main test logic already completed, exit early
  }

  // Note: We don't check test_started && test_in_progress here because
  // the test framework calls the function from the beginning each time it
  // resumes. Operations with operation IDs (like launch_game) will skip if
  // already done, allowing the test to continue from where it left off.

  if (!test_started) {
    log_info("TEST: ========== STARTING validate_battle_report_persistence "
             "test ==========");
    test_started = true;
  } else {
    log_info("TEST: validate_battle_report_persistence - Resuming "
             "(test_started=true, test_in_progress={})",
             app.test_in_progress);
  }

  // Clean up any existing battle reports for this test
  std::string results_dir = "output/battles/results";
  if (std::filesystem::exists(results_dir)) {
    // Count existing files before test
    size_t initial_file_count = 0;
    for (const auto &entry : std::filesystem::directory_iterator(results_dir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".json") {
        initial_file_count++;
      }
    }
    log_info("TEST: Found {} existing battle report files", initial_file_count);
  }

  log_info("TEST: Step 1 - Launching game (test_in_progress={})",
           app.test_in_progress);
  app.launch_game();
  log_info("TEST: Step 1 - After launch_game() (test_in_progress={})",
           app.test_in_progress);
  app.wait_for_frames(1);
  log_info("TEST: Step 1 - After wait_for_frames(1) (test_in_progress={})",
           app.test_in_progress);

  // Use operation IDs to prevent these steps from running multiple times
  static const TestOperationID setup_op = TestApp::generate_operation_id(
      std::source_location::current(),
      "validate_battle_report_persistence.setup");
  log_info("TEST: Checking setup_op - completed={}",
           app.completed_operations.count(setup_op) > 0 ? 1 : 0);
  if (app.completed_operations.count(setup_op) == 0) {
    log_info("TEST: Step 2 - Clearing battle dishes (setup_op not completed)");
    app.clear_battle_dishes();

    log_info("TEST: Step 3 - Setting up battle load request");
    ValidateBattleReportPersistenceTestHelpers::
        ensure_battle_load_request_exists();

    log_info("TEST: Step 4 - Setting up battle screen");
    app.setup_battle();
    app.wait_for_frames(1);

    // Set a known seed for testing
    uint64_t test_seed = 12345678901234567890ULL;
    log_info("TEST: Step 5 - Setting seed to {}", test_seed);
    SeededRng::get().set_seed(test_seed);
    ValidateBattleReportPersistenceTestHelpers::ensure_replay_state_exists(
        test_seed);

    // Create a simple battle scenario
    log_info("TEST: Step 6 - Creating player dish");
    afterhours::EntityID player_dish_id =
        app.create_dish(DishType::Potato)
            .on_team(DishBattleState::TeamSide::Player)
            .at_slot(0)
            .with_combat_stats()
            .commit();
    log_info("TEST: Created player dish with ID {}", player_dish_id);

    log_info("TEST: Step 7 - Creating opponent dish");
    afterhours::EntityID opponent_dish_id =
        app.create_dish(DishType::Potato)
            .on_team(DishBattleState::TeamSide::Opponent)
            .at_slot(0)
            .with_combat_stats()
            .commit();
    log_info("TEST: Created opponent dish with ID {}", opponent_dish_id);

    app.completed_operations.insert(setup_op);
    log_info("TEST: Setup completed, marked setup_op as done");
  } else {
    log_info("TEST: Setup already completed (setup_op found), skipping dish "
             "creation");
  }

  // Find dishes by querying - they may have been created in a previous call
  // Set a known seed for testing (needed for BattleLoadRequest paths)
  uint64_t test_seed = 12345678901234567890ULL;

  // Use operation ID to prevent these steps from running multiple times
  static const TestOperationID battle_setup_op = TestApp::generate_operation_id(
      std::source_location::current(),
      "validate_battle_report_persistence.battle_setup");
  if (app.completed_operations.count(battle_setup_op) == 0) {
    app.wait_for_frames(5);

    // Set up BattleLoadRequest with valid paths and mark as loaded
    // This is needed for BattleProcessorSystem to start the battle
    log_info("TEST: Step 8 - Setting up BattleLoadRequest");
    auto request_entity =
        afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    if (request_entity.get().has<BattleLoadRequest>()) {
      BattleLoadRequest &request =
          request_entity.get().get<BattleLoadRequest>();
      request.playerJsonPath =
          "output/battles/temp_player_" + std::to_string(test_seed) + ".json";
      request.opponentJsonPath =
          "output/battles/temp_opponent_" + std::to_string(test_seed) + ".json";
      request.loaded = true;
      log_info("TEST: Set BattleLoadRequest - playerPath='{}', "
               "opponentPath='{}', loaded={}",
               request.playerJsonPath, request.opponentJsonPath,
               request.loaded);
    } else {
      log_error("TEST: ERROR - BattleLoadRequest singleton not found!");
    }

    log_info("TEST: Step 9 - Waiting for battle to initialize");
    app.wait_for_battle_initialized(10.0f);
    log_info("TEST: Battle initialized");

    app.completed_operations.insert(battle_setup_op);
  } else {
    log_info("TEST: Battle setup already completed, skipping");
  }

  // Use operation ID to prevent stats setup from running multiple times
  static const TestOperationID stats_setup_op = TestApp::generate_operation_id(
      std::source_location::current(),
      "validate_battle_report_persistence.stats_setup");
  if (app.completed_operations.count(stats_setup_op) == 0) {
    // Wait a frame for ComputeCombatStatsSystem to run, then set stats
    // ComputeCombatStatsSystem recalculates stats from dish type every frame,
    // so we need to set stats AFTER it has run
    app.wait_for_frames(2);

    // Find dishes by querying - always find them fresh since EntityIDs might
    // change after entity cleanup/merging
    log_info("TEST: Step 10 - Finding dishes to set combat stats");
    afterhours::EntityID player_dish_id = -1;
    afterhours::EntityID opponent_dish_id = -1;

    for (afterhours::Entity &entity : EQ({.force_merge = true})
                                          .whereHasComponent<IsDish>()
                                          .whereHasComponent<DishBattleState>()
                                          .whereInSlotIndex(0)
                                          .gen()) {
      const DishBattleState &dbs = entity.get<DishBattleState>();
      if (dbs.team_side == DishBattleState::TeamSide::Player) {
        player_dish_id = entity.id;
        log_info("TEST: Found player dish with ID {}", player_dish_id);
      } else if (dbs.team_side == DishBattleState::TeamSide::Opponent) {
        opponent_dish_id = entity.id;
        log_info("TEST: Found opponent dish with ID {}", opponent_dish_id);
      }
    }

    if (player_dish_id == -1 || opponent_dish_id == -1) {
      log_error("TEST: ERROR - Could not find dishes! player_id={}, "
                "opponent_id={}. Waiting...",
                player_dish_id, opponent_dish_id);
      // Wait a bit and try again - dishes might not be created yet
      app.wait_for_frames(5);
      return;
    }

    // Set stats so player wins quickly - do this AFTER battle initialization
    // so ComputeCombatStatsSystem doesn't overwrite them immediately
    log_info(
        "TEST: Step 11 - Setting combat stats (player: 30/10, opponent: 3/5)");
    app.set_dish_combat_stats(player_dish_id, 30, 10);
    app.set_dish_combat_stats(opponent_dish_id, 3, 5);

    // Wait another frame to ensure stats are set before combat starts
    app.wait_for_frames(1);

    app.completed_operations.insert(stats_setup_op);
  } else {
    log_info("TEST: Stats setup already completed, skipping");
  }

  log_info("TEST: Checking BattleProcessor state");

  // Check BattleProcessor state
  auto processor_entity =
      afterhours::EntityHelper::get_singleton<BattleProcessor>();
  if (processor_entity.get().has<BattleProcessor>()) {
    const BattleProcessor &processor =
        processor_entity.get().get<BattleProcessor>();
    log_info("TEST: BattleProcessor state - isBattleActive={}, "
             "simulationStarted={}, simulationComplete={}",
             processor.isBattleActive(), processor.simulationStarted,
             processor.simulationComplete);
  }

  // Wait for dishes to enter combat - in visual mode, animations take time
  // Wait longer to ensure combat actually progresses
  log_info(
      "TEST: Step 12 - Waiting 120 frames for animations and initial combat");
  app.wait_for_frames(120); // 2 seconds for animations and initial combat
  log_info("TEST: Finished waiting 120 frames");

  // Check dish counts
  int player_count = app.count_active_player_dishes();
  int opponent_count = app.count_active_opponent_dishes();
  log_info("TEST: Active dishes - Player: {}, Opponent: {}", player_count,
           opponent_count);

  // Wait for battle to complete - give it plenty of time in visual mode
  log_info("TEST: Step 13 - Waiting for battle to complete (timeout: 120s)");
  app.wait_for_battle_complete(120.0f);
  log_info("TEST: Battle completed!");

  // Wait for Results screen
  log_info("TEST: Step 14 - Waiting for Results screen");
  app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
  log_info("TEST: On Results screen");

  // Wait for SaveBattleReportSystem to run (it runs once when Results screen is
  // shown) Give it a few frames to save the file
  log_info("TEST: Step 15 - Waiting 10 frames for SaveBattleReportSystem to "
           "save file");
  app.wait_for_frames(10);
  log_info("TEST: Finished waiting for SaveBattleReportSystem");

  // Verify BattleResult exists
  log_info("TEST: Step 16 - Verifying BattleResult exists");
  const auto componentId = afterhours::components::get_type_id<BattleResult>();
  app.expect_true(
      afterhours::EntityHelper::get().singletonMap.contains(componentId),
      "BattleResult singleton should exist");

  auto result_entity = afterhours::EntityHelper::get_singleton<BattleResult>();
  app.expect_true(result_entity.get().has<BattleResult>(),
                  "BattleResult component should exist");

  // Verify battle report file was created
  std::filesystem::path results_path(results_dir);
  std::filesystem::create_directories(results_path);

  // Find the most recently created battle report file
  std::string latest_report_file;
  std::filesystem::file_time_type latest_time;
  bool found_file = false;

  for (const auto &entry : std::filesystem::directory_iterator(results_path)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      auto file_time = std::filesystem::last_write_time(entry.path());
      if (!found_file || file_time > latest_time) {
        latest_time = file_time;
        latest_report_file = entry.path().string();
        found_file = true;
      }
    }
  }

  app.expect_true(found_file, "Battle report file should be created");

  if (found_file) {
    log_info("TEST: Found battle report file: {}", latest_report_file);

    // Verify file exists and is readable
    app.expect_true(server::FileStorage::file_exists(latest_report_file),
                    "Battle report file should exist");

    // Load and verify JSON structure
    nlohmann::json report_json =
        server::FileStorage::load_json_from_file(latest_report_file);

    // Verify required fields exist
    app.expect_true(report_json.contains("seed"),
                    "Battle report should contain 'seed' field");
    app.expect_true(report_json.contains("opponentId"),
                    "Battle report should contain 'opponentId' field");
    app.expect_true(report_json.contains("outcomes"),
                    "Battle report should contain 'outcomes' field");
    app.expect_true(report_json.contains("events"),
                    "Battle report should contain 'events' field");

    // Verify seed matches
    uint64_t saved_seed = report_json["seed"].get<uint64_t>();
    app.expect_eq(saved_seed, test_seed,
                  "Battle report seed should match test seed");

    // Verify outcomes is an array
    app.expect_true(report_json["outcomes"].is_array(),
                    "Battle report 'outcomes' should be an array");

    // Verify events is an array
    app.expect_true(report_json["events"].is_array(),
                    "Battle report 'events' should be an array");

    // Verify filename format (should contain timestamp and seed)
    std::string filename =
        std::filesystem::path(latest_report_file).filename().string();
    log_info("TEST: Battle report filename: {}", filename);

    // Filename should match pattern: YYYYMMDD_HHMMSS_<seed>.json
    // Check that it contains the seed
    std::string seed_str = std::to_string(test_seed);
    app.expect_true(filename.find(seed_str) != std::string::npos,
                    "Battle report filename should contain seed");

    // Verify outcomes structure if any exist
    if (report_json["outcomes"].size() > 0) {
      auto &first_outcome = report_json["outcomes"][0];
      app.expect_true(first_outcome.contains("slotIndex"),
                      "Outcome should contain 'slotIndex'");
      app.expect_true(first_outcome.contains("ticks"),
                      "Outcome should contain 'ticks'");
      app.expect_true(first_outcome.contains("winner"),
                      "Outcome should contain 'winner'");
    }

    log_info("TEST: Battle report JSON structure verified successfully");
  }

  log_info("TEST: ========== validate_battle_report_persistence test COMPLETED "
           "SUCCESSFULLY ==========");

  // Mark main test logic as completed to prevent re-running
  app.completed_operations.insert(main_test_op);

  // Mark test as completed to prevent re-running (use same static variable from
  // start)
  test_completed = true;
}

TEST(validate_battle_report_file_retention) {
  log_info("TEST: Starting validate_battle_report_file_retention test");

  std::string results_dir = "output/battles/results";
  std::filesystem::create_directories(results_dir);

  // Count initial files
  size_t initial_count = 0;
  for (const auto &entry : std::filesystem::directory_iterator(results_dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      initial_count++;
    }
  }

  log_info("TEST: Initial file count: {}", initial_count);

  // Create a battle report to trigger retention logic
  app.launch_game();
  app.wait_for_frames(1);

  app.clear_battle_dishes();

  ValidateBattleReportPersistenceTestHelpers::
      ensure_battle_load_request_exists();
  app.setup_battle();
  app.wait_for_frames(1);

  uint64_t test_seed = 9876543210ULL;
  SeededRng::get().set_seed(test_seed);
  ValidateBattleReportPersistenceTestHelpers::ensure_replay_state_exists(
      test_seed);

  afterhours::EntityID player_dish_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Player)
          .at_slot(0)
          .with_combat_stats()
          .commit();

  afterhours::EntityID opponent_dish_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Opponent)
          .at_slot(0)
          .with_combat_stats()
          .commit();

  app.wait_for_frames(5);

  app.set_dish_combat_stats(player_dish_id, 30, 10);
  app.set_dish_combat_stats(opponent_dish_id, 3, 5);

  app.wait_for_battle_initialized(10.0f);
  app.wait_for_frames(30);

  app.wait_for_frames(1);
  GameStateManager::Screen current_screen =
      GameStateManager::get().active_screen;
  if (current_screen != GameStateManager::Screen::Results) {
    app.wait_for_ui_exists("Skip to Results", 10.0f);
    app.click("Skip to Results");
  }

  app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
  app.wait_for_frames(
      30); // Give SaveBattleReportSystem time to run and apply retention

  // Count files after save
  size_t final_count = 0;
  for (const auto &entry : std::filesystem::directory_iterator(results_dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      final_count++;
    }
  }

  log_info("TEST: Final file count: {}", final_count);

  // Verify that file retention is working (should not exceed 20 files)
  // Note: This test verifies the retention logic runs, but doesn't guarantee
  // exactly 20 files since other tests may have created files
  app.expect_true(final_count <= 20,
                  "File retention should keep at most 20 files");

  log_info("TEST: File retention verified ({} files, max 20)", final_count);
}
