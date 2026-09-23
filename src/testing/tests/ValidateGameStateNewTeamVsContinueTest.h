#pragma once

#include "../../game_state_manager.h"
#include "../../shop.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

TEST(validate_game_state_new_team_vs_continue) {
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_frames(20);

  app.once([&] {
    app.set_wallet_gold(150);
    app.set_test_int("saved_round", app.read_round());
  });

  app.trigger_game_state_save();
  app.wait_for_frames(5);
  app.once([&] {
    app.expect_true(app.save_file_exists(), "save file should exist");
  });

  app.once([&] {
    GameStateManager::get().set_next_screen(GameStateManager::Screen::Main);
  });
  app.wait_for_screen(GameStateManager::Screen::Main, 5.0f);
  app.wait_for_frames(10);

  app.wait_for_ui_exists("New Team", 5.0f);
  app.wait_for_ui_exists("Continue", 5.0f);

  app.click("New Team");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_frames(10);

  app.once([&] {
    app.expect_false(app.save_file_exists(),
                     "save file should be deleted after New Team");
    app.expect_eq(app.read_round(), 1, "round should be reset to 1");
  });

  app.once([&] { app.set_wallet_gold(200); });
  app.trigger_game_state_save();
  app.wait_for_frames(5);
  app.once([&] {
    app.expect_true(app.save_file_exists(), "save file should exist again");
  });

  app.once([&] {
    GameStateManager::get().set_next_screen(GameStateManager::Screen::Main);
  });
  app.wait_for_screen(GameStateManager::Screen::Main, 5.0f);
  app.wait_for_frames(10);

  app.once([&] {
    app.set_wallet_gold(0);
    afterhours::Entity &round_entity =
        afterhours::EntityHelper::get_singleton<Round>().get();
    if (round_entity.has<Round>()) {
      round_entity.get<Round>().current = 1;
    }
  });

  app.wait_for_ui_exists("Continue", 5.0f);
  app.click("Continue");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_frames(20);

  app.expect_wallet_has(200, "gold should be restored from save");
  app.expect_eq(app.read_round(), app.get_test_int("saved_round").value(),
                "round should be restored from save");
}
