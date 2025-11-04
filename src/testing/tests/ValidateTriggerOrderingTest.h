#pragma once

#include "../../components/combat_stats.h"
#include "../../components/dish_battle_state.h"
#include "../../components/dish_level.h"
#include "../../components/is_dish.h"
#include "../../components/trigger_event.h"
#include "../../components/trigger_queue.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../query.h"
#include "../../systems/ComputeCombatStatsSystem.h"
#include "../../systems/TriggerDispatchSystem.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

namespace ValidateTriggerOrderingTestHelpers {

static afterhours::Entity &get_or_create_trigger_queue() {
  afterhours::RefEntity tq_ref =
      afterhours::EntityHelper::get_singleton<TriggerQueue>();
  afterhours::Entity &tq_entity = tq_ref.get();
  if (!tq_entity.has<TriggerQueue>()) {
    tq_entity.addComponent<TriggerQueue>();
    afterhours::EntityHelper::registerSingleton<TriggerQueue>(tq_entity);
  }
  return tq_entity;
}

static afterhours::Entity &add_dish_to_menu(DishType type,
                                            DishBattleState::TeamSide team_side,
                                            int queue_index,
                                            DishBattleState::Phase phase) {
  auto &dish = afterhours::EntityHelper::createEntity();
  dish.addComponent<IsDish>(type);
  dish.addComponent<DishLevel>(1);

  auto &dbs = dish.addComponent<DishBattleState>();
  dbs.queue_index = queue_index;
  dbs.team_side = team_side;
  dbs.phase = phase;

  return dish;
}

} // namespace ValidateTriggerOrderingTestHelpers

