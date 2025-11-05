#include "../log.h"
#include "battle_api.h"
#include "file_storage.h"
#include <argh.h>
#include <filesystem>
#include <nlohmann/json.hpp>

int main(int argc, char *argv[]) {
  argh::parser cmdl(argc, argv);

  int port = 8080;
  cmdl({"-p", "--port"}, 8080) >> port;

  std::string config_path;
  if (cmdl({"-c", "--config"}) >> config_path) {
    if (FileStorage::file_exists(config_path)) {
      nlohmann::json config = FileStorage::load_json_from_file(config_path);

      if (!config.empty() && config.contains("port")) {
        port = config["port"];
      }
    } else {
      log_warn("Config file not found: {}", config_path);
    }
  }

  server::TeamManager::track_opponent_file_count();

  server::BattleAPI api;

  log_info("Battle server starting on port {}", port);
  log_info("Headless mode: enabled");

  api.start(port);

  return 0;
}
