#pragma once

#include "../../components/battle_load_request.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../test_app.h"
#include "../test_macros.h"
#include "../test_server_helpers.h"
#include <afterhours/ah.h>

TEST(validate_server_failure_during_pending_request) {
  app.delete_save_file();
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

  app.run_once("setup", [&app]() {
    test_server_helpers::server_integration_test_setup("PENDING_REQUEST_TEST");
    app.create_inventory_item(DishType::Potato, 0);
  });
  app.wait_for_frames(5);

  app.run_once("kill_server", [&app]() { app.kill_server(); });

  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_frames(1);

  app.run_once("request_attempted", [&app]() {
    afterhours::RefEntity request_ref =
        afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    app.expect_true(request_ref.get().has<BattleLoadRequest>(),
                    "BattleLoadRequest exists");
    const BattleLoadRequest &req = request_ref.get().get<BattleLoadRequest>();
    app.expect_true(req.serverRequestPending || !req.playerJsonPath.empty() ||
                        !req.opponentJsonPath.empty(),
                    "server request attempted");
    app.force_network_check();
  });
  app.wait_for_frames(30);

  app.run_once("still_in_battle_flow", [&app]() {
    GameStateManager::Screen screen = app.read_current_screen();
    app.expect_true(screen == GameStateManager::Screen::Battle ||
                        screen == GameStateManager::Screen::Results,
                    "screen stays in battle flow after server failure");
  });

  log_info("PENDING_REQUEST_TEST: stayed in battle flow after server failure");
}
