#pragma once

#include "../../components/battle_load_request.h"
#include "../../game_state_manager.h"
#include "../test_app.h"
#include "../test_macros.h"
#include <afterhours/ah.h>
#include <cstdlib>
#include <string>

namespace {
std::string get_server_url() {
  const char *env = std::getenv("INTEGRATION_SERVER_URL");
  return env ? std::string(env) : std::string("http://localhost:8080");
}
} // namespace

TEST(validate_server_battle_integration) {
  static bool setup_complete = false;

  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

  // Setup BattleLoadRequest with server URL (only once)
  setup_complete = setup_complete || []() {
    log_info(
        "INTEGRATION_TEST: Setting up BattleLoadRequest with server URL...");

    std::string server_url = get_server_url();
    log_info("INTEGRATION_TEST: Using server URL: {}", server_url);

    // Always create new entity and register as singleton (will replace
    // existing)
    afterhours::Entity &request_entity =
        afterhours::EntityHelper::createEntity();
    request_entity.addComponent<BattleLoadRequest>();
    BattleLoadRequest &request = request_entity.get<BattleLoadRequest>();

    request.serverUrl = server_url;
    request.playerJsonPath = "";
    request.opponentJsonPath = "";
    request.loaded = false;
    request.serverRequestPending = false;

    afterhours::EntityHelper::registerSingleton<BattleLoadRequest>(
        request_entity);
    afterhours::EntityHelper::merge_entity_arrays();

    log_info("INTEGRATION_TEST: BattleLoadRequest configured with server URL");
    return true;
  }();

  app.wait_for_frames(1);

  // Navigate to battle
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);

  // Wait for server request to complete - files must be set
  app.wait_for_frames(60); // Give server time to respond
  auto request_opt =
      afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
  app.expect_singleton_has_component<BattleLoadRequest>(request_opt,
                                                        "BattleLoadRequest");

  const BattleLoadRequest &req = request_opt.get().get<BattleLoadRequest>();
  app.expect_false(req.playerJsonPath.empty(),
                   "playerJsonPath should be set by server");
  app.expect_false(req.opponentJsonPath.empty(),
                   "opponentJsonPath should be set by server");

  log_info("INTEGRATION_TEST: Server request completed, files ready");
  log_info("  Player file: {}", req.playerJsonPath);
  log_info("  Opponent file: {}", req.opponentJsonPath);

  // Wait for battle to initialize and dishes to enter combat
  app.wait_for_battle_initialized(30.0f);
  app.wait_for_dishes_in_combat(1, 30.0f);

  app.wait_for_ui_exists("Skip to Results", 5.0f);

  // Wait 5 seconds to see the battle in action, then skip to results
  log_info("INTEGRATION_TEST: Waiting 5 seconds to watch battle...");
  app.wait_for_frames(300); // 5 seconds at 60fps

  log_info("INTEGRATION_TEST: Clicking 'Skip to Results' to finish battle...");
  app.click("Skip to Results");
  app.wait_for_results_screen(10.0f);

  log_info("INTEGRATION_TEST: ✅ Test passed - battle completed and results "
           "screen shown");
}
