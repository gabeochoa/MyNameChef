#pragma once

#include "team_types.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

namespace server {
struct TeamManager {
  static std::vector<TeamFilePath> get_opponent_files();
  static std::optional<TeamFilePath> select_random_opponent();
  static nlohmann::json load_team_from_file(const TeamFilePath &path);
  static bool validate_team_json(const nlohmann::json &request_json);
  static void track_opponent_file_count();
};
} // namespace server
