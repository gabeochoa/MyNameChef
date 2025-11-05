#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace server {
struct TeamManager {
  static std::vector<std::string> get_opponent_files();
  static std::string select_random_opponent();
  static nlohmann::json load_team_from_file(const std::string &path);
  static bool validate_team_json(const nlohmann::json &team_json);
  static void track_opponent_file_count();
};
} // namespace server
