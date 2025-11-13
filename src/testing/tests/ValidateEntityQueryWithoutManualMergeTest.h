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
  log_info("VALIDATION_TEST: Starting entity query validation tests");

  // Test 1: Use Entity Reference Directly (No Query Needed)
  log_info("VALIDATION_TEST: Test 1 - Use entity reference directly");
  app.launch_game();

  // Create entity and use reference directly
  auto &dish = EntityHelper::createEntity();
  dish.addComponent<IsDish>(DishType::Potato);
  dish.addComponent<Transform>(vec2{100.0f, 100.0f}, vec2{80.0f, 80.0f});

  // Use reference directly - no merge or query needed
  app.expect_true(dish.has<IsDish>(), "dish has IsDish component");
  app.expect_true(dish.has<Transform>(), "dish has Transform component");
  app.expect_eq(static_cast<int>(dish.get<IsDish>().type),
                static_cast<int>(DishType::Potato), "dish type is Potato");

  log_info("VALIDATION_TEST: Test 1 PASSED - Reference works directly");

  // Test 2: Entities Queryable After One Frame
  log_info("VALIDATION_TEST: Test 2 - Entities queryable after one frame");

  // Create entities directly in test
  std::vector<EntityID> created_ids;
  for (int i = 0; i < 3; ++i) {
    auto &e = EntityHelper::createEntity();
    e.addComponent<IsDish>(DishType::Potato);
    e.addComponent<Transform>(vec2{100.0f + i * 100.0f, 100.0f},
                              vec2{80.0f, 80.0f});
    created_ids.push_back(e.id);
  }

  // Wait one frame to let system loop merge automatically
  app.wait_for_frames(1);

  // Query entities using normal EntityQuery (no force_merge)
  int found_count = 0;
  for (Entity &entity : EQ().whereHasComponent<IsDish>().gen()) {
    if (std::find(created_ids.begin(), created_ids.end(), entity.id) !=
        created_ids.end()) {
      found_count++;
    }
  }

  app.expect_eq(found_count, 3, "all 3 created entities found after one frame");
  log_info(
      "VALIDATION_TEST: Test 2 PASSED - Entities queryable after system loop");

  // Test 3: Battle Team Entities Queryable After System Loop
  log_info("VALIDATION_TEST: Test 3 - Battle team entities queryable after "
           "system loop");

  // Navigate to shop first
  app.navigate_to_shop();

  // Create battle team data
  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  // Create mock battle team data
  auto cq_ref = EntityHelper::get_singleton<CombatQueue>();
  auto &cq_entity = cq_ref.get();
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

  // Wait for systems to instantiate teams
  app.wait_for_frames(5);

  // Query for battle dishes
  int player_dishes = 0;
  int opponent_dishes = 0;
  for (Entity &entity : EQ().whereHasComponent<DishBattleState>().gen()) {
    const DishBattleState &dbs = entity.get<DishBattleState>();
    if (dbs.team_side == DishBattleState::TeamSide::Player) {
      player_dishes++;
    } else if (dbs.team_side == DishBattleState::TeamSide::Opponent) {
      opponent_dishes++;
    }
  }

  app.expect_true(player_dishes > 0, "player dishes found after system loop");
  app.expect_true(opponent_dishes > 0,
                  "opponent dishes found after system loop");
  log_info("VALIDATION_TEST: Test 3 PASSED - Battle team entities queryable");

  // Test 4: Inventory Slots Queryable After Generation
  log_info(
      "VALIDATION_TEST: Test 4 - Inventory slots queryable after generation");

  // Navigate to shop screen (triggers GenerateInventorySlots)
  GameStateManager::get().set_next_screen(GameStateManager::Screen::Shop);
  GameStateManager::get().update_screen();
  app.wait_for_frames(2); // Wait for systems to generate slots

  // Query for inventory slots
  int inventory_slots = 0;
  int sell_slot = 0;
  for (Entity &entity : EQ().whereHasComponent<IsDropSlot>().gen()) {
    const IsDropSlot &slot = entity.get<IsDropSlot>();
    if (slot.accepts_inventory_items && slot.accepts_shop_items) {
      inventory_slots++;
    } else if (slot.accepts_inventory_items && !slot.accepts_shop_items) {
      sell_slot++;
    }
  }

  app.expect_eq(inventory_slots, 7, "7 inventory slots found");
  app.expect_eq(sell_slot, 1, "1 sell slot found");
  log_info("VALIDATION_TEST: Test 4 PASSED - Inventory slots queryable");

  // Test 5: Shop Slots Queryable After Generation
  log_info("VALIDATION_TEST: Test 5 - Shop slots queryable after generation");

  // Shop slots should already be generated (we're on shop screen)
  int shop_slots = 0;
  for (Entity &entity : EQ().whereHasComponent<IsDropSlot>().gen()) {
    const IsDropSlot &slot = entity.get<IsDropSlot>();
    if (!slot.accepts_inventory_items && slot.accepts_shop_items) {
      shop_slots++;
    }
  }

  app.expect_eq(shop_slots, 7, "7 shop slots found");
  log_info("VALIDATION_TEST: Test 5 PASSED - Shop slots queryable");

  // Test 6: force_merge Works When Needed
  log_info("VALIDATION_TEST: Test 6 - force_merge works when needed");

  // Create entities directly in test
  EntityID force_merge_test_id = -1;
  {
    auto &e = EntityHelper::createEntity();
    e.addComponent<IsDish>(DishType::Salmon);
    e.addComponent<Transform>(vec2{200.0f, 200.0f}, vec2{80.0f, 80.0f});
    force_merge_test_id = e.id;
  }

  // Query immediately using force_merge (without waiting a frame)
  auto found_opt =
      EQ({.force_merge = true}).whereID(force_merge_test_id).gen_first();
  app.expect_true(found_opt.has_value(), "entity found with force_merge");
  app.expect_eq(static_cast<int>(found_opt.asE().get<IsDish>().type),
                static_cast<int>(DishType::Salmon), "entity type matches");

  log_info("VALIDATION_TEST: Test 6 PASSED - force_merge works");

  // Test 7: Shop Items Queryable After System Runs
  log_info("VALIDATION_TEST: Test 7 - Shop items queryable after system runs");

  // Wait for shop systems to populate items
  app.wait_for_frames(3);

  // Query for shop items
  int shop_items = 0;
  for (Entity &entity : EQ().whereHasComponent<IsShopItem>().gen()) {
    shop_items++;
  }

  app.expect_true(shop_items > 0, "shop items found after system runs");
  log_info("VALIDATION_TEST: Test 7 PASSED - Shop items queryable");

  log_info("VALIDATION_TEST: All validation tests PASSED");
}
