#pragma once

#include "../../components/combat_queue.h"
#include "../../components/dish_battle_state.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../../query.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

TEST(validate_dish_ordering) {
  static int execution_step = 0;

  if (execution_step == 0) {
    log_info("TEST: Starting validate_dish_ordering test");
    execution_step = 1;
  }

  if (execution_step == 1) {
    app.launch_game();
    log_info("TEST: Game launched");

    GameStateManager::Screen current_screen = app.read_current_screen();
    log_info("TEST: Current screen after launch: {}",
             static_cast<int>(current_screen));

    if (current_screen != GameStateManager::Screen::Shop) {
      log_info("TEST: Not on shop screen, navigating there");
      app.wait_for_ui_exists("Play");
      app.click("Play");
      app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
    }
    execution_step = 2;
  }

  if (execution_step == 2) {
    app.expect_screen_is(GameStateManager::Screen::Shop);
    log_info("TEST: On shop screen");

    app.wait_for_frames(5);
    log_info("TEST: Waited for shop initialization");

    app.wait_for_ui_exists("Next Round");
    log_info("TEST: 'Next Round' button found");
    execution_step = 3;
  }

  static bool step3_clicked = false;
  static bool step3_waiting = false;

  if (execution_step == 3) {
    GameStateManager::Screen current_screen = app.read_current_screen();

    if (current_screen == GameStateManager::Screen::Battle) {
      log_info("TEST: Already on battle screen, advancing to step 4");
      step3_waiting = false;
      step3_clicked = true;
      execution_step = 4;
    } else if (step3_waiting &&
               app.wait_state.type == TestApp::WaitState::Screen &&
               app.wait_state.target_screen ==
                   GameStateManager::Screen::Battle) {
      return;
    } else if (step3_clicked) {
      step3_waiting = true;
      app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
      return;
    } else {
      log_info("TEST: Clicking 'Next Round' button");
      app.click("Next Round");
      log_info("TEST: Click completed");

      app.wait_for_frames(3);
      log_info("TEST: Waited 3 frames for export and navigation");

      step3_clicked = true;
      step3_waiting = true;
      app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);

      GameStateManager::Screen final_screen = app.read_current_screen();
      if (final_screen == GameStateManager::Screen::Battle) {
        log_info("TEST: Successfully transitioned to battle screen");
        step3_waiting = false;
        execution_step = 4;
      } else {
        return;
      }
    }
  }

  if (execution_step == 4) {
    app.expect_screen_is(GameStateManager::Screen::Battle);
    log_info("TEST: Confirmed on battle screen");

    static int transition_wait_count = 0;
    if (transition_wait_count < 10) {
      transition_wait_count++;
      app.wait_state.type = TestApp::WaitState::FrameDelay;
      app.wait_state.frame_delay_count = 1;
      throw std::runtime_error("WAIT_FOR_FRAME_DELAY_CONTINUE");
    }
    log_info("TEST: Waited for systems to execute after transition");

    static int init_check_count = 0;
    static bool init_logged_start = false;

    if (!init_logged_start) {
      log_info("TEST: Waiting for battle initialization");
      init_logged_start = true;
    }

    init_check_count++;

    int in_queue = 0;
    int entering = 0;
    int in_combat = 0;
    int onserve_fired = 0;

    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
      const DishBattleState &dbs = e.get<DishBattleState>();
      if (dbs.phase == DishBattleState::Phase::InQueue)
        in_queue++;
      if (dbs.phase == DishBattleState::Phase::Entering)
        entering++;
      if (dbs.phase == DishBattleState::Phase::InCombat)
        in_combat++;
      if (dbs.onserve_fired)
        onserve_fired++;
    }

    if (init_check_count % 5 == 0 || in_combat > 0) {
      log_info("TEST: Battle init check {} - InQueue: {}, Entering: {}, "
               "InCombat: {}, OnServe fired: {}",
               init_check_count, in_queue, entering, in_combat, onserve_fired);
    }

    if (in_combat > 0 || entering > 0) {
      log_info("TEST: Battle initialization complete");
      execution_step = 5;
    } else if (init_check_count >= 2000) {
      log_error("TEST: Battle initialization timeout after {} checks",
                init_check_count);
      app.fail("Battle initialization timeout");
    } else {
      app.wait_state.type = TestApp::WaitState::FrameDelay;
      app.wait_state.frame_delay_count = 1;
      throw std::runtime_error("WAIT_FOR_FRAME_DELAY_CONTINUE");
    }
  }

  if (execution_step == 5) {
    log_info("TEST: Starting dish ordering validation");

    auto combat_queue_opt =
        afterhours::EntityHelper::get_singleton<CombatQueue>();
    if (!combat_queue_opt.get().has<CombatQueue>()) {
      log_error("TEST: CombatQueue singleton not found");
      app.fail("CombatQueue singleton not found");
      return;
    }

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

    if (player_slots.empty() || opponent_slots.empty()) {
      log_error("TEST: No dishes found - player: {}, opponent: {}",
                player_slots.size(), opponent_slots.size());
      app.fail("No dishes found in battle");
      return;
    }

    execution_step = 6;
  }

  if (execution_step == 6) {
    static int last_course_index = -1;
    static std::vector<int> courses_seen;
    static int wait_iteration = 0;
    static bool ordering_validated = false;

    auto combat_queue_opt =
        afterhours::EntityHelper::get_singleton<CombatQueue>();
    if (!combat_queue_opt.get().has<CombatQueue>()) {
      app.fail("CombatQueue singleton not found during tracking");
      return;
    }

    const CombatQueue &cq = combat_queue_opt.get().get<CombatQueue>();

    if (cq.current_index != last_course_index) {
      last_course_index = cq.current_index;
      if (std::find(courses_seen.begin(), courses_seen.end(),
                    cq.current_index) == courses_seen.end()) {
        courses_seen.push_back(cq.current_index);
        log_info("TEST: Course {} started (slot {})", cq.current_index + 1,
                 cq.current_index);
      }

      auto player_dish =
          EQ().whereHasComponent<DishBattleState>()
              .whereInSlotIndex(cq.current_index)
              .whereTeamSide(DishBattleState::TeamSide::Player)
              .whereLambda([](const afterhours::Entity &e) {
                const DishBattleState &dbs = e.get<DishBattleState>();
                return dbs.phase == DishBattleState::Phase::Entering ||
                       dbs.phase == DishBattleState::Phase::InCombat;
              })
              .gen_first();

      auto opponent_dish =
          EQ().whereHasComponent<DishBattleState>()
              .whereInSlotIndex(cq.current_index)
              .whereTeamSide(DishBattleState::TeamSide::Opponent)
              .whereLambda([](const afterhours::Entity &e) {
                const DishBattleState &dbs = e.get<DishBattleState>();
                return dbs.phase == DishBattleState::Phase::Entering ||
                       dbs.phase == DishBattleState::Phase::InCombat;
              })
              .gen_first();

      if (player_dish && opponent_dish) {
        const DishBattleState &player_dbs = player_dish->get<DishBattleState>();
        const DishBattleState &opponent_dbs =
            opponent_dish->get<DishBattleState>();

        log_info("TEST: Course {} - Player slot {} vs Opponent slot {}",
                 cq.current_index + 1, player_dbs.queue_index,
                 opponent_dbs.queue_index);

        if (player_dbs.queue_index != cq.current_index ||
            opponent_dbs.queue_index != cq.current_index) {
          log_error(
              "TEST: Course {} - Mismatch! Expected slots {}, but player is at "
              "{} and opponent is at {}",
              cq.current_index + 1, cq.current_index, player_dbs.queue_index,
              opponent_dbs.queue_index);
          app.fail("Dish slot mismatch - dishes not properly ordered");
          return;
        }
      } else {
        if (wait_iteration < 100) {
          wait_iteration++;
          app.wait_state.type = TestApp::WaitState::FrameDelay;
          app.wait_state.frame_delay_count = 2;
          throw std::runtime_error("WAIT_FOR_FRAME_DELAY_CONTINUE");
        } else {
          log_error("TEST: Could not find dishes for course {}",
                    cq.current_index);
          app.fail("Could not find dishes for current course");
          return;
        }
      }
    }

    std::sort(courses_seen.begin(), courses_seen.end());

    bool courses_in_order = true;
    for (size_t i = 0; i < courses_seen.size(); ++i) {
      if (courses_seen[i] != static_cast<int>(i)) {
        courses_in_order = false;
        break;
      }
    }

    if (!ordering_validated && courses_seen.size() >= 2) {
      std::string courses_str;
      for (size_t i = 0; i < courses_seen.size(); ++i) {
        if (i > 0)
          courses_str += ", ";
        courses_str += std::to_string(courses_seen[i]);
      }
      log_info("TEST: Courses seen so far: [{}]", courses_str);
      if (!courses_in_order) {
        log_error("TEST: Courses not in sequential order!");
        app.fail("Courses are not being processed in sequential order");
        return;
      }
      log_info("TEST: Dish ordering validated - courses processed in order");
      ordering_validated = true;
      execution_step = 7;
      return;
    }

    if (cq.complete) {
      std::string courses_str;
      for (size_t i = 0; i < courses_seen.size(); ++i) {
        if (i > 0)
          courses_str += ", ";
        courses_str += std::to_string(courses_seen[i]);
      }
      log_info("TEST: Battle complete - courses processed: [{}]", courses_str);
      if (!courses_in_order) {
        log_error("TEST: Final check - courses not in sequential order!");
        app.fail("Final courses not in sequential order");
        return;
      }
      log_info("TEST: All courses processed in correct order");
      execution_step = 7;
      return;
    }

    wait_iteration++;
    if (wait_iteration >= 3000) {
      log_error("TEST: Timeout waiting for courses to complete");
      app.fail("Timeout waiting for battle to progress");
      return;
    }

    app.wait_state.type = TestApp::WaitState::FrameDelay;
    app.wait_state.frame_delay_count = 2;
    throw std::runtime_error("WAIT_FOR_FRAME_DELAY_CONTINUE");
  }

  if (execution_step == 7) {
    log_info("TEST: validate_dish_ordering test completed successfully");
    execution_step = 99;
  }

  if (execution_step >= 99) {
    return;
  }
}
