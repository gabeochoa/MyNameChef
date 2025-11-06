#include "../log.h"
#include "../preload.h"
#include "../render_backend.h"
#include "../rl.h"
#include "../shop.h"
#include "battle_api.h"
#include "file_storage.h"
#include "test_framework.h"
#include <afterhours/ah.h>
#include <argh.h>
#include <filesystem>
#include <nlohmann/json.hpp>

bool render_backend::is_headless_mode = true;
int render_backend::step_delay_ms = 0;
bool running = true;

int main(int argc, char *argv[]) {
  argh::parser cmdl(argc, argv);

  // Initialize Preload once for the entire server process
  // This must be done before creating ServerContext instances (tests or API)
  Preload::get().init("battle_server", true).make_singleton();

  // Initialize combat and battle processor managers once for the entire server
  // process These are shared singletons that don't need to be recreated per
  // battle
  auto &manager_entity = afterhours::EntityHelper::createEntity();
  make_combat_manager(manager_entity);
  make_battle_processor_manager(manager_entity);

  if (cmdl[{"--run-tests", "-t"}]) {
    log_info("Running server unit tests...");
    bool all_passed = server::test::TestRegistry::get().run_all_tests();
    return all_passed ? 0 : 1;
  }

  int port = 8080;
  cmdl({"-p", "--port"}, 8080) >> port;

  std::string config_path;
  if (cmdl({"-c", "--config"}) >> config_path) {
    if (server::FileStorage::file_exists(config_path)) {
      nlohmann::json config =
          server::FileStorage::load_json_from_file(config_path);

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
