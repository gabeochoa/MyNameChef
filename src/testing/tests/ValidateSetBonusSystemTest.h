#pragma once

#include "../../components/applied_set_bonuses.h"
#include "../../components/battle_synergy_counts.h"
#include "../../components/cuisine_tag.h"
#include "../../components/dish_battle_state.h"
#include "../../components/dish_level.h"
#include "../../components/is_dish.h"
#include "../../components/persistent_combat_modifiers.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../shop.h"
#include "../../systems/ApplySetBonusesSystem.h"
#include "../../systems/BattleSynergyCountingSystem.h"
#include "../test_app.h"
#include "../test_macros.h"
#include <afterhours/ah.h>
#include <magic_enum/magic_enum.hpp>
#include <map>

namespace ValidateSetBonusSystemTestHelpers {

static afterhours::RefEntity get_or_create_battle_synergy_counts() {
  afterhours::RefEntity entity =
      afterhours::EntityHelper::get_singleton<BattleSynergyCounts>();
  afterhours::Entity &e = entity.get();
  if (!e.has<BattleSynergyCounts>()) {
    e.addComponent<BattleSynergyCounts>();
    afterhours::EntityHelper::registerSingleton<BattleSynergyCounts>(e);
  }
  return entity;
}

static afterhours::RefEntity get_or_create_applied_set_bonuses() {
  afterhours::RefEntity entity =
      afterhours::EntityHelper::get_singleton<AppliedSetBonuses>();
  afterhours::Entity &e = entity.get();
  if (!e.has<AppliedSetBonuses>()) {
    e.addComponent<AppliedSetBonuses>();
    afterhours::EntityHelper::registerSingleton<AppliedSetBonuses>(e);
  }
  return entity;
}

static void reset_test_state() {
  // Clear applied bonuses from previous tests
  afterhours::RefEntity applied_entity = get_or_create_applied_set_bonuses();
  AppliedSetBonuses &applied = applied_entity.get().get<AppliedSetBonuses>();
  applied.applied_cuisine.clear();

  // Clear synergy counts
  afterhours::RefEntity counts_entity = get_or_create_battle_synergy_counts();
  BattleSynergyCounts &counts = counts_entity.get().get<BattleSynergyCounts>();
  counts.player_cuisine_counts.clear();
  counts.opponent_cuisine_counts.clear();
}

static void navigate_to_battle_screen(TestApp &app) {
  // Check if already on Battle screen
  GameStateManager::get().update_screen();
  auto &gsm = GameStateManager::get();
  if (gsm.active_screen == GameStateManager::Screen::Battle) {
    return; // Already on battle screen
  }

  // Navigate to Shop if not already there
  if (gsm.active_screen != GameStateManager::Screen::Shop) {
    app.wait_for_ui_exists("Play");
    app.click("Play");
    app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  }

  // Ensure inventory has dishes (required to proceed)
  const auto inventory = app.read_player_inventory();
  if (inventory.empty()) {
    app.create_inventory_item(DishType::Potato, 0);
    app.wait_for_frames(2);
  }

  // Click "Next Round" to navigate to battle
  app.wait_for_ui_exists("Next Round");
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_frames(5); // Let battle initialize
}

static void trigger_synergy_systems() {
  // Clear synergy counts first to ensure clean state
  afterhours::RefEntity counts_entity = get_or_create_battle_synergy_counts();
  BattleSynergyCounts &counts = counts_entity.get().get<BattleSynergyCounts>();
  counts.player_cuisine_counts.clear();
  counts.opponent_cuisine_counts.clear();

  // Clear applied bonuses to allow re-application
  afterhours::RefEntity applied_entity = get_or_create_applied_set_bonuses();
  AppliedSetBonuses &applied = applied_entity.get().get<AppliedSetBonuses>();
  applied.applied_cuisine.clear();

  // Manually trigger synergy counting and bonus application systems
  // after creating dishes, since they only run once at battle start.
  // Create new instances to reset their internal state flags.
  BattleSynergyCountingSystem countingSystem;
  // Force run by temporarily setting calculated to false
  countingSystem.calculated = false;
  if (countingSystem.should_run(1.0f / 60.0f)) {
    countingSystem.once(1.0f / 60.0f);
  }

  ApplySetBonusesSystem bonusSystem;
  // Force run by temporarily setting applied to false
  bonusSystem.applied = false;
  if (bonusSystem.should_run(1.0f / 60.0f)) {
    bonusSystem.once(1.0f / 60.0f);
  }
}

static void clear_existing_battle_dishes() {
  // Clear any existing dishes from the normal battle flow
  // so we can test with only our test dishes
  for (afterhours::Entity &entity : afterhours::EntityQuery()
                                        .whereHasComponent<IsDish>()
                                        .whereHasComponent<DishBattleState>()
                                        .gen()) {
    entity.cleanup = true;
  }
  afterhours::EntityHelper::cleanup();
  afterhours::EntityHelper::merge_entity_arrays();
}

} // namespace ValidateSetBonusSystemTestHelpers

