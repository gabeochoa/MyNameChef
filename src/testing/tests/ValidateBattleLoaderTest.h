#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/battle_team_data.h"
#include "../../components/combat_queue.h"
#include "../../game_state_manager.h"
#include "../test_macros.h"
#include <filesystem>
#include <fstream>

TEST(validate_battle_loader_missing_player_file) {
  app.launch_game();
  app.setup_battle();

  app.once([&] {
    std::filesystem::create_directories("output/battles");
    std::ofstream opponent(
        "output/battles/temp_opponent_loader_missing_player.json");
    opponent << "{\"team\":[{\"slot\":0,\"dishType\":\"Potato\",\"level\":1}]}";
    opponent.close();
    auto request_ref =
        afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    app.expect_true(request_ref.get().has<BattleLoadRequest>(),
                    "BattleLoadRequest exists before loader test");
    BattleLoadRequest &request =
        request_ref.get().get<BattleLoadRequest>();
    request.playerJsonPath =
        "output/battles/temp_player_loader_missing_player.json";
    request.opponentJsonPath =
        "output/battles/temp_opponent_loader_missing_player.json";
    request.serverUrl = "";
    request.serverRequestPending = false;
    request.loaded = false;
  });

  app.wait_for_frames(5);

  app.once([&] {
    auto request_ref =
        afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    app.expect_true(request_ref.get().has<BattleLoadRequest>(),
                    "BattleLoadRequest exists after loader runs");
    app.expect_true(
        request_ref.get().get<BattleLoadRequest>().loaded,
        "loader finishes gracefully with a missing player file");
    auto manager_entity = afterhours::EntityHelper::get_singleton<CombatQueue>();
    app.expect_true(manager_entity.get().has<CombatQueue>(),
                    "CombatQueue exists after loader runs");
    app.expect_true(
        manager_entity.get().has<BattleTeamDataPlayer>(),
        "player team data is assigned on failed load");
    app.expect_true(
        manager_entity.get().get<BattleTeamDataPlayer>().team.empty(),
        "failed player load installs an empty team, not stale data");
    app.expect_true(
        manager_entity.get().has<BattleTeamDataOpponent>(),
        "opponent team data is assigned");
    app.expect_false(
        manager_entity.get().get<BattleTeamDataOpponent>().team.empty(),
        "opponent team still loads when the player file is missing");
  });
}
