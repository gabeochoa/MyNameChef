#pragma once

#include "../components/battle_history.h"
#include "../game_state_manager.h"
#include "../log.h"
#include "../server/file_storage.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <nlohmann/json.hpp>

struct LoadBattleHistorySystem : afterhours::System<> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::History;
  }

  void once(float) override {
    // Check if BattleHistory singleton exists
    const auto componentId =
        afterhours::components::get_type_id<BattleHistory>();
    auto &singletonMap = afterhours::EntityHelper::get().singletonMap;

    if (singletonMap.contains(componentId)) {
      auto history_entity =
          afterhours::EntityHelper::get_singleton<BattleHistory>();
      if (history_entity.get().has<BattleHistory>()) {
        BattleHistory &history = history_entity.get().get<BattleHistory>();
        if (history.loaded) {
          return; // Already loaded
        }
      }
    }

    // Load battle history
    std::string results_dir = "output/battles/results";
    if (!std::filesystem::exists(results_dir)) {
      log_info("LoadBattleHistory: Results directory does not exist");
      return;
    }

    std::vector<BattleHistoryEntry> entries;

    // Collect all JSON files
    std::vector<std::pair<std::filesystem::path, std::filesystem::file_time_type>>
        files_with_times;

    for (const auto &entry : std::filesystem::directory_iterator(results_dir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".json") {
        files_with_times.push_back(
            {entry.path(), std::filesystem::last_write_time(entry.path())});
      }
    }

    // Sort by modification time (newest first)
    std::sort(files_with_times.begin(), files_with_times.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    // Load up to 20 most recent files
    size_t count = 0;
    for (const auto &[filepath, file_time] : files_with_times) {
      if (count >= 20) {
        break;
      }

      try {
        nlohmann::json report_json =
            server::FileStorage::load_json_from_file(filepath.string());

        BattleHistoryEntry entry;
        entry.filePath = filepath.string();
        entry.filename = filepath.filename().string();

        if (report_json.contains("seed")) {
          entry.seed = report_json["seed"].get<uint64_t>();
        }
        if (report_json.contains("opponentId")) {
          entry.opponentId = report_json["opponentId"].get<std::string>();
        }

        // Parse timestamp from filename (YYYYMMDD_HHMMSS_<seed>.json)
        std::string filename = entry.filename;
        if (filename.length() >= 15) {
          // Use file modification time as timestamp
          // Convert file_time_type to system_clock::time_point
          auto file_time_since_epoch = file_time.time_since_epoch();
          entry.timestamp = std::chrono::system_clock::time_point(
              std::chrono::duration_cast<std::chrono::system_clock::duration>(
                  file_time_since_epoch));
        } else {
          entry.timestamp = std::chrono::system_clock::now();
        }

        // Count wins from outcomes
        if (report_json.contains("outcomes") &&
            report_json["outcomes"].is_array()) {
          for (const auto &outcome : report_json["outcomes"]) {
            if (outcome.contains("winner")) {
              std::string winner = outcome["winner"].get<std::string>();
              if (winner == "Player") {
                entry.playerWins++;
              } else if (winner == "Opponent") {
                entry.opponentWins++;
              } else if (winner == "Tie") {
                entry.ties++;
              }
            }
          }
        }

        entries.push_back(entry);
        count++;
      } catch (const std::exception &e) {
        log_error("LoadBattleHistory: Failed to load {}: {}", filepath.string(),
                  e.what());
      }
    }

    // Create or update BattleHistory singleton
    if (!singletonMap.contains(componentId)) {
      auto &history_entity = afterhours::EntityHelper::createEntity();
      BattleHistory history;
      history.entries = std::move(entries);
      history.loaded = true;
      history_entity.addComponent<BattleHistory>(std::move(history));
      afterhours::EntityHelper::registerSingleton<BattleHistory>(history_entity);
    } else {
      auto history_entity =
          afterhours::EntityHelper::get_singleton<BattleHistory>();
      if (history_entity.get().has<BattleHistory>()) {
        BattleHistory &history = history_entity.get().get<BattleHistory>();
        history.entries = std::move(entries);
        history.loaded = true;
      }
    }

    log_info("LoadBattleHistory: Loaded {} battle reports", entries.size());
  }
};