TEST(validate_set_bonus_american_2_piece) {
  using namespace ValidateSetBonusSystemTestHelpers;

  app.launch_game();
  navigate_to_battle_screen(app);

  // Clear existing battle dishes so we only test with our dishes
  clear_existing_battle_dishes();

  // Create 2 American dishes using TestDishBuilder
  auto dish1_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(0)
                      .with_cuisine_tag(CuisineTagType::American)
                      .commit();

  auto dish2_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(1)
                      .with_cuisine_tag(CuisineTagType::American)
                      .commit();

  // Create a non-American dish (should not get bonus)
  auto dish3_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(2)
                      .commit();

  // Merge entities and trigger synergy systems
  afterhours::EntityHelper::merge_entity_arrays();
  trigger_synergy_systems();

  app.wait_for_frames(5);

  // Validate using TestApp helpers
  app.expect_synergy_count(CuisineTagType::American, 2,
                           DishBattleState::TeamSide::Player);
  app.expect_modifier(dish1_id, 0, 1);
  app.expect_modifier(dish2_id, 0, 1);
  app.expect_modifier(dish3_id, 0, 1);
}

TEST(validate_set_bonus_american_4_piece) {
  using namespace ValidateSetBonusSystemTestHelpers;

  app.launch_game();
  navigate_to_battle_screen(app);

  // Clear existing battle dishes so we only test with our dishes
  clear_existing_battle_dishes();

  // Create 4 American dishes using TestDishBuilder
  auto dish1_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(0)
                      .with_cuisine_tag(CuisineTagType::American)
                      .commit();

  auto dish2_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(1)
                      .with_cuisine_tag(CuisineTagType::American)
                      .commit();

  auto dish3_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(2)
                      .with_cuisine_tag(CuisineTagType::American)
                      .commit();

  auto dish4_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(3)
                      .with_cuisine_tag(CuisineTagType::American)
                      .commit();

  // Merge entities and trigger synergy systems
  afterhours::EntityHelper::merge_entity_arrays();
  trigger_synergy_systems();

  app.wait_for_frames(5);

  // Validate using TestApp helpers
  // 2-piece + 4-piece = +1 +2 = +3 Body total
  app.expect_synergy_count(CuisineTagType::American, 4,
                           DishBattleState::TeamSide::Player);
  app.expect_modifier(dish1_id, 0, 3);
  app.expect_modifier(dish2_id, 0, 3);
  app.expect_modifier(dish3_id, 0, 3);
  app.expect_modifier(dish4_id, 0, 3);
}

TEST(validate_set_bonus_american_6_piece) {
  using namespace ValidateSetBonusSystemTestHelpers;

  app.launch_game();
  navigate_to_battle_screen(app);

  // Clear existing battle dishes so we only test with our dishes
  clear_existing_battle_dishes();

  // Create 6 American dishes using TestDishBuilder
  auto dish1_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(0)
                      .with_cuisine_tag(CuisineTagType::American)
                      .commit();

  auto dish2_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(1)
                      .with_cuisine_tag(CuisineTagType::American)
                      .commit();

  auto dish3_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(2)
                      .with_cuisine_tag(CuisineTagType::American)
                      .commit();

  auto dish4_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(3)
                      .with_cuisine_tag(CuisineTagType::American)
                      .commit();

  auto dish5_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(4)
                      .with_cuisine_tag(CuisineTagType::American)
                      .commit();

  auto dish6_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(5)
                      .with_cuisine_tag(CuisineTagType::American)
                      .commit();

  // Merge entities and trigger synergy systems
  afterhours::EntityHelper::merge_entity_arrays();
  trigger_synergy_systems();

  app.wait_for_frames(5);

  // Validate using TestApp helpers
  // 2-piece + 4-piece + 6-piece = +1 +2 +3 = +6 Body total
  app.expect_synergy_count(CuisineTagType::American, 6,
                           DishBattleState::TeamSide::Player);
  app.expect_modifier(dish1_id, 0, 6);
  app.expect_modifier(dish2_id, 0, 6);
  app.expect_modifier(dish3_id, 0, 6);
  app.expect_modifier(dish4_id, 0, 6);
  app.expect_modifier(dish5_id, 0, 6);
  app.expect_modifier(dish6_id, 0, 6);
}

TEST(validate_set_bonus_no_synergy) {
  using namespace ValidateSetBonusSystemTestHelpers;

  app.launch_game();
  navigate_to_battle_screen(app);

  // Clear existing battle dishes so we only test with our dishes
  clear_existing_battle_dishes();

  // Create dishes without matching cuisine tags using TestDishBuilder
  auto dish1_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(0)
                      .commit();

  auto dish2_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(1)
                      .commit();

  // Create dish with different cuisine (Thai, not American)
  auto thai_dish_id = app.create_dish(DishType::Potato)
                          .on_team(DishBattleState::TeamSide::Player)
                          .at_slot(2)
                          .with_cuisine_tag(CuisineTagType::Thai)
                          .commit();

  // Merge entities and trigger synergy systems
  afterhours::EntityHelper::merge_entity_arrays();
  trigger_synergy_systems();

  app.wait_for_frames(5);

  // Validate using TestApp helpers
  // Should have 0 American synergy count
  app.expect_synergy_count(CuisineTagType::American, 0,
                           DishBattleState::TeamSide::Player);
  // No bonuses should be applied
  app.expect_modifier(dish1_id, 0, 0);
  app.expect_modifier(dish2_id, 0, 0);
  app.expect_modifier(thai_dish_id, 0, 0);
}
