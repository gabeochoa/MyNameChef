#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/battle_result.h"
#include "../../components/replay_state.h"
#include "../../game_state_manager.h"
#include "../../log.h"
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
  log_info("TEST: Starting validate_battle_report_persistence test");

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

  app.launch_game();
  app.wait_for_frames(1);

  app.clear_battle_dishes();

  ValidateBattleReportPersistenceTestHelpers::
      ensure_battle_load_request_exists();
  app.setup_battle();
  app.wait_for_frames(1);

  // Set a known seed for testing
  uint64_t test_seed = 12345678901234567890ULL;
  SeededRng::get().set_seed(test_seed);
  ValidateBattleReportPersistenceTestHelpers::ensure_replay_state_exists(
      test_seed);

  // Create a simple battle scenario
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

  // Set stats so player wins quickly
  app.set_dish_combat_stats(player_dish_id, 30, 10);
  app.set_dish_combat_stats(opponent_dish_id, 3, 5);

  app.wait_for_battle_initialized(10.0f);
  app.wait_for_frames(30);

  // Wait for battle to complete
  app.wait_for_battle_complete(60.0f);

  // Wait for Results screen
  app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
  app.wait_for_frames(20); // Give SaveBattleReportSystem time to run

  // Verify BattleResult exists
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

  app.wait_for_battle_complete(60.0f);

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
