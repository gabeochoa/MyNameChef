#include "team_manager.h"
#include "../log.h"
#include "../seeded_rng.h"
#include "file_storage.h"
#include <algorithm>

namespace server {
std::vector<TeamFilePath> TeamManager::get_opponent_files() {
  std::string opponents_dir = "resources/battles/opponents/";
  return FileStorage::list_files_in_directory(opponents_dir, ".json");
}

std::optional<TeamFilePath> TeamManager::select_random_opponent() {
  auto files = get_opponent_files();
  if (files.empty()) {
    log_error("No opponent files found in resources/battles/opponents/");
    return {};
  }

  SeededRng &rng = SeededRng::get();
  size_t index = rng.gen_index(files.size());

  return files[index];
}

nlohmann::json TeamManager::load_team_from_file(const TeamFilePath &path) {
  return FileStorage::load_json_from_file(path);
}

bool TeamManager::validate_team_json(const nlohmann::json &request_json) {
  if (!request_json.contains("team")) {
    return false;
  }

  if (!request_json["team"].is_array()) {
    return false;
  }

  const auto &team = request_json["team"];
  if (team.size() == 0 || team.size() > 7) {
    return false;
  }

  for (const auto &dish : team) {
    if (!dish.contains("dishType") || !dish.contains("slot")) {
      return false;
    }

    int slot = dish["slot"];
    if (slot < 0 || slot >= 7) {
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

void TeamManager::track_opponent_file_count() {
  std::string opponents_dir = "resources/battles/opponents/";
  size_t count = FileStorage::count_files_in_directory(opponents_dir, ".json");

  if (count > 1000) {
    log_warn("Opponent file count is high: {}. Consider cleanup/archival.",
             count);
  } else {
    log_info("Found {} opponent files", count);
  }
}
} // namespace server
