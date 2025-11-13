#pragma once

#include "../../components/dish_battle_state.h"
#include "../../components/is_dish.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../test_macros.h"
#include <afterhours/ah.h>
#include <algorithm>
#include <string>
#include <vector>

TEST(validate_battle_slot_compaction) {
  log_info("BATTLE_SLOT_COMPACTION_TEST: Starting battle slot compaction "
           "validation");

  app.launch_game();
  log_info("BATTLE_SLOT_COMPACTION_TEST: Game launched");

  GameStateManager::Screen current_screen = app.read_current_screen();
  if (current_screen != GameStateManager::Screen::Shop) {
    app.wait_for_ui_exists("Play");
    app.click("Play");
    app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  }
  app.expect_screen_is(GameStateManager::Screen::Shop);

  app.wait_for_frames(5);

  app.create_inventory_item(DishType::Potato, 0);
  app.wait_for_frames(2);
  app.create_inventory_item(DishType::Burger, 2);
  app.wait_for_frames(2);
  app.create_inventory_item(DishType::Pizza, 5);
  app.wait_for_frames(2);

  log_info("BATTLE_SLOT_COMPACTION_TEST: Created inventory with gaps - slots "
           "0, 2, 5");

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

  app.expect_screen_is(GameStateManager::Screen::Battle);

  app.wait_for_frames(30);

  std::vector<int> player_slots;
  for (afterhours::Entity &entity : afterhours::EntityQuery()
                                        .whereHasComponent<DishBattleState>()
                                        .whereHasComponent<IsDish>()
                                        .gen()) {
    const DishBattleState &dbs = entity.get<DishBattleState>();
    if (dbs.team_side == DishBattleState::TeamSide::Player) {
      player_slots.push_back(dbs.queue_index);
    }
  }

  std::sort(player_slots.begin(), player_slots.end());

  std::string slots_str;
  for (size_t i = 0; i < player_slots.size(); ++i) {
    if (i > 0)
      slots_str += ", ";
    slots_str += std::to_string(player_slots[i]);
  }
  log_info("BATTLE_SLOT_COMPACTION_TEST: Player battle slots: [{}]", slots_str);

  app.expect_count_eq(static_cast<int>(player_slots.size()), 3,
                      "should have 3 player dishes");

  app.expect_eq(player_slots[0], 0, "first dish should be at slot 0");
  app.expect_eq(player_slots[1], 1, "second dish should be at slot 1");
  app.expect_eq(player_slots[2], 2, "third dish should be at slot 2");

  bool has_gaps = false;
  for (size_t i = 1; i < player_slots.size(); ++i) {
    if (player_slots[i] != player_slots[i - 1] + 1) {
      has_gaps = true;
      break;
    }
  }

  app.expect_false(has_gaps, "battle slots should have no gaps");

  log_info("BATTLE_SLOT_COMPACTION_TEST: ✅ Battle slot compaction validated - "
           "slots are sequential starting at 0");
}
