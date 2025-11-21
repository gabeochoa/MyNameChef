#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/combat_queue.h"
#include "../../components/combat_stats.h"
#include "../../components/dish_battle_state.h"
#include "../../components/transform.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../../query.h"
#include "../test_app.h"
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
  static bool dishes_created = false;
  static afterhours::EntityID player_dish_id = 0;
  static afterhours::EntityID opponent_dish_0_id = 0;
  static afterhours::EntityID opponent_dish_1_id = 0;
  static const TestOperationID setup_op = TestApp::generate_operation_id(
      std::source_location::current(),
      "validate_survivor_carryover_single.setup");

  log_info(
      "TEST: ===== Starting validate_survivor_carryover_single test =====");

  if (app.completed_operations.count(setup_op) > 0) {
    log_info("TEST: Setup already completed, skipping dish creation");
  } else {
    log_info("TEST: [1] Calling app.launch_game()");
    app.launch_game();
    log_info("TEST: [1] app.launch_game() completed");

    log_info("TEST: [2] Calling app.wait_for_frames(1)");
    app.wait_for_frames(1);
    log_info("TEST: [2] app.wait_for_frames(1) completed");

    log_info("TEST: [3] Calling app.clear_battle_dishes()");
    app.clear_battle_dishes();
    log_info("TEST: [3] app.clear_battle_dishes() completed");

    log_info("TEST: [4] Ensuring battle load request exists");
    ValidateSurvivorCarryoverTestHelpers::ensure_battle_load_request_exists();
    log_info("TEST: [4] Battle load request ensured");

    log_info("TEST: [5] Calling app.setup_battle()");
    app.setup_battle();
    log_info("TEST: [5] app.setup_battle() completed");

    log_info("TEST: [6] Calling app.wait_for_frames(1)");
    app.wait_for_frames(1);
    log_info("TEST: [6] app.wait_for_frames(1) completed");

    if (!dishes_created) {
      log_info("TEST: [7] Creating player dish");
      player_dish_id = app.create_dish(DishType::Potato)
                           .on_team(DishBattleState::TeamSide::Player)
                           .at_slot(0)
                           .with_combat_stats()
                           .commit();
      log_info("TEST: [7] Created player dish with id: {}", player_dish_id);

      log_info("TEST: [8] Creating opponent dish 0");
      opponent_dish_0_id = app.create_dish(DishType::Potato)
                               .on_team(DishBattleState::TeamSide::Opponent)
                               .at_slot(0)
                               .with_combat_stats()
                               .commit();
      log_info("TEST: [8] Created opponent dish 0 with id: {}",
               opponent_dish_0_id);

      log_info("TEST: [9] Creating opponent dish 1");
      opponent_dish_1_id = app.create_dish(DishType::Potato)
                               .on_team(DishBattleState::TeamSide::Opponent)
                               .at_slot(1)
                               .with_combat_stats()
                               .commit();
      log_info("TEST: [9] Created opponent dish 1 with id: {}",
               opponent_dish_1_id);

      dishes_created = true;
      app.completed_operations.insert(setup_op);
    } else {
      log_info("TEST: Dishes already created - player: {}, opponent_0: {}, "
               "opponent_1: {}",
               player_dish_id, opponent_dish_0_id, opponent_dish_1_id);
    }
  }

  log_info("TEST: [10] Calling app.wait_for_frames(5)");
  app.wait_for_frames(5);
  log_info("TEST: [10] app.wait_for_frames(5) completed");

  log_info("TEST: [11] Setting combat stats - Player: 50 body, 1 zing; "
           "Opponent 0: 2 body, 15 zing; Opponent 1: 15 body, 1 zing");
  app.set_dish_combat_stats(player_dish_id, 50, 1);
  app.set_dish_combat_stats(opponent_dish_0_id, 2, 15);
  app.set_dish_combat_stats(opponent_dish_1_id, 15, 1);
  log_info("TEST: [11] Combat stats set");

  log_info("TEST: [12] Calling app.wait_for_battle_initialized(10.0f)");
  app.wait_for_battle_initialized(10.0f);
  log_info("TEST: [12] app.wait_for_battle_initialized() completed");

  log_info("TEST: [13] Calling app.wait_for_frames(30)");
  app.wait_for_frames(30);
  log_info("TEST: [13] app.wait_for_frames(30) completed");

  log_info("TEST: [14] Calling app.wait_for_course_complete(0, 60.0f)");
  app.wait_for_course_complete(0, 60.0f);
  log_info("TEST: [14] app.wait_for_course_complete(0) completed");

  log_info("TEST: [15] Calling app.wait_for_reorganization(5.0f)");
  app.wait_for_reorganization(5.0f);
  log_info("TEST: [15] app.wait_for_reorganization() completed");

  log_info("TEST: [16] Calling app.expect_battle_not_complete()");
  app.expect_battle_not_complete(
      "Battle should continue with opponent dish remaining after course 0");
  log_info("TEST: [16] app.expect_battle_not_complete() completed");

  log_info("TEST: [17] Checking player dish body (should have remaining after "
           "course 0)");
  auto player_entity_opt_body =
      afterhours::EntityHelper::getEntityForID(player_dish_id);
  if (player_entity_opt_body &&
      player_entity_opt_body.asE().has<CombatStats>()) {
    int actual_body =
        player_entity_opt_body.asE().get<CombatStats>().currentBody;
    log_info("TEST: [17] Player dish body: {}", actual_body);
    app.expect_true(actual_body > 0,
                    "Player dish should have remaining Body after course 0");
  } else {
    app.fail("Player dish not found or missing CombatStats");
  }
  log_info("TEST: [17] Player dish body check completed");

  log_info("TEST: [18] Checking dish phase - may be InQueue (after "
           "reorganization) or InCombat (if course 1 started)");
  auto player_entity_opt =
      afterhours::EntityHelper::getEntityForID(player_dish_id);
  if (player_entity_opt) {
    const DishBattleState &dbs = player_entity_opt.asE().get<DishBattleState>();
    log_info("TEST: [18] Player dish phase: {} (0=InQueue, 1=Entering, "
             "2=InCombat, 3=Finished)",
             static_cast<int>(dbs.phase));
    app.expect_true(dbs.phase == DishBattleState::Phase::InQueue ||
                        dbs.phase == DishBattleState::Phase::InCombat,
                    "Survivor should be reset to InQueue or in InCombat if "
                    "course 1 started");
  } else {
    app.fail("Player dish not found");
  }
  log_info("TEST: [18] Dish phase check completed");

  log_info("TEST: [20] Counting opponent dishes after course 0");
  int opponent_count_after_course_0 = app.count_active_opponent_dishes();
  log_info("TEST: [20] Opponent dishes after course 0: {}",
           opponent_count_after_course_0);

  log_info("TEST: [21] Calling app.expect_count_gte() for player dishes");
  app.expect_count_gte(app.count_active_player_dishes(), 1,
                       "Player should have at least 1 active dish (survivor)");
  log_info("TEST: [21] app.expect_count_gte() for player completed");

  log_info("TEST: [22] Calling app.expect_count_gte() for opponent dishes");
  app.expect_count_gte(opponent_count_after_course_0, 1,
                       "Opponent should have at least 1 active dish remaining");
  log_info("TEST: [22] app.expect_count_gte() for opponent completed");

  log_info("TEST: [23] Calling app.wait_for_frames(20)");
  app.wait_for_frames(20);
  log_info("TEST: [23] app.wait_for_frames(20) completed");

  log_info("TEST: [24] Calling app.expect_dish_at_index(player_dish_id, 0)");
  app.expect_dish_at_index(player_dish_id, 0, DishBattleState::TeamSide::Player,
                           "Player dish should stay at index 0");
  log_info("TEST: [24] app.expect_dish_at_index() completed");

  log_info("TEST: [25] Querying for opponent dish at index 0");
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
  log_info("TEST: [25] Opponent at zero query completed, has_value: {}",
           opponent_at_zero.has_value());

  log_info("TEST: [26] Calling app.expect_true(opponent_at_zero.has_value())");
  app.expect_true(
      opponent_at_zero.has_value(),
      "Opponent should have a dish at index 0 after reorganization");
  log_info("TEST: [26] app.expect_true() completed");

  log_info("TEST: [27] Calling app.wait_for_frames(20)");
  app.wait_for_frames(20);
  log_info("TEST: [27] app.wait_for_frames(20) completed");

  log_info("TEST: [28] Calling app.wait_for_dishes_in_combat(1, 60.0f)");
  app.wait_for_dishes_in_combat(
      1, 60.0f, "Survivor should enter combat with next opponent");
  log_info("TEST: [28] app.wait_for_dishes_in_combat() completed");

  log_info("TEST: [29] Calling app.wait_for_battle_complete(180.0f) - waiting "
           "for opponent to be defeated");
  app.wait_for_battle_complete(180.0f);
  log_info("TEST: [29] app.wait_for_battle_complete() completed");

  log_info("TEST: [33] Calling app.wait_for_frames(50)");
  app.wait_for_frames(50);
  log_info("TEST: [33] app.wait_for_frames(50) completed");

  log_info("TEST: [34] Calling app.expect_battle_complete()");
  app.expect_battle_complete("Battle should be marked as complete");
  log_info("TEST: [34] app.expect_battle_complete() completed");

  log_info("TEST: [35] Calling app.expect_count_eq() for opponent dishes");
  app.expect_count_eq(
      app.count_active_opponent_dishes(), 0,
      "Opponent should have no active dishes after battle completes");
  log_info("TEST: [35] app.expect_count_eq() completed");

  log_info(
      "TEST: ===== validate_survivor_carryover_single test COMPLETED =====");
}

