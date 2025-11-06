#include "team_manager.h"
#include "../log.h"
#include "../seeded_rng.h"
#include "file_storage.h"
#include <algorithm>
#include <random>

namespace server {
std::vector<TeamFilePath>
TeamManager::get_opponent_files(const std::filesystem::path &opponents_path) {
  return FileStorage::list_files_in_directory(opponents_path.string(), ".json");
}

std::optional<TeamFilePath> TeamManager::select_random_opponent(
    const std::filesystem::path &opponents_path) {
  auto files = get_opponent_files(opponents_path);
  if (files.empty()) {
    log_error("No opponent files found in {}", opponents_path.string());
    return {};
  }

  SeededRng &rng = SeededRng::get();
  size_t index = rng.gen_index(files.size());

  return files[index];
}

std::optional<TeamFilePath> TeamManager::select_random_opponent_with_fallback(
    const std::filesystem::path &opponents_path, int max_attempts) {
  auto files = get_opponent_files(opponents_path);
  if (files.empty()) {
    log_error("No opponent files found in {}", opponents_path.string());
    return {};
  }

  if (files.size() == 1) {
    TeamFilePath candidate = files[0];
    if (!is_path_safe(candidate, opponents_path)) {
      log_error("Opponent file path is not safe: {}", candidate);
      return {};
    }
    return candidate;
  }

  std::vector<size_t> indices;
  for (size_t i = 0; i < files.size(); ++i) {
    indices.push_back(i);
  }

  SeededRng &rng = SeededRng::get();
  std::shuffle(indices.begin(), indices.end(),
               std::mt19937(rng.gen_index(UINT32_MAX)));

  int attempts = std::min(max_attempts, static_cast<int>(files.size()));
  for (int attempt = 0; attempt < attempts; ++attempt) {
    size_t index = indices[attempt];
    TeamFilePath candidate = files[index];

    if (!is_path_safe(candidate, opponents_path)) {
      log_warn(
          "Opponent file path is not safe: {}, trying another (attempt {}/{})",
          candidate, attempt + 1, attempts);
      continue;
    }

    nlohmann::json test_load = load_team_from_file(candidate);
    if (!test_load.empty() && validate_team_json(test_load)) {
      return candidate;
    }

    log_warn(
        "Opponent file {} failed validation, trying another (attempt {}/{})",
        candidate, attempt + 1, attempts);
  }

  log_error("Failed to find valid opponent after {} attempts", max_attempts);
  return {};
}

bool TeamManager::is_path_safe(const std::string &file_path,
                               const std::filesystem::path &allowed_dir) {
  try {
    std::filesystem::path file_path_obj(file_path);
    std::filesystem::path canonical_file =
        std::filesystem::canonical(file_path_obj);
    std::filesystem::path canonical_dir =
        std::filesystem::canonical(allowed_dir);

    std::string file_str = canonical_file.string();
    std::string dir_str = canonical_dir.string();

    return file_str.find(dir_str) == 0;
  } catch (const std::exception &e) {
    log_error("Path safety check failed for {}: {}", file_path, e.what());
    return false;
  }
}

nlohmann::json TeamManager::load_team_from_file(const TeamFilePath &path) {
  return FileStorage::load_json_from_file(path);
}

bool TeamManager::validate_team_json(const nlohmann::json &request_json,
                                     int max_team_size) {
  if (!request_json.contains("team")) {
    return false;
  }

  if (!request_json["team"].is_array()) {
    return false;
  }

  const auto &team = request_json["team"];
  if (team.size() == 0 || team.size() > static_cast<size_t>(max_team_size)) {
    return false;
  }

  for (const auto &dish : team) {
    if (!dish.contains("dishType") || !dish.contains("slot")) {
      return false;
    }

    int slot = dish["slot"];
    if (slot < 0 || slot >= max_team_size) {
      return false;
    }

    if (dish.contains("level")) {
      int level = dish["level"];
      if (level < 1) {
        return false;
      }
    }

    if (dish.contains("powerups") && !dish["powerups"].is_array()) {
      return false;
    }
  }

  return true;
}

void TeamManager::track_opponent_file_count(
    const std::filesystem::path &opponents_path) {
  size_t count =
      FileStorage::count_files_in_directory(opponents_path.string(), ".json");

  if (count > 1000) {
    log_warn("Opponent file count is high: {}. Consider cleanup/archival.",
             count);
  } else {
    log_info("Found {} opponent files in {}", count, opponents_path.string());
  }
}
} // namespace server
