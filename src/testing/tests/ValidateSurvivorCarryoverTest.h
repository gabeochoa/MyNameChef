#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/combat_queue.h"
#include "../../components/combat_stats.h"
#include "../../components/dish_battle_state.h"
#include "../../components/transform.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../../query.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

namespace ValidateSurvivorCarryoverTestHelpers {
static void ensure_battle_load_request_exists() {
  const auto componentId =
      afterhours::components::get_type_id<BattleLoadRequest>();
  auto &singletonMap = afterhours::EntityHelper::get().singletonMap;
  if (singletonMap.contains(componentId)) {
    return;
  }
  auto &requestEntity = afterhours::EntityHelper::createEntity();
  BattleLoadRequest request;
  request.playerJsonPath = "";
  request.opponentJsonPath = "";
  request.loaded = false;
  requestEntity.addComponent<BattleLoadRequest>(std::move(request));
  afterhours::EntityHelper::registerSingleton<BattleLoadRequest>(requestEntity);
}
} // namespace ValidateSurvivorCarryoverTestHelpers

TEST(validate_survivor_carryover_single) {
  log_info("TEST: Starting validate_survivor_carryover_single test");

  app.launch_game();
  app.wait_for_frames(1);

  app.clear_battle_dishes();

  ValidateSurvivorCarryoverTestHelpers::ensure_battle_load_request_exists();
  app.setup_battle();
  app.wait_for_frames(1);

  afterhours::EntityID player_dish_id = app.create_dish(DishType::Potato)
                                           .on_team(DishBattleState::TeamSide::Player)
                                           .at_slot(0)
                                           .with_combat_stats()
                                           .commit();

  afterhours::EntityID opponent_dish_0_id = app.create_dish(DishType::Potato)
                                                .on_team(DishBattleState::TeamSide::Opponent)
                                                .at_slot(0)
                                                .with_combat_stats()
                                                .commit();

  afterhours::EntityID opponent_dish_1_id = app.create_dish(DishType::Potato)
                                                .on_team(DishBattleState::TeamSide::Opponent)
                                                .at_slot(1)
                                                .with_combat_stats()
                                                .commit();

  app.wait_for_frames(5);

  app.set_dish_combat_stats(player_dish_id, 30, 1);
  app.set_dish_combat_stats(opponent_dish_0_id, 3, 15);
  app.set_dish_combat_stats(opponent_dish_1_id, 3, 15);

  app.wait_for_battle_initialized(10.0f);
  app.wait_for_frames(30);

  app.wait_for_course_complete(0, 30.0f);

  app.wait_for_frames(20);

  app.expect_dish_body(player_dish_id, 1, "Player dish should have remaining Body");
  app.expect_dish_phase(player_dish_id, DishBattleState::Phase::InQueue,
                        "Survivor should be reset to InQueue");

  app.expect_count_gte(app.count_active_player_dishes(), 1,
                       "Player should have at least 1 active dish (survivor)");
  app.expect_count_gte(app.count_active_opponent_dishes(), 1,
                       "Opponent should have at least 1 active dish remaining");

  app.wait_for_frames(20);

  app.expect_dish_at_index(player_dish_id, 0, DishBattleState::TeamSide::Player,
                           "Player dish should stay at index 0");

  afterhours::OptEntity opponent_at_zero =
      afterhours::EntityQuery({.force_merge = true})
          .whereHasComponent<DishBattleState>()
          .whereLambda([](const afterhours::Entity &e) {
            const DishBattleState &dbs = e.get<DishBattleState>();
            return dbs.team_side == DishBattleState::TeamSide::Opponent &&
                   dbs.queue_index == 0 &&
                   dbs.phase != DishBattleState::Phase::Finished;
          })
          .gen_first();

  app.expect_true(opponent_at_zero.has_value(),
                  "Opponent should have a dish at index 0 after reorganization");

  app.expect_battle_not_complete("Battle should continue with opponent dish remaining");

  app.wait_for_frames(20);

  app.wait_for_dishes_in_combat(1, 10.0f, "Survivor should enter combat with next opponent");

  app.wait_for_course_complete(1, 30.0f);

  app.wait_for_frames(30);

  app.wait_for_battle_complete(30.0f);
  
  app.expect_battle_complete("Battle should be marked as complete");
}