TEST(validate_survivor_carryover_positions) {
  log_info("TEST: Starting validate_survivor_carryover_positions test");

  app.launch_game();
  app.wait_for_frames(1);

  app.clear_battle_dishes();

  ValidateSurvivorCarryoverTestHelpers::ensure_battle_load_request_exists();
  app.setup_battle();
  app.wait_for_frames(1);

  afterhours::EntityID player_dish_0_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Player)
          .at_slot(0)
          .with_combat_stats()
          .commit();

  afterhours::EntityID player_dish_1_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Player)
          .at_slot(1)
          .with_combat_stats()
          .commit();

  afterhours::EntityID opponent_dish_0_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Opponent)
          .at_slot(0)
          .with_combat_stats()
          .commit();

  afterhours::EntityID opponent_dish_1_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Opponent)
          .at_slot(1)
          .with_combat_stats()
          .commit();

  afterhours::EntityID opponent_dish_2_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Opponent)
          .at_slot(2)
          .with_combat_stats()
          .commit();

  app.wait_for_frames(5);

  app.set_dish_combat_stats(player_dish_0_id, 31, 1);
  app.set_dish_combat_stats(player_dish_1_id, 31, 1);
  app.set_dish_combat_stats(opponent_dish_0_id, 2, 15);
  app.set_dish_combat_stats(opponent_dish_1_id, 2, 15);
  app.set_dish_combat_stats(opponent_dish_2_id, 2, 15);

  app.wait_for_battle_initialized(10.0f);
  app.wait_for_frames(30);

  app.wait_for_course_complete(0, 30.0f);

  app.wait_for_frames(20);

  app.expect_dish_at_index(player_dish_0_id, 0,
                           DishBattleState::TeamSide::Player,
                           "Player dish 0 should stay at index 0");
  app.expect_dish_at_index(player_dish_1_id, 1,
                           DishBattleState::TeamSide::Player,
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

  app.expect_true(
      opponent_at_zero.has_value(),
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

  app.expect_true(
      opponent_at_one.has_value(),
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

  afterhours::EntityID player_dish_0_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Player)
          .at_slot(0)
          .with_combat_stats()
          .commit();

  afterhours::EntityID player_dish_1_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Player)
          .at_slot(1)
          .with_combat_stats()
          .commit();

  afterhours::EntityID player_dish_2_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Player)
          .at_slot(2)
          .with_combat_stats()
          .commit();

  afterhours::EntityID opponent_dish_0_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Opponent)
          .at_slot(0)
          .with_combat_stats()
          .commit();

  afterhours::EntityID opponent_dish_1_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Opponent)
          .at_slot(1)
          .with_combat_stats()
          .commit();

  afterhours::EntityID opponent_dish_2_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Opponent)
          .at_slot(2)
          .with_combat_stats()
          .commit();

  afterhours::EntityID opponent_dish_3_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Opponent)
          .at_slot(3)
          .with_combat_stats()
          .commit();

  app.wait_for_frames(5);

  app.set_dish_combat_stats(player_dish_0_id, 31, 1);
  app.set_dish_combat_stats(player_dish_1_id, 31, 1);
  app.set_dish_combat_stats(player_dish_2_id, 31, 1);
  app.set_dish_combat_stats(opponent_dish_0_id, 2, 15);
  app.set_dish_combat_stats(opponent_dish_1_id, 2, 15);
  app.set_dish_combat_stats(opponent_dish_2_id, 2, 15);
  app.set_dish_combat_stats(opponent_dish_3_id, 2, 15);

  app.wait_for_battle_initialized(10.0f);
  app.wait_for_frames(30);

  app.wait_for_course_complete(0, 60.0f);

  app.wait_for_frames(50);

  app.expect_dish_body(player_dish_0_id, 1,
                       "Player dish 0 should have remaining Body");
  app.expect_dish_body(player_dish_1_id, 1,
                       "Player dish 1 should have remaining Body");
  app.expect_dish_body(player_dish_2_id, 1,
                       "Player dish 2 should have remaining Body");

  app.expect_dish_phase(player_dish_0_id, DishBattleState::Phase::InQueue,
                        "Player dish 0 should be reset to InQueue");
  app.expect_dish_phase(player_dish_1_id, DishBattleState::Phase::InQueue,
                        "Player dish 1 should be reset to InQueue");
  app.expect_dish_phase(player_dish_2_id, DishBattleState::Phase::InQueue,
                        "Player dish 2 should be reset to InQueue");

  app.wait_for_frames(50);

  app.expect_dish_at_index(player_dish_0_id, 0,
                           DishBattleState::TeamSide::Player,
                           "Player dish 0 should stay at index 0");
  app.expect_dish_at_index(player_dish_1_id, 1,
                           DishBattleState::TeamSide::Player,
                           "Player dish 1 should stay at index 1");
  app.expect_dish_at_index(player_dish_2_id, 2,
                           DishBattleState::TeamSide::Player,
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

  app.expect_true(
      opponent_at_zero.has_value(),
      "Opponent should have a dish at index 0 after reorganization");

  app.wait_for_dishes_in_combat(
      1, 60.0f, "Player dish 0 should enter combat with next opponent");

  app.wait_for_course_complete(1, 120.0f);

  app.wait_for_frames(150);

  int opponent_count = app.count_active_opponent_dishes();
  int player_count = app.count_active_player_dishes();
  log_info("TEST: After course 1 - Player: {}, Opponent: {}", player_count,
           opponent_count);

  if (opponent_count == 0 || player_count == 0) {
    log_info("TEST: Battle already complete");
  } else {
    app.wait_for_battle_complete(120.0f);
  }

  app.wait_for_frames(50);
  app.expect_battle_complete(
      "Battle should complete when opponent has no active dishes");
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

  afterhours::EntityID player_dish_0_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Player)
          .at_slot(0)
          .with_combat_stats()
          .commit();

  afterhours::EntityID player_dish_1_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Player)
          .at_slot(1)
          .with_combat_stats()
          .commit();

  afterhours::EntityID opponent_dish_0_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Opponent)
          .at_slot(0)
          .with_combat_stats()
          .commit();

  afterhours::EntityID opponent_dish_1_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Opponent)
          .at_slot(1)
          .with_combat_stats()
          .commit();

  app.wait_for_frames(5);

  app.set_dish_combat_stats(player_dish_0_id, 31, 1);
  app.set_dish_combat_stats(player_dish_1_id, 31, 1);
  app.set_dish_combat_stats(opponent_dish_0_id, 2, 15);
  app.set_dish_combat_stats(opponent_dish_1_id, 2, 15);

  app.wait_for_battle_initialized(10.0f);
  app.wait_for_frames(30);

  app.wait_for_course_complete(0, 60.0f);

  app.wait_for_frames(50);

  app.expect_battle_not_complete(
      "Battle should not complete after course 1 - opponent still has dish 1");
  app.expect_count_gte(app.count_active_opponent_dishes(), 1,
                       "Opponent should have at least 1 active dish remaining");

  app.wait_for_frames(50);

  app.wait_for_dishes_in_combat(
      1, 60.0f, "Player dish 0 should enter combat with opponent dish 1");

  app.wait_for_course_complete(1, 120.0f);

  app.wait_for_frames(150);

  int opponent_count = app.count_active_opponent_dishes();
  int player_count = app.count_active_player_dishes();
  log_info("TEST: After course 1 - Player: {}, Opponent: {}", player_count,
           opponent_count);

  if (opponent_count == 0 || player_count == 0) {
    log_info("TEST: Battle already complete");
  } else {
    app.wait_for_battle_complete(120.0f);
  }

  app.wait_for_frames(50);
  app.expect_battle_complete("Battle should complete after course 2 when "
                             "opponent has no active dishes");
  app.expect_count_eq(app.count_active_opponent_dishes(), 0,
                      "Opponent should have no active dishes");
}

