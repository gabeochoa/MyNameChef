#pragma once

#include <afterhours/ah.h>
#include <chrono>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

struct BattleReport : afterhours::BaseComponent {
  std::string opponentId;
  uint64_t seed = 0;
  std::vector<nlohmann::json> outcomes; // Course outcomes as JSON
  std::vector<nlohmann::json> events;    // Battle events as JSON
  bool receivedFromServer = false;
  std::chrono::system_clock::time_point timestamp;
  std::string filePath; // Path to saved JSON file

  BattleReport() : timestamp(std::chrono::system_clock::now()) {}

  /**
   * Serialize BattleReport to JSON string matching server format
   */
  std::string to_json_string() const {
    nlohmann::json j;
    j["seed"] = seed;
    j["opponentId"] = opponentId;
    j["outcomes"] = outcomes;
    j["events"] = events;
    return j.dump(2); // Pretty print with 2-space indent
  }

  /**
   * Deserialize BattleReport from JSON string
   */
  void from_json_string(const std::string &json_str) {
    nlohmann::json j = nlohmann::json::parse(json_str);
    if (j.contains("seed")) {
      seed = j["seed"].get<uint64_t>();
    }
    if (j.contains("opponentId")) {
      opponentId = j["opponentId"].get<std::string>();
    }
    if (j.contains("outcomes") && j["outcomes"].is_array()) {
      outcomes = j["outcomes"].get<std::vector<nlohmann::json>>();
    }
    if (j.contains("events") && j["events"].is_array()) {
      events = j["events"].get<std::vector<nlohmann::json>>();
    }
  }

  /**
   * Generate filename in format: YYYYMMDD_HHMMSS_<battle_id>.json
   * Uses seed as battle_id
   */
  std::string get_filename() const {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    std::string timestamp_str = ss.str();
    std::string battle_id = std::to_string(seed);
    return timestamp_str + "_" + battle_id + ".json";
  }
};

