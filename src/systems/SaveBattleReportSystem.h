#pragma once

#include "../components/battle_load_request.h"
#include "../components/battle_report.h"
#include "../components/battle_result.h"
#include "../components/replay_state.h"
#include "../game_state_manager.h"
#include "../log.h"
#include "../seeded_rng.h"
#include "../server/file_storage.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <fstream>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

struct SaveBattleReportSystem : afterhours::System<> {
  bool saved = false;
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();

    // Reset saved flag when leaving results screen
    if (last_screen == GameStateManager::Screen::Results &&
        gsm.active_screen != GameStateManager::Screen::Results) {
      saved = false;
    }

    last_screen = gsm.active_screen;
    return gsm.active_screen == GameStateManager::Screen::Results && !saved;
  }

  void once(float) override {
    // Check if BattleResult singleton exists
    const auto componentId =
        afterhours::components::get_type_id<BattleResult>();
    if (!afterhours::EntityHelper::get().singletonMap.contains(componentId)) {
      log_warn("SaveBattleReport: BattleResult singleton not found");
      saved = true; // Mark as saved to avoid repeated attempts
      return;
    }

    auto result_entity =
        afterhours::EntityHelper::get_singleton<BattleResult>();
    if (!result_entity.get().has<BattleResult>()) {
      log_warn("SaveBattleReport: BattleResult component not found");
      saved = true;
      return;
    }

    const BattleResult &result = result_entity.get().get<BattleResult>();

    // Get seed from ReplayState or SeededRng
    uint64_t seed = 0;
    std::string opponent_id = "local_opponent";

    // Try to get seed from ReplayState
    const auto replay_component_id =
        afterhours::components::get_type_id<ReplayState>();
    if (afterhours::EntityHelper::get().singletonMap.contains(
            replay_component_id)) {
      auto replay_entity =
          afterhours::EntityHelper::get_singleton<ReplayState>();
      if (replay_entity.get().has<ReplayState>()) {
        const ReplayState &replay = replay_entity.get().get<ReplayState>();
        seed = replay.seed;
        // Try to extract opponent ID from opponentJsonPath
        if (!replay.opponentJsonPath.empty()) {
          std::filesystem::path opp_path(replay.opponentJsonPath);
          std::string filename = opp_path.filename().string();
          // Extract opponent ID from filename (e.g.,
          // "temp_opponent_12345.json")
          if (filename.find("opponent_") != std::string::npos) {
            size_t start = filename.find("opponent_") + 9;
            size_t end = filename.find(".json");
            if (end != std::string::npos) {
              opponent_id = filename.substr(start, end - start);
            }
          }
        }
      }
    }

    // Fallback to SeededRng seed if ReplayState not available
    if (seed == 0) {
      seed = SeededRng::get().seed;
      log_info("SAVE_BATTLE_REPORT: Using SeededRng seed: {}", seed);
    } else {
      log_info("SAVE_BATTLE_REPORT: Using ReplayState seed: {}", seed);
    }

    // Create BattleReport
    BattleReport report;
    report.seed = seed;
    report.opponentId = opponent_id;
    log_info("SAVE_BATTLE_REPORT: Saving report with seed: {}, opponentId: {}",
             report.seed, report.opponentId);
    report.receivedFromServer = false;
    report.timestamp = std::chrono::system_clock::now();

    // Convert BattleResult outcomes to JSON
    for (const auto &outcome : result.outcomes) {
      nlohmann::json outcome_json;
      outcome_json["slotIndex"] = outcome.slotIndex;
      outcome_json["ticks"] = outcome.ticks;
      std::string winner_str;
      switch (outcome.winner) {
      case BattleResult::CourseOutcome::Winner::Player:
        winner_str = "Player";
        break;
      case BattleResult::CourseOutcome::Winner::Opponent:
        winner_str = "Opponent";
        break;
      case BattleResult::CourseOutcome::Winner::Tie:
        winner_str = "Tie";
        break;
      }
      outcome_json["winner"] = winner_str;
      report.outcomes.push_back(outcome_json);
    }

    // Events array is empty for now (client-side doesn't collect events)
    report.events = std::vector<nlohmann::json>();

    // Save to file
    std::string results_dir = "output/battles/results";
    server::FileStorage::ensure_directory_exists(results_dir);

    std::string filename = report.get_filename();
    std::string filepath =
        (std::filesystem::path(results_dir) / filename).string();

    // Serialize to JSON
    nlohmann::json report_json;
    report_json["seed"] = report.seed;
    report_json["opponentId"] = report.opponentId;
    report_json["outcomes"] = report.outcomes;
    report_json["events"] = report.events;

    if (!server::FileStorage::save_json_to_file(filepath, report_json)) {
      log_error("SaveBattleReport: Failed to save battle report to {}",
                filepath);
      saved = true; // Mark as saved to avoid repeated attempts
      return;
    }

    report.filePath = filepath;
    log_info("SaveBattleReport: Saved battle report to {}", filepath);

    // Apply file retention (keep last 20 files)
    server::FileStorage::cleanup_old_files(results_dir, 20, ".json");

    saved = true;
  }
};
