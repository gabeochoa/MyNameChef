#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/replay_state.h"
#include "../../game_state_manager.h"
#include "../../replay.h"
#include "../test_macros.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

TEST(validate_replay_speed_controls) {
  app.launch_game();
  app.setup_battle();

  app.wait_for_ui_exists("0.5x", 5.0f);
  app.click("0.5x");
  app.wait_for_frames(1);
  app.once([&] {
    auto replay_ref = afterhours::EntityHelper::get_singleton<ReplayState>();
    app.expect_true(replay_ref.get().has<ReplayState>(),
                    "ReplayState exists after clicking 0.5x");
    app.expect_eq(replay_ref.get().get<ReplayState>().timeScale, 0.5f,
                  "0.5x button sets timeScale to 0.5");
  });

  app.click("1x");
  app.wait_for_frames(1);
  app.once([&] {
    auto replay_ref = afterhours::EntityHelper::get_singleton<ReplayState>();
    app.expect_eq(replay_ref.get().get<ReplayState>().timeScale, 1.0f,
                  "1x button sets timeScale to 1.0");
  });

  app.click("2x");
  app.wait_for_frames(1);
  app.once([&] {
    auto replay_ref = afterhours::EntityHelper::get_singleton<ReplayState>();
    app.expect_eq(replay_ref.get().get<ReplayState>().timeScale, 2.0f,
                  "2x button sets timeScale to 2.0");
  });

  app.click("4x");
  app.wait_for_frames(1);
  app.once([&] {
    auto replay_ref = afterhours::EntityHelper::get_singleton<ReplayState>();
    app.expect_eq(replay_ref.get().get<ReplayState>().timeScale, 4.0f,
                  "4x button sets timeScale to 4.0");
  });
}

TEST(validate_replay_watch_again) {
  app.launch_game();
  app.setup_battle();

  app.wait_for_ui_exists("Skip to Results", 10.0f);
  app.click("Skip to Results");
  app.wait_for_screen(GameStateManager::Screen::Results, 10.0f);
  app.wait_for_frames(2);

  app.once([&] {
    std::filesystem::create_directories("output/battles/results");
    std::filesystem::create_directories("output/battles");
    std::ofstream report("output/battles/results/29990101_000000_424242.json");
    report << "{\"seed\":424242,\"opponentId\":\"watch_again\",\"outcomes\":["
              "{\"slotIndex\":0,\"ticks\":1,\"winner\":\"Player\"}],"
              "\"events\":[]}";
    report.close();
    std::ofstream player("output/battles/temp_player_424242.json");
    player << "{\"team\":[{\"slot\":0,\"dishType\":\"Potato\",\"level\":1}]}";
    player.close();
    std::ofstream opponent("output/battles/temp_opponent_424242.json");
    opponent << "{\"team\":[{\"slot\":0,\"dishType\":\"Potato\",\"level\":1}]}";
    opponent.close();
  });

  app.wait_for_ui_exists("Watch Again", 5.0f);
  app.click("Watch Again");
  app.wait_for_screen(GameStateManager::Screen::Battle, 5.0f);

  app.once([&] {
    auto replay_ref = afterhours::EntityHelper::get_singleton<ReplayState>();
    app.expect_true(replay_ref.get().has<ReplayState>(),
                    "ReplayState exists after Watch Again");
    const ReplayState &replay = replay_ref.get().get<ReplayState>();
    uint64_t expected_seed = 424242;
    app.expect_eq(replay.seed, expected_seed,
                  "Watch Again replays the latest report seed");
    app.expect_true(replay.is_replay,
                    "Watch Again marks the battle as a replay");
    app.expect_false(replay.from_history,
                     "Watch Again is not a History replay");
    auto request_ref =
        afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    app.expect_true(request_ref.get().has<BattleLoadRequest>(),
                    "BattleLoadRequest exists after Watch Again");
    const BattleLoadRequest &request =
        request_ref.get().get<BattleLoadRequest>();
    app.expect_eq(request.playerJsonPath,
                  std::string("output/battles/temp_player_424242.json"),
                  "Watch Again loads the player team file");
    app.expect_eq(request.opponentJsonPath,
                  std::string("output/battles/temp_opponent_424242.json"),
                  "Watch Again loads the opponent team file");
  });

  app.once([&] {
    std::filesystem::remove("output/battles/results/29990101_000000_424242.json");
  });
}

TEST(validate_replay_invalid_report) {
  app.launch_game();
  app.once([&] {
    app.expect_false(
        replay::start_replay_from_report_file(
            "output/battles/results/replay_test_missing.json", false),
        "missing report must not start a replay");
    std::ofstream malformed(
        "output/battles/results/replay_test_malformed.json");
    malformed << "{not json";
    malformed.close();
    app.expect_false(
        replay::start_replay_from_report_file(
            "output/battles/results/replay_test_malformed.json", false),
        "malformed report must not start a replay");
    std::ofstream report("output/battles/results/replay_test_no_teams.json");
    report << "{\"seed\":777001,\"opponentId\":\"missing\",\"outcomes\":[],"
              "\"events\":[]}";
    report.close();
    app.expect_false(
        replay::start_replay_from_report_file(
            "output/battles/results/replay_test_no_teams.json", false),
        "report without team files must not start a replay");
    std::filesystem::remove(
        "output/battles/results/replay_test_malformed.json");
    std::filesystem::remove("output/battles/results/replay_test_no_teams.json");
  });
}