TEST(validate_survivor_carryover_positions) {
  log_info("TEST: Starting validate_survivor_carryover_positions test");

  app.launch_game();
  app.wait_for_frames(1);

  app.clear_battle_dishes();

  ValidateSurvivorCarryoverTestHelpers::ensure_battle_load_request_exists();
  app.setup_battle();
  app.wait_for_frames(1);

  afterhours::EntityID player_dish_0_id = app.create_dish(DishType::Potato)
                                             .on_team(DishBattleState::TeamSide::Player)
                                             .at_slot(0)
                                             .with_combat_stats()
                                             .commit();

  afterhours::EntityID player_dish_1_id = app.create_dish(DishType::Potato)
                                             .on_team(DishBattleState::TeamSide::Player)
                                             .at_slot(1)
                                             .with_combat_stats()
                                             .commit();

  afterhours::EntityID opponent_dish_0_id = app.create_dish(DishType::Potato)
                                               .on_team(DishBattleState::TeamSide::Opponent)
                                               .at_slot(0)
                                               .with_combat_stats()
                                               .commit();

  afterhours::EntityID opponent_dish_1_id = app.create_dish(DishType::Potato)
                                               .on_team(DishBattleState::TeamSide::Opponent)
                                               .at_slot(1)
                                               .with_combat_stats()
                                               .commit();

  afterhours::EntityID opponent_dish_2_id = app.create_dish(DishType::Potato)
                                               .on_team(DishBattleState::TeamSide::Opponent)
                                               .at_slot(2)
                                               .with_combat_stats()
                                               .commit();

  app.wait_for_frames(5);

  app.set_dish_combat_stats(player_dish_0_id, 30, 1);
  app.set_dish_combat_stats(player_dish_1_id, 30, 1);
  app.set_dish_combat_stats(opponent_dish_0_id, 3, 15);
  app.set_dish_combat_stats(opponent_dish_1_id, 3, 15);
  app.set_dish_combat_stats(opponent_dish_2_id, 3, 15);

  app.wait_for_battle_initialized(10.0f);
  app.wait_for_frames(30);

  app.wait_for_course_complete(0, 30.0f);

  app.wait_for_frames(20);

  app.expect_dish_at_index(player_dish_0_id, 0, DishBattleState::TeamSide::Player,
                           "Player dish 0 should stay at index 0");
  app.expect_dish_at_index(player_dish_1_id, 1, DishBattleState::TeamSide::Player,
                           "Player dish 1 should stay at index 1");

  afterhours::OptEntity opponent_at_zero =
      afterhours::EntityQuery({.force_merge = true})
          .whereHasComponent<DishBattleState>()
          .whereLambda([](const afterhours::Entity &e) {
            const DishBattleState &dbs = e.get<DishBattleState>();
            return dbs.team_side == DishBattleState::TeamSide::Opponent &&
                   dbs.queue_index == 0 &&
                   dbs.phase != DishBattleState::Phase::Finished;
          })
          .gen_first();

  app.expect_true(opponent_at_zero.has_value(),
                  "Opponent should have a dish at index 0 after reorganization");

  afterhours::OptEntity opponent_at_one =
      afterhours::EntityQuery({.force_merge = true})
          .whereHasComponent<DishBattleState>()
          .whereLambda([](const afterhours::Entity &e) {
            const DishBattleState &dbs = e.get<DishBattleState>();
            return dbs.team_side == DishBattleState::TeamSide::Opponent &&
                   dbs.queue_index == 1 &&
                   dbs.phase != DishBattleState::Phase::Finished;
          })
          .gen_first();

  app.expect_true(opponent_at_one.has_value(),
                  "Opponent should have a dish at index 1 after reorganization");
}

