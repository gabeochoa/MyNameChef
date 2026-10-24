#pragma once

#include "components/battle_load_request.h"
#include "components/replay_state.h"
#include "log.h"
#include "seeded_rng.h"
#include "settings.h"
#include <afterhours/ah.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

namespace replay {

inline ReplayState &ensure_replay_state() {
  if (!afterhours::EntityHelper::has_singleton<ReplayState>()) {
    auto &replay_entity = afterhours::EntityHelper::createEntity();
    replay_entity.addComponent<ReplayState>();
    afterhours::EntityHelper::registerSingleton<ReplayState>(replay_entity);
  }
  auto replay_entity = afterhours::EntityHelper::get_singleton<ReplayState>();
  if (!replay_entity.get().has<ReplayState>()) {
    replay_entity.get().addComponent<ReplayState>();
  }
  return replay_entity.get().get<ReplayState>();
}

inline BattleLoadRequest &ensure_battle_load_request() {
  if (!afterhours::EntityHelper::has_singleton<BattleLoadRequest>()) {
    auto &request_entity = afterhours::EntityHelper::createEntity();
    request_entity.addComponent<BattleLoadRequest>();
    afterhours::EntityHelper::registerSingleton<BattleLoadRequest>(
        request_entity);
  }
  auto request_entity =
      afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
  if (!request_entity.get().has<BattleLoadRequest>()) {
    request_entity.get().addComponent<BattleLoadRequest>();
  }
  return request_entity.get().get<BattleLoadRequest>();
}

inline void set_replay_speed(float speed) {
  if (afterhours::EntityHelper::has_singleton<ReplayState>()) {
    auto replay_entity = afterhours::EntityHelper::get_singleton<ReplayState>();
    if (replay_entity.get().has<ReplayState>()) {
      replay_entity.get().get<ReplayState>().timeScale = speed;
    }
  }
  Settings::get().set_battle_speed(speed);
}

inline std::string team_path(bool is_player, uint64_t seed) {
  std::string prefix = is_player ? "output/battles/temp_player_"
                                 : "output/battles/temp_opponent_";
  return prefix + std::to_string(seed) + ".json";
}

inline bool start_battle_replay(uint64_t seed, bool from_history) {
  std::string player_path = team_path(true, seed);
  std::string opponent_path = team_path(false, seed);
  if (!std::filesystem::exists(player_path) ||
      !std::filesystem::exists(opponent_path)) {
    log_warn("REPLAY_START: missing team files for seed {}", seed);
    return false;
  }
  SeededRng::get().set_seed(seed);
  ReplayState &rs = ensure_replay_state();
  rs.seed = seed;
  rs.active = true;
  rs.from_history = from_history;
  rs.is_replay = true;
  rs.paused = false;
  rs.timeScale = 1.0f;
  rs.clockMs = 0;
  rs.targetMs = 0;
  rs.currentFrame = 0;
  rs.totalFrames = 0;
  rs.playerJsonPath = player_path;
  rs.opponentJsonPath = opponent_path;
  BattleLoadRequest &request = ensure_battle_load_request();
  request.playerJsonPath = player_path;
  request.opponentJsonPath = opponent_path;
  request.loaded = false;
  request.serverRequestPending = false;
  return true;
}

inline bool start_replay_from_report_file(const std::string &filepath,
                                           bool from_history) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    log_warn("REPLAY_START: report {} not found", filepath);
    return false;
  }
  nlohmann::json report_json;
  try {
    file >> report_json;
  } catch (const std::exception &) {
    log_warn("REPLAY_START: report {} is not valid json", filepath);
    return false;
  }
  if (!report_json.contains("seed") ||
      !report_json["seed"].is_number_unsigned()) {
    log_warn("REPLAY_START: report {} has no seed", filepath);
    return false;
  }
  return start_battle_replay(report_json["seed"].get<uint64_t>(),
                              from_history);
}

inline std::string find_latest_battle_report() {
  std::string results_dir = "output/battles/results";
  if (!std::filesystem::exists(results_dir)) {
    return "";
  }
  std::string latest;
  std::filesystem::file_time_type latest_time;
  bool found = false;
  for (const auto &entry : std::filesystem::directory_iterator(results_dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".json") {
      continue;
    }
    std::filesystem::file_time_type file_time =
        std::filesystem::last_write_time(entry.path());
    if (!found || file_time > latest_time) {
      latest = entry.path().string();
      latest_time = file_time;
      found = true;
    }
  }
  return latest;
}

inline bool start_replay_from_latest_report(bool from_history) {
  std::string latest = find_latest_battle_report();
  if (latest.empty()) {
    return false;
  }
  return start_replay_from_report_file(latest, from_history);
}

} // namespace replay
