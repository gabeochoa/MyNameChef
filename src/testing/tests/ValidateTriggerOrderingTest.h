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
#include "../test_app.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

TEST(validate_trigger_ordering) {
  log_info("TRIGGER_ORDERING_TEST: Testing trigger event ordering logic");

  app.launch_game();

  afterhours::EntityID player_dish1_id =
      app.create_dish(DishType::Salmon)
          .on_team(DishBattleState::TeamSide::Player)
          .at_slot(0)
          .in_phase(DishBattleState::Phase::InQueue)
          .commit();

  afterhours::EntityID player_dish2_id =
      app.create_dish(DishType::Salmon)
          .on_team(DishBattleState::TeamSide::Player)
          .at_slot(1)
          .in_phase(DishBattleState::Phase::InQueue)
          .commit();

  afterhours::EntityID opponent_dish1_id =
      app.create_dish(DishType::Potato)
          .on_team(DishBattleState::TeamSide::Opponent)
          .at_slot(0)
          .in_phase(DishBattleState::Phase::InQueue)
          .commit();

  afterhours::EntityID opponent_dish2_id =
      app.create_dish(DishType::Salmon)
          .on_team(DishBattleState::TeamSide::Opponent)
          .at_slot(1)
          .in_phase(DishBattleState::Phase::InQueue)
          .commit();

  app.wait_for_frames(2);

  for (afterhours::Entity &e :
       EQ({.force_merge = true}).whereHasComponent<IsDish>().gen()) {
    app.expect_true(e.has<CombatStats>(),
                    "dish " + std::to_string(e.id) + " has CombatStats");
  }

  app.fire_trigger(TriggerHook::OnServe, opponent_dish2_id, 1,
                   DishBattleState::TeamSide::Opponent);
  app.fire_trigger(TriggerHook::OnServe, player_dish1_id, 0,
                   DishBattleState::TeamSide::Player);
  app.fire_trigger(TriggerHook::OnServe, player_dish2_id, 1,
                   DishBattleState::TeamSide::Player);
  app.fire_trigger(TriggerHook::OnServe, opponent_dish1_id, 0,
                   DishBattleState::TeamSide::Opponent);

  afterhours::Entity &tq_entity =
      afterhours::EntityHelper::get_singleton<TriggerQueue>().get();
  TriggerQueue &queue = tq_entity.get<TriggerQueue>();

  TriggerDispatchSystem::sort_events(queue);

  app.expect_count_eq(static_cast<int>(queue.events.size()), 4,
                      "event count after dispatch ordering");

  const std::vector<afterhours::EntityID> expected_order = {
      player_dish1_id, opponent_dish1_id, player_dish2_id, opponent_dish2_id};
  const std::vector<int> expected_slots = {0, 0, 1, 1};
  const std::vector<DishBattleState::TeamSide> expected_teams = {
      DishBattleState::TeamSide::Player, DishBattleState::TeamSide::Opponent,
      DishBattleState::TeamSide::Player, DishBattleState::TeamSide::Opponent};

  for (size_t i = 0; i < expected_order.size(); ++i) {
    const std::string idx = std::to_string(i);
    app.expect_count_eq(queue.events[i].slotIndex, expected_slots[i],
                        "event " + idx + " slot index");
    app.expect_true(queue.events[i].teamSide == expected_teams[i],
                    "event " + idx + " team side");
    app.expect_count_eq(queue.events[i].sourceEntityId,
                        static_cast<int>(expected_order[i]),
                        "event " + idx + " source entity");
  }

  log_info("TRIGGER_ORDERING_TEST: All ordering checks PASSED");
}