TEST(validate_survivor_carryover_multiple) {
  log_info("TEST: Starting validate_survivor_carryover_multiple test");

  app.launch_game();
  app.wait_for_frames(1);

  app.clear_battle_dishes();

  ValidateSurvivorCarryoverTestHelpers::ensure_battle_load_request_exists();
  app.setup_battle();
  app.wait_for_frames(1);

  afterhours::EntityID player_dish_0_id = app.create_dish(DishType::Potato)
                                             .on_team(DishBattleState::TeamSide::Player)
                                             .at_slot(0)
                                             .with_combat_stats()
                                             .commit();

  afterhours::EntityID player_dish_1_id = app.create_dish(DishType::Potato)
                                             .on_team(DishBattleState::TeamSide::Player)
                                             .at_slot(1)
                                             .with_combat_stats()
                                             .commit();

  afterhours::EntityID player_dish_2_id = app.create_dish(DishType::Potato)
                                             .on_team(DishBattleState::TeamSide::Player)
                                             .at_slot(2)
                                             .with_combat_stats()
                                             .commit();

  afterhours::EntityID opponent_dish_0_id = app.create_dish(DishType::Potato)
                                               .on_team(DishBattleState::TeamSide::Opponent)
                                               .at_slot(0)
                                               .with_combat_stats()
                                               .commit();

  afterhours::EntityID opponent_dish_1_id = app.create_dish(DishType::Potato)
                                               .on_team(DishBattleState::TeamSide::Opponent)
                                               .at_slot(1)
                                               .with_combat_stats()
                                               .commit();

  afterhours::EntityID opponent_dish_2_id = app.create_dish(DishType::Potato)
                                               .on_team(DishBattleState::TeamSide::Opponent)
                                               .at_slot(2)
                                               .with_combat_stats()
                                               .commit();

  afterhours::EntityID opponent_dish_3_id = app.create_dish(DishType::Potato)
                                               .on_team(DishBattleState::TeamSide::Opponent)
                                               .at_slot(3)
                                               .with_combat_stats()
                                               .commit();

  app.wait_for_frames(5);

  app.set_dish_combat_stats(player_dish_0_id, 30, 1);
  app.set_dish_combat_stats(player_dish_1_id, 30, 1);
  app.set_dish_combat_stats(player_dish_2_id, 30, 1);
  app.set_dish_combat_stats(opponent_dish_0_id, 3, 15);
  app.set_dish_combat_stats(opponent_dish_1_id, 3, 15);
  app.set_dish_combat_stats(opponent_dish_2_id, 3, 15);
  app.set_dish_combat_stats(opponent_dish_3_id, 3, 15);

  app.wait_for_battle_initialized(10.0f);
  app.wait_for_frames(30);

  app.wait_for_course_complete(0, 30.0f);

  app.wait_for_frames(20);

  app.expect_dish_body(player_dish_0_id, 1, "Player dish 0 should have remaining Body");
  app.expect_dish_body(player_dish_1_id, 1, "Player dish 1 should have remaining Body");
  app.expect_dish_body(player_dish_2_id, 1, "Player dish 2 should have remaining Body");

  app.expect_dish_phase(player_dish_0_id, DishBattleState::Phase::InQueue,
                        "Player dish 0 should be reset to InQueue");
  app.expect_dish_phase(player_dish_1_id, DishBattleState::Phase::InQueue,
                        "Player dish 1 should be reset to InQueue");
  app.expect_dish_phase(player_dish_2_id, DishBattleState::Phase::InQueue,
                        "Player dish 2 should be reset to InQueue");

  app.wait_for_frames(20);

  app.expect_dish_at_index(player_dish_0_id, 0, DishBattleState::TeamSide::Player,
                           "Player dish 0 should stay at index 0");
  app.expect_dish_at_index(player_dish_1_id, 1, DishBattleState::TeamSide::Player,
                           "Player dish 1 should stay at index 1");
  app.expect_dish_at_index(player_dish_2_id, 2, DishBattleState::TeamSide::Player,
                           "Player dish 2 should stay at index 2");

  afterhours::OptEntity opponent_at_zero =
      afterhours::EntityQuery({.force_merge = true})
          .whereHasComponent<DishBattleState>()
          .whereLambda([](const afterhours::Entity &e) {
            const DishBattleState &dbs = e.get<DishBattleState>();
            return dbs.team_side == DishBattleState::TeamSide::Opponent &&
                   dbs.queue_index == 0 &&
                   dbs.phase != DishBattleState::Phase::Finished;
          })
          .gen_first();

  app.expect_true(opponent_at_zero.has_value(),
                  "Opponent should have a dish at index 0 after reorganization");

  app.wait_for_dishes_in_combat(1, 10.0f, "Player dish 0 should enter combat with next opponent");

  app.wait_for_course_complete(1, 30.0f);

  app.expect_battle_complete("Battle should complete when opponent has no active dishes");
  app.expect_count_eq(app.count_active_opponent_dishes(), 0,
                      "Opponent should have no active dishes");
}