TEST(validate_survivor_carryover_simultaneous_defeat) {
  log_info(
      "TEST: Starting validate_survivor_carryover_simultaneous_defeat test");

  app.launch_game();
  app.wait_for_frames(1);

  app.clear_battle_dishes();

  ValidateSurvivorCarryoverTestHelpers::ensure_battle_load_request_exists();
  app.setup_battle();
  app.wait_for_frames(1);

  afterhours::EntityID player_dish_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Player)
          .at_slot(0)
          .with_combat_stats()
          .commit();

  afterhours::EntityID opponent_dish_0_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Opponent)
          .at_slot(0)
          .with_combat_stats()
          .commit();

  afterhours::EntityID opponent_dish_1_id =
      app.create_dish(DishType::Potato)
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

  app.wait_for_course_complete(0, 60.0f);

  app.wait_for_frames(50);

  app.expect_dish_phase(
      player_dish_id, DishBattleState::Phase::Finished,
      "Player dish should be Finished after simultaneous defeat");
  app.expect_dish_phase(
      opponent_dish_0_id, DishBattleState::Phase::Finished,
      "Opponent dish 0 should be Finished after simultaneous defeat");

  int player_count = app.count_active_player_dishes();
  int opponent_count = app.count_active_opponent_dishes();
  log_info("TEST: After simultaneous defeat - Player: {}, Opponent: {}",
           player_count, opponent_count);

  app.expect_count_eq(player_count, 0, "Player should have no active dishes");
  app.expect_count_gte(opponent_count, 1,
                       "Opponent should have at least 1 active dish remaining");

  if (player_count == 0) {
    app.wait_for_frames(50);
    app.wait_for_battle_complete(60.0f);
  }

  app.wait_for_frames(50);
  app.expect_battle_complete(
      "Battle should complete when player has no active dishes");
}
