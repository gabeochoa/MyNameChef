#pragma once

#include "team_types.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

namespace server {
struct TeamManager {
  static std::vector<TeamFilePath>
  get_opponent_files(const std::filesystem::path &opponents_path);
  static std::optional<TeamFilePath>
  select_random_opponent(const std::filesystem::path &opponents_path);
  static nlohmann::json load_team_from_file(const TeamFilePath &path);
  static bool validate_team_json(const nlohmann::json &request_json);
  static void
  track_opponent_file_count(const std::filesystem::path &opponents_path);
};
} // namespace server
