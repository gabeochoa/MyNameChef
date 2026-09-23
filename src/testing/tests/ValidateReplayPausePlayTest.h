#pragma once

#include "../../game_state_manager.h"
#include "../test_macros.h"

TEST(validate_replay_pause_play) {
  app.launch_game();
  app.setup_battle();

  app.wait_for_ui_exists("Pause", 5.0f);

  app.once([&] {
    app.expect_false(app.read_replay_paused(),
                     "replay should not be paused initially");
  });

  app.click("Pause");
  app.wait_for_frames(1);

  app.once([&] {
    app.expect_true(app.read_replay_paused(),
                    "replay should be paused after clicking Pause");
  });

  app.wait_for_ui_exists("Play", 5.0f);

  app.click("Play");
  app.wait_for_frames(1);

  app.expect_false(app.read_replay_paused(),
                   "replay should not be paused after clicking Play");
}