TEST(validate_survivor_carryover_battle_completion) {
  log_info("TEST: Starting validate_survivor_carryover_battle_completion test");

  app.launch_game();
  app.wait_for_frames(1);

  app.clear_battle_dishes();

  ValidateSurvivorCarryoverTestHelpers::ensure_battle_load_request_exists();
  app.setup_battle();
  app.wait_for_frames(1);

  afterhours::EntityID player_dish_0_id = app.create_dish(DishType::Potato)
                                             .on_team(DishBattleState::TeamSide::Player)
                                             .at_slot(0)
                                             .with_combat_stats()
                                             .commit();

  afterhours::EntityID player_dish_1_id = app.create_dish(DishType::Potato)
                                             .on_team(DishBattleState::TeamSide::Player)
                                             .at_slot(1)
                                             .with_combat_stats()
                                             .commit();

  afterhours::EntityID opponent_dish_0_id = app.create_dish(DishType::Potato)
                                               .on_team(DishBattleState::TeamSide::Opponent)
                                               .at_slot(0)
                                               .with_combat_stats()
                                               .commit();

  afterhours::EntityID opponent_dish_1_id = app.create_dish(DishType::Potato)
                                               .on_team(DishBattleState::TeamSide::Opponent)
                                               .at_slot(1)
                                               .with_combat_stats()
                                               .commit();

  app.wait_for_frames(5);

  app.set_dish_combat_stats(player_dish_0_id, 30, 1);
  app.set_dish_combat_stats(player_dish_1_id, 30, 1);
  app.set_dish_combat_stats(opponent_dish_0_id, 3, 15);
  app.set_dish_combat_stats(opponent_dish_1_id, 3, 15);

  app.wait_for_battle_initialized(10.0f);
  app.wait_for_frames(30);

  app.wait_for_course_complete(0, 30.0f);

  app.expect_battle_not_complete("Battle should not complete after course 1 - opponent still has dish 1");
  app.expect_count_gte(app.count_active_opponent_dishes(), 1,
                       "Opponent should have at least 1 active dish remaining");

  app.wait_for_frames(20);

  app.wait_for_dishes_in_combat(1, 10.0f, "Player dish 0 should enter combat with opponent dish 1");

  app.wait_for_course_complete(1, 30.0f);

  app.expect_battle_complete("Battle should complete after course 2 when opponent has no active dishes");
  app.expect_count_eq(app.count_active_opponent_dishes(), 0,
                      "Opponent should have no active dishes");
}

TEST(validate_survivor_carryover_simultaneous_defeat) {
  log_info("TEST: Starting validate_survivor_carryover_simultaneous_defeat test");

  app.launch_game();
  app.wait_for_frames(1);

  app.clear_battle_dishes();

  ValidateSurvivorCarryoverTestHelpers::ensure_battle_load_request_exists();
  app.setup_battle();
  app.wait_for_frames(1);

  afterhours::EntityID player_dish_id = app.create_dish(DishType::Potato)
                                           .on_team(DishBattleState::TeamSide::Player)
                                           .at_slot(0)
                                           .with_combat_stats()
                                           .commit();

  afterhours::EntityID opponent_dish_0_id = app.create_dish(DishType::Potato)
                                               .on_team(DishBattleState::TeamSide::Opponent)
                                               .at_slot(0)
                                               .with_combat_stats()
                                               .commit();

  afterhours::EntityID opponent_dish_1_id = app.create_dish(DishType::Potato)
                                               .on_team(DishBattleState::TeamSide::Opponent)
                                               .at_slot(1)
                                               .with_combat_stats()
                                               .commit();

  app.wait_for_frames(5);

  app.set_dish_combat_stats(player_dish_id, 10, 10);
  app.set_dish_combat_stats(opponent_dish_0_id, 10, 10);
  app.set_dish_combat_stats(opponent_dish_1_id, 5, 10);

  app.wait_for_battle_initialized(10.0f);
  app.wait_for_frames(30);

  app.wait_for_course_complete(0, 30.0f);

  app.wait_for_frames(20);

  app.expect_dish_phase(player_dish_id, DishBattleState::Phase::Finished,
                        "Player dish should be Finished after simultaneous defeat");
  app.expect_dish_phase(opponent_dish_0_id, DishBattleState::Phase::Finished,
                        "Opponent dish 0 should be Finished after simultaneous defeat");

  app.expect_count_eq(app.count_active_player_dishes(), 0,
                      "Player should have no active dishes");
  app.expect_count_gte(app.count_active_opponent_dishes(), 1,
                       "Opponent should have at least 1 active dish remaining");

  app.expect_battle_complete("Battle should complete when player has no active dishes");
}
