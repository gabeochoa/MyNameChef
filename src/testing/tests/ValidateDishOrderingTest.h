#pragma once

#include "../../components/combat_queue.h"
#include "../../components/dish_battle_state.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../../query.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

TEST(validate_dish_ordering) {
  log_info("TEST: Starting validate_dish_ordering test");

  app.launch_game();

  GameStateManager::Screen current_screen = app.read_current_screen();
  if (current_screen != GameStateManager::Screen::Shop) {
    app.wait_for_ui_exists("Play");
    app.click("Play");
    app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  }
  app.expect_screen_is(GameStateManager::Screen::Shop);

  app.wait_for_frames(5);
  app.wait_for_ui_exists("Next Round");

  GameStateManager::Screen battle_screen = app.read_current_screen();
  if (battle_screen != GameStateManager::Screen::Battle) {
    app.click("Next Round");
    app.wait_for_frames(3);
    app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  }
  app.expect_screen_is(GameStateManager::Screen::Battle);

  app.wait_for_frames(10);

  app.wait_for_battle_initialized(10.0f);

  auto combat_queue_opt =
      afterhours::EntityHelper::get_singleton<CombatQueue>();
  app.expect_singleton_has_component<CombatQueue>(combat_queue_opt,
                                                  "CombatQueue");

  const CombatQueue &cq = combat_queue_opt.get().get<CombatQueue>();
  log_info("TEST: CombatQueue - current_index: {}, total_courses: {}, "
           "complete: {}",
           cq.current_index, cq.total_courses, cq.complete);

  std::vector<int> player_slots;
  std::vector<int> opponent_slots;

  for (afterhours::Entity &e :
       afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
    const DishBattleState &dbs = e.get<DishBattleState>();
    if (dbs.team_side == DishBattleState::TeamSide::Player) {
      player_slots.push_back(dbs.queue_index);
    } else {
      opponent_slots.push_back(dbs.queue_index);
    }
  }

  std::sort(player_slots.begin(), player_slots.end());
  std::sort(opponent_slots.begin(), opponent_slots.end());

  std::string player_slots_str;
  for (size_t i = 0; i < player_slots.size(); ++i) {
    if (i > 0)
      player_slots_str += ", ";
    player_slots_str += std::to_string(player_slots[i]);
  }
  std::string opponent_slots_str;
  for (size_t i = 0; i < opponent_slots.size(); ++i) {
    if (i > 0)
      opponent_slots_str += ", ";
    opponent_slots_str += std::to_string(opponent_slots[i]);
  }

  log_info("TEST: Player dishes at slots: [{}]", player_slots_str);
  log_info("TEST: Opponent dishes at slots: [{}]", opponent_slots_str);

  app.expect_count_gt(static_cast<int>(player_slots.size()), 0,
                      "player dishes");
  app.expect_count_gt(static_cast<int>(opponent_slots.size()), 0,
                      "opponent dishes");

  static int last_course_index = -1;
  static std::vector<int> courses_seen;
  static bool ordering_validated = false;

  auto combat_queue_opt_tracking =
      afterhours::EntityHelper::get_singleton<CombatQueue>();
  app.expect_singleton_has_component<CombatQueue>(combat_queue_opt_tracking,
                                                  "CombatQueue");

  const CombatQueue &cq_tracking =
      combat_queue_opt_tracking.get().get<CombatQueue>();

  if (cq_tracking.current_index != last_course_index) {
    last_course_index = cq_tracking.current_index;
    if (std::find(courses_seen.begin(), courses_seen.end(),
                  cq_tracking.current_index) == courses_seen.end()) {
      courses_seen.push_back(cq_tracking.current_index);
      log_info("TEST: Course {} started (slot {})",
               cq_tracking.current_index + 1, cq_tracking.current_index);
    }

    auto player_dish =
        EQ().whereHasComponent<DishBattleState>()
            .whereInSlotIndex(cq_tracking.current_index)
            .whereTeamSide(DishBattleState::TeamSide::Player)
            .whereLambda([](const afterhours::Entity &e) {
              const DishBattleState &dbs = e.get<DishBattleState>();
              return dbs.phase == DishBattleState::Phase::Entering ||
                     dbs.phase == DishBattleState::Phase::InCombat;
            })
            .gen_first();

    auto opponent_dish =
        EQ().whereHasComponent<DishBattleState>()
            .whereInSlotIndex(cq_tracking.current_index)
            .whereTeamSide(DishBattleState::TeamSide::Opponent)
            .whereLambda([](const afterhours::Entity &e) {
              const DishBattleState &dbs = e.get<DishBattleState>();
              return dbs.phase == DishBattleState::Phase::Entering ||
                     dbs.phase == DishBattleState::Phase::InCombat;
            })
            .gen_first();

    app.expect_count_gt((player_dish ? 1 : 0), 0, "player dish for course");
    app.expect_count_gt((opponent_dish ? 1 : 0), 0, "opponent dish for course");

    if (player_dish && opponent_dish) {
      const DishBattleState &player_dbs = player_dish->get<DishBattleState>();
      const DishBattleState &opponent_dbs =
          opponent_dish->get<DishBattleState>();

      log_info("TEST: Course {} - Player slot {} vs Opponent slot {}",
               cq_tracking.current_index + 1, player_dbs.queue_index,
               opponent_dbs.queue_index);

      app.expect_count_eq(player_dbs.queue_index, cq_tracking.current_index,
                          "player dish queue index");
      app.expect_count_eq(opponent_dbs.queue_index, cq_tracking.current_index,
                          "opponent dish queue index");
    }
  }

  std::sort(courses_seen.begin(), courses_seen.end());

  for (size_t i = 0; i < courses_seen.size(); ++i) {
    app.expect_count_eq(courses_seen[i], static_cast<int>(i),
                        "course index order");
  }

  if (!ordering_validated && courses_seen.size() >= 2) {
    ordering_validated = true;
    return;
  }

  if (cq_tracking.complete) {
    log_info("TEST: validate_dish_ordering test completed successfully");
    return;
  }

  app.wait_state.type = TestApp::WaitState::FrameDelay;
  app.wait_state.frame_delay_count = 2;
  throw std::runtime_error("WAIT_FOR_FRAME_DELAY_CONTINUE");
}
