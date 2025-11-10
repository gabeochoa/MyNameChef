#include "../log.h"
#include "../preload.h"
#include "../render_backend.h"
#include "../rl.h"
#include "../shop.h"
#include "battle_api.h"
#include "file_storage.h"
#include "server_config.h"
#include "server_context.h"
#include "test_framework.h"
#include <afterhours/ah.h>
#include <argh.h>
#include <filesystem>
#include <nlohmann/json.hpp>

bool render_backend::is_headless_mode = true;
int render_backend::step_delay_ms = 0;
float render_backend::timing_speed_scale = 1.0f;
bool running = true;

int main(int argc, char *argv[]) {
  argh::parser cmdl(argc, argv);

  if (cmdl[{"--help", "-h"}]) {
    std::cout << "Battle Server - HTTP API server for battle simulation\n\n";
    std::cout << "Usage: battle_server [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "  -t, --run-tests         Run server unit tests\n";
    std::cout << "  --list-tests            List all available tests\n";
    std::cout << "  -c, --config <path>     Load configuration from JSON file\n";
    std::cout << "\n";
    std::cout << "Examples:\n";
    std::cout << "  battle_server --config server_config.json\n";
    std::cout << "  battle_server --run-tests\n";
    return 0;
  }

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
    return server::test::TestRegistry::get().run_all_tests() ? 0 : 1;
  }

  if (cmdl["--list-tests"]) {
    // Output format: one test name per line (for easy parsing)
    auto &registry = server::test::TestRegistry::get();
    auto test_list = registry.list_tests();
    for (const auto &name : test_list) {
      std::cout << name << std::endl;
    }
    return 0;
  }

  server::ServerConfig config = server::ServerConfig::defaults();

  std::string config_path;
  if (cmdl({"-c", "--config"}) >> config_path) {
    config = server::ServerConfig::load_from_json(config_path);
    log_info("Loaded config from: {}", config_path);
  } else {
    log_info("No config file specified, using defaults");
  }

  std::filesystem::path opponents_path = config.get_opponents_path();
  std::filesystem::create_directories(opponents_path);
  server::TeamManager::track_opponent_file_count(opponents_path);

  server::BattleAPI api(config);

  log_info("Battle server starting on port {}", config.port);
  log_info("Headless mode: enabled");
  log_info("Base path: {}", config.base_path);
  log_info("Timeout: {} seconds", config.timeout_seconds);
  log_info("Error detail level: {}", config.error_detail_level);
  log_info("Debug mode: {}", config.debug ? "enabled" : "disabled");

  // Start the HTTP server (blocking call)
  // ECS systems run inside BattleSimulator when battles are active
  api.start(config.port);

  return 0;
}
