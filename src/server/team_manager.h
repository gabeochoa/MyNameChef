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
  static std::optional<TeamFilePath> select_random_opponent_with_fallback(
      const std::filesystem::path &opponents_path, int max_attempts = 5);
  static nlohmann::json load_team_from_file(const TeamFilePath &path);
  static bool validate_team_json(const nlohmann::json &request_json,
                                 int max_team_size = 7);
  static void
  track_opponent_file_count(const std::filesystem::path &opponents_path);

private:
  static bool is_path_safe(const std::string &file_path,
                           const std::filesystem::path &allowed_dir);
};
} // namespace server
