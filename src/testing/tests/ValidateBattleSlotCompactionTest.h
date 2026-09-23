#pragma once

#include "../../components/dish_battle_state.h"
#include "../../components/is_dish.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../test_macros.h"
#include <afterhours/ah.h>
#include <algorithm>
#include <vector>

TEST(validate_battle_slot_compaction) {
  app.delete_save_file();
  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");

  app.run_once("inventory_with_gaps", [&app]() {
    app.create_inventory_item(DishType::Potato, 0);
    app.create_inventory_item(DishType::Burger, 2);
    app.create_inventory_item(DishType::Pizza, 5);
  });
  app.wait_for_frames(2);

  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_battle_initialized(10.0f);

  app.run_once("check_slots", [&app]() {
    std::vector<int> player_slots;
    for (afterhours::Entity &entity :
         afterhours::EntityQuery({.force_merge = true})
             .whereHasComponent<DishBattleState>()
             .whereHasComponent<IsDish>()
             .gen()) {
      const DishBattleState &dbs = entity.get<DishBattleState>();
      if (dbs.team_side == DishBattleState::TeamSide::Player) {
        player_slots.push_back(dbs.queue_index);
      }
    }
    std::sort(player_slots.begin(), player_slots.end());

    app.expect_count_eq(static_cast<int>(player_slots.size()), 3,
                        "player battle dishes");
    for (size_t i = 0; i < player_slots.size(); ++i) {
      app.expect_eq(player_slots[i], static_cast<int>(i),
                    "battle slot " + std::to_string(i));
    }
  });

  log_info("BATTLE_SLOT_COMPACTION_TEST: slots are sequential from 0");
}
