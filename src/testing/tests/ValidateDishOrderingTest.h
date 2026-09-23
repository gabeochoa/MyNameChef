#pragma once

#include "../../components/combat_queue.h"
#include "../../components/dish_battle_state.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../../query.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

TEST(validate_dish_ordering) {
  app.launch_game();
  app.navigate_to_shop();
  app.create_inventory_item(DishType::Potato, 0);
  app.create_inventory_item(DishType::Potato, 1);
  app.create_inventory_item(DishType::Potato, 2);
  app.wait_for_frames(2);
  app.navigate_to_battle();

  app.wait_for_frames(10);

  app.wait_for_battle_initialized(10.0f);

  auto combat_queue_opt =
      afterhours::EntityHelper::get_singleton<CombatQueue>();
  app.expect_singleton_has_component<CombatQueue>(combat_queue_opt,
                                                  "CombatQueue");
  const CombatQueue &cq = combat_queue_opt.get().get<CombatQueue>();

  static std::vector<int> courses_seen;
  static bool ordering_validated = false;

  if (courses_seen.empty() || courses_seen.back() != cq.current_index) {
    if (std::find(courses_seen.begin(), courses_seen.end(),
                  cq.current_index) == courses_seen.end()) {
      courses_seen.push_back(cq.current_index);
      log_info("TEST: Course {} started (slot {})", cq.current_index + 1,
               cq.current_index);
    }

    auto active_phase = [](const afterhours::Entity &e) {
      const DishBattleState &dbs = e.get<DishBattleState>();
      return dbs.phase == DishBattleState::Phase::Entering ||
             dbs.phase == DishBattleState::Phase::InCombat;
    };

    auto player_dish = EQ({.force_merge = true})
                           .whereHasComponent<DishBattleState>()
                           .whereInSlotIndex(cq.current_index)
                           .whereTeamSide(DishBattleState::TeamSide::Player)
                           .whereLambda(active_phase)
                           .gen_first();

    auto opponent_dish = EQ({.force_merge = true})
                             .whereHasComponent<DishBattleState>()
                             .whereInSlotIndex(cq.current_index)
                             .whereTeamSide(DishBattleState::TeamSide::Opponent)
                             .whereLambda(active_phase)
                             .gen_first();

    if (player_dish && opponent_dish) {
      app.expect_count_eq(player_dish->get<DishBattleState>().queue_index,
                          cq.current_index, "player dish queue index");
      app.expect_count_eq(opponent_dish->get<DishBattleState>().queue_index,
                          cq.current_index, "opponent dish queue index");
    }
  }

  for (size_t i = 1; i < courses_seen.size(); ++i) {
    app.expect_count_gt(courses_seen[i], courses_seen[i - 1],
                        "course index order");
  }

  if (!ordering_validated && courses_seen.size() >= 2) {
    ordering_validated = true;
    log_info("TEST: validate_dish_ordering observed ordered courses");
    return;
  }

  if (cq.complete ||
      app.read_current_screen() == GameStateManager::Screen::Results) {
    log_info("TEST: validate_dish_ordering test completed successfully");
    return;
  }

  app.wait_state.type = TestApp::WaitState::FrameDelay;
  app.wait_state.frame_delay_count = 2;
  app.wait_state.operation_id = 0;
  app.yield([&app]() {
    app.test_resuming = true;
    TestRegistry::get().run_test(app.current_test_name, app);
  });
}
