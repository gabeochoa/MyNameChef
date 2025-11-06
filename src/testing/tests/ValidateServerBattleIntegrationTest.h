#pragma once

#include "../../components/battle_load_request.h"
#include "../../game_state_manager.h"
#include "../test_app.h"
#include "../test_macros.h"
#include <afterhours/ah.h>
#include <cstdlib>

TEST(validate_server_battle_integration) {
  static enum {
    NAVIGATE_TO_SHOP,
    SETUP_BATTLE_REQUEST,
    NAVIGATE_TO_BATTLE,
    WAIT_FOR_BATTLE_COMPLETE,
    VERIFY_RESULTS
  } phase = NAVIGATE_TO_SHOP;

  app.launch_game();

  // Step 1: Navigate to shop screen
  if (phase == NAVIGATE_TO_SHOP) {
    log_info("INTEGRATION_TEST: Navigating to shop screen...");
    app.navigate_to_shop();
    app.wait_for_ui_exists("Next Round");
    phase = SETUP_BATTLE_REQUEST;
    return;
  }

  // Step 2: Set up BattleLoadRequest with server URL
  if (phase == SETUP_BATTLE_REQUEST) {
    log_info(
        "INTEGRATION_TEST: Setting up BattleLoadRequest with server URL...");

    const char *server_url_env = std::getenv("INTEGRATION_SERVER_URL");
    std::string server_url =
        server_url_env ? server_url_env : "http://localhost:8080";

    log_info("INTEGRATION_TEST: Using server URL: {}", server_url);

    auto battle_request_opt =
        afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    afterhours::Entity &request_entity = battle_request_opt.get();

    if (!request_entity.has<BattleLoadRequest>()) {
      request_entity.addComponent<BattleLoadRequest>();
    }

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
    phase = NAVIGATE_TO_BATTLE;
    return;
  }

  // Step 3: Navigate to battle screen
  if (phase == NAVIGATE_TO_BATTLE) {
    log_info("INTEGRATION_TEST: Navigating to battle screen...");
    app.click("Next Round");
    app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
    app.wait_for_ui_exists("Skip to Results", 5.0f);

    log_info("INTEGRATION_TEST: Waiting for battle to initialize...");
    app.wait_for_frames(10);
    phase = WAIT_FOR_BATTLE_COMPLETE;
    return;
  }

  // Step 4: Wait for battle to complete
  if (phase == WAIT_FOR_BATTLE_COMPLETE) {
    log_info("INTEGRATION_TEST: Battle started, waiting for completion...");
    app.wait_for_battle_complete(60.0f);
    app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
    phase = VERIFY_RESULTS;
    return;
  }

  // Step 5: Verify results
  if (phase == VERIFY_RESULTS) {
    log_info("INTEGRATION_TEST: Battle completed successfully!");

    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Results) {
      app.fail("Expected to be on Results screen after battle");
    }

    log_info("INTEGRATION_TEST: ✅ Test passed - battle completed and results "
             "screen shown");
  }
}
