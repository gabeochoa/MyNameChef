#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/battle_team_data.h"
#include "../../components/combat_queue.h"
#include "../../components/dish_battle_state.h"
#include "../../components/is_dish.h"
#include "../../components/is_drop_slot.h"
#include "../../components/is_inventory_item.h"
#include "../../components/is_shop_item.h"
#include "../../components/transform.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../query.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

using namespace afterhours;

TEST(validate_entity_query_without_manual_merge) {
  app.launch_game();

  app.once([&] {
    Entity &dish = EntityHelper::createEntity();
    dish.addComponent<IsDish>(DishType::Potato);
    dish.addComponent<Transform>(vec2{100.0f, 100.0f}, vec2{80.0f, 80.0f});
    app.expect_true(dish.has<IsDish>(), "dish has IsDish component");
    app.expect_true(dish.has<Transform>(), "dish has Transform component");
    app.expect_eq(static_cast<int>(dish.get<IsDish>().type),
                  static_cast<int>(DishType::Potato), "dish type is Potato");

    for (int i = 0; i < 3; ++i) {
      Entity &e = EntityHelper::createEntity();
      e.addComponent<IsDish>(DishType::Potato);
      e.addComponent<Transform>(vec2{100.0f + i * 100.0f, 100.0f},
                                vec2{80.0f, 80.0f});
      app.set_test_int("created_id_" + std::to_string(i), e.id);
    }
  });

  app.wait_for_frames(1);

  int found_count = 0;
  for (int i = 0; i < 3; ++i) {
    EntityID id = app.get_test_int("created_id_" + std::to_string(i)).value();
    if (EQ({.force_merge = true}).whereID(id).gen_first().has_value()) {
      found_count++;
    }
  }
  app.expect_eq(found_count, 3, "all 3 created entities found after one frame");

  app.navigate_to_shop();
  app.wait_for_frames(3);

  app.once([&] {
    int shop_items = static_cast<int>(EQ({.force_merge = true})
                                          .whereHasComponent<IsShopItem>()
                                          .gen_count());
    app.expect_true(shop_items > 0, "shop items found after system runs");
  });

  app.once([&] {
    GameStateManager::get().to_battle();
    Entity &cq_entity = EntityHelper::get_singleton<CombatQueue>().get();
    app.expect_false(cq_entity.has<BattleTeamDataPlayer>(),
                     "BattleTeamDataPlayer should not exist yet");
    BattleTeamDataPlayer player_data;
    player_data.team.push_back({DishType::Potato, 0, 1});
    player_data.instantiated = false;
    cq_entity.addComponent<BattleTeamDataPlayer>(std::move(player_data));

    app.expect_false(cq_entity.has<BattleTeamDataOpponent>(),
                     "BattleTeamDataOpponent should not exist yet");
    BattleTeamDataOpponent opponent_data;
    opponent_data.team.push_back({DishType::Potato, 0, 1});
    opponent_data.instantiated = false;
    cq_entity.addComponent<BattleTeamDataOpponent>(std::move(opponent_data));
  });
  app.wait_for_screen(GameStateManager::Screen::Battle, 5.0f);
  app.wait_for_frames(5);

  app.once([&] {
    int player_dishes = 0;
    int opponent_dishes = 0;
    for (Entity &entity :
         EQ({.force_merge = true}).whereHasComponent<DishBattleState>().gen()) {
      const DishBattleState &dbs = entity.get<DishBattleState>();
      if (dbs.team_side == DishBattleState::TeamSide::Player) {
        player_dishes++;
      } else {
        opponent_dishes++;
      }
    }
    app.expect_true(player_dishes > 0, "player dishes found after system loop");
    app.expect_true(opponent_dishes > 0,
                    "opponent dishes found after system loop");
  });

  app.once([&] {
    GameStateManager::get().set_next_screen(GameStateManager::Screen::Shop);
  });
  app.wait_for_screen(GameStateManager::Screen::Shop, 5.0f);
  app.wait_for_frames(2);

  int inventory_slots = 0;
  int sell_slot = 0;
  int shop_slots = 0;
  for (Entity &entity :
       EQ({.force_merge = true}).whereHasComponent<IsDropSlot>().gen()) {
    const IsDropSlot &slot = entity.get<IsDropSlot>();
    if (slot.accepts_inventory_items && slot.accepts_shop_items) {
      inventory_slots++;
    } else if (slot.accepts_inventory_items && !slot.accepts_shop_items) {
      sell_slot++;
    } else if (!slot.accepts_inventory_items && slot.accepts_shop_items) {
      shop_slots++;
    }
  }
  app.expect_eq(inventory_slots, 7, "7 inventory slots found");
  app.expect_eq(sell_slot, 1, "1 sell slot found");
  app.expect_eq(shop_slots, 7, "7 shop slots found");

  app.once([&] {
    Entity &e = EntityHelper::createEntity();
    e.addComponent<IsDish>(DishType::Salmon);
    e.addComponent<Transform>(vec2{200.0f, 200.0f}, vec2{80.0f, 80.0f});
    OptEntity found_opt = EQ({.force_merge = true}).whereID(e.id).gen_first();
    app.expect_true(found_opt.has_value(), "entity found with force_merge");
    app.expect_eq(static_cast<int>(found_opt.asE().get<IsDish>().type),
                  static_cast<int>(DishType::Salmon), "entity type matches");
  });
}