TEST(validate_trigger_ordering) {
  (void)app;
  using namespace ValidateTriggerOrderingTestHelpers;
  log_info("TRIGGER_ORDERING_TEST: Testing trigger event ordering logic");

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  // Create test scenario:
  // Player team: 2 dishes (higher total Zing = 10)
  // Opponent team: 2 dishes (lower total Zing = 6)
  // We'll test ordering across slots and teams

  // Player team (higher total Zing = 10)
  auto &player_dish1 =
      add_dish_to_menu(DishType::Salmon, DishBattleState::TeamSide::Player, 0,
                       DishBattleState::Phase::InQueue); // Salmon has umami=3,
                                                         // freshness=2 = 5 zing
  auto &player_dish2 =
      add_dish_to_menu(DishType::Salmon, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::InQueue); // 5 zing

  // Opponent team (lower total Zing = 6)
  auto &opponent_dish1 =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Opponent, 0,
                       DishBattleState::Phase::InQueue); // 1 zing
  auto &opponent_dish2 =
      add_dish_to_menu(DishType::Salmon, DishBattleState::TeamSide::Opponent, 1,
                       DishBattleState::Phase::InQueue); // 5 zing

  afterhours::EntityHelper::merge_entity_arrays();

  // Run ComputeCombatStatsSystem to calculate baseZing
  ComputeCombatStatsSystem computeStats;
  for (afterhours::Entity &e : EQ({.ignore_temp_warning = true})
                                   .whereHasComponent<IsDish>()
                                   .whereHasComponent<DishLevel>()
                                   .gen()) {
    if (computeStats.should_run(1.0f / 60.0f)) {
      IsDish &dish = e.get<IsDish>();
      DishLevel &lvl = e.get<DishLevel>();
      computeStats.for_each_with(e, dish, lvl, 1.0f / 60.0f);
    }
  }

  // Verify CombatStats were calculated
  for (afterhours::Entity &e :
       EQ({.ignore_temp_warning = true}).whereHasComponent<IsDish>().gen()) {
    if (!e.has<CombatStats>()) {
      log_error("TRIGGER_ORDERING_TEST: Dish {} missing CombatStats", e.id);
      return;
    }
  }

  // Create trigger events in random order to test sorting
  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();

  // Add events in non-ordered sequence:
  // slot 1: opponent_dish2 (lower team total)
  // slot 0: player_dish1 (higher team total)
  // slot 1: player_dish2 (higher team total)
  // slot 0: opponent_dish1 (lower team total)
  queue.add_event(TriggerHook::OnServe, opponent_dish2.id, 1,
                  DishBattleState::TeamSide::Opponent);
  queue.add_event(TriggerHook::OnServe, player_dish1.id, 0,
                  DishBattleState::TeamSide::Player);
  queue.add_event(TriggerHook::OnServe, player_dish2.id, 1,
                  DishBattleState::TeamSide::Player);
  queue.add_event(TriggerHook::OnServe, opponent_dish1.id, 0,
                  DishBattleState::TeamSide::Opponent);

  // Run TriggerDispatchSystem to order events
  TriggerDispatchSystem dispatch;
  if (dispatch.should_run(1.0f / 60.0f)) {
    dispatch.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  // Verify ordering:
  // Expected order:
  // 1. slot 0, Player team (higher total Zing: 15 vs 6)
  // 2. slot 0, Opponent team (lower total Zing)
  // 3. slot 1, Player team (higher total Zing)
  // 4. slot 1, Opponent team (lower total Zing)

  if (queue.events.size() != 4) {
    log_error("TRIGGER_ORDERING_TEST: Expected 4 events, got {}",
              queue.events.size());
    return;
  }

  // Check slot 0 events
  if (queue.events[0].slotIndex != 0) {
    log_error("TRIGGER_ORDERING_TEST: First event should be slot 0, got {}",
              queue.events[0].slotIndex);
    return;
  }
  if (queue.events[0].teamSide != DishBattleState::TeamSide::Player) {
    log_error("TRIGGER_ORDERING_TEST: First event (slot 0) should be Player "
              "team (higher total), got Opponent");
    return;
  }
  if (queue.events[0].sourceEntityId != player_dish1.id) {
    log_error(
        "TRIGGER_ORDERING_TEST: First event should be player_dish1, got {}",
        queue.events[0].sourceEntityId);
    return;
  }

  if (queue.events[1].slotIndex != 0) {
    log_error("TRIGGER_ORDERING_TEST: Second event should be slot 0, got {}",
              queue.events[1].slotIndex);
    return;
  }
  if (queue.events[1].teamSide != DishBattleState::TeamSide::Opponent) {
    log_error("TRIGGER_ORDERING_TEST: Second event (slot 0) should be Opponent "
              "team (lower total), got Player");
    return;
  }
  if (queue.events[1].sourceEntityId != opponent_dish1.id) {
    log_error(
        "TRIGGER_ORDERING_TEST: Second event should be opponent_dish1, got {}",
        queue.events[1].sourceEntityId);
    return;
  }

  // Check slot 1 events
  if (queue.events[2].slotIndex != 1) {
    log_error("TRIGGER_ORDERING_TEST: Third event should be slot 1, got {}",
              queue.events[2].slotIndex);
    return;
  }
  if (queue.events[2].teamSide != DishBattleState::TeamSide::Player) {
    log_error("TRIGGER_ORDERING_TEST: Third event (slot 1) should be Player "
              "team (higher total), got Opponent");
    return;
  }
  if (queue.events[2].sourceEntityId != player_dish2.id) {
    log_error(
        "TRIGGER_ORDERING_TEST: Third event should be player_dish2, got {}",
        queue.events[2].sourceEntityId);
    return;
  }

  if (queue.events[3].slotIndex != 1) {
    log_error("TRIGGER_ORDERING_TEST: Fourth event should be slot 1, got {}",
              queue.events[3].slotIndex);
    return;
  }
  if (queue.events[3].teamSide != DishBattleState::TeamSide::Opponent) {
    log_error("TRIGGER_ORDERING_TEST: Fourth event (slot 1) should be Opponent "
              "team (lower total), got Player");
    return;
  }
  if (queue.events[3].sourceEntityId != opponent_dish2.id) {
    log_error(
        "TRIGGER_ORDERING_TEST: Fourth event should be opponent_dish2, got {}",
        queue.events[3].sourceEntityId);
    return;
  }

  log_info("TRIGGER_ORDERING_TEST: All ordering checks PASSED");
}
