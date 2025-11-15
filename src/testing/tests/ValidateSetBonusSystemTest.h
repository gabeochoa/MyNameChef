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

static afterhours::Entity &create_test_dish_with_cuisine(
    CuisineTagType cuisine, DishBattleState::TeamSide team_side,
    int queue_index = -1,
    DishBattleState::Phase phase = DishBattleState::Phase::InQueue) {
  auto &dish = afterhours::EntityHelper::createEntity();
  dish.addComponent<IsDish>(DishType::Potato);
  dish.addComponent<DishLevel>(1);
  dish.addComponent<CuisineTag>(cuisine);

  auto &dbs = dish.addComponent<DishBattleState>();
  dbs.queue_index = (queue_index >= 0) ? queue_index : 0;
  dbs.team_side = team_side;
  dbs.phase = phase;

  return dish;
}

static afterhours::Entity &create_test_dish_without_cuisine(
    DishBattleState::TeamSide team_side, int queue_index = -1,
    DishBattleState::Phase phase = DishBattleState::Phase::InQueue) {
  auto &dish = afterhours::EntityHelper::createEntity();
  dish.addComponent<IsDish>(DishType::Potato);
  dish.addComponent<DishLevel>(1);
  // No CuisineTag added

  auto &dbs = dish.addComponent<DishBattleState>();
  dbs.queue_index = (queue_index >= 0) ? queue_index : 0;
  dbs.team_side = team_side;
  dbs.phase = phase;

  return dish;
}

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

static bool validate_synergy_count(const BattleSynergyCounts &counts,
                                   CuisineTagType cuisine, int expected_count,
                                   DishBattleState::TeamSide team) {
  const std::map<CuisineTagType, int> &team_counts =
      (team == DishBattleState::TeamSide::Player)
          ? counts.player_cuisine_counts
          : counts.opponent_cuisine_counts;

  auto it = team_counts.find(cuisine);
  if (it == team_counts.end()) {
    if (expected_count == 0) {
      return true;
    }
    log_error("SET_BONUS_TEST: Cuisine {} not found in counts (expected {})",
              magic_enum::enum_name(cuisine), expected_count);
    return false;
  }

  if (it->second != expected_count) {
    log_error("SET_BONUS_TEST: Wrong count for {} - expected {}, got {}",
              magic_enum::enum_name(cuisine), expected_count, it->second);
    return false;
  }

  return true;
}

static bool validate_persistent_modifier(afterhours::Entity &dish,
                                         int expected_zing, int expected_body) {
  if (!dish.has<PersistentCombatModifiers>()) {
    if (expected_zing == 0 && expected_body == 0) {
      return true; // No modifier expected
    }
    log_error("SET_BONUS_TEST: Missing PersistentCombatModifiers on dish {}",
              dish.id);
    return false;
  }

  const PersistentCombatModifiers &mod = dish.get<PersistentCombatModifiers>();
  if (mod.zingDelta != expected_zing || mod.bodyDelta != expected_body) {
    log_error("SET_BONUS_TEST: Wrong modifier values on dish {} - expected "
              "zing={} body={}, got zing={} body={}",
              dish.id, expected_zing, expected_body, mod.zingDelta,
              mod.bodyDelta);
    return false;
  }

  return true;
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

struct TestDishes {
  afterhours::Entity *american1 = nullptr;
  afterhours::Entity *american2 = nullptr;
  afterhours::Entity *non_american = nullptr;
};

struct TestDishes4 {
  afterhours::Entity *american1 = nullptr;
  afterhours::Entity *american2 = nullptr;
  afterhours::Entity *american3 = nullptr;
  afterhours::Entity *american4 = nullptr;
};

struct TestDishes6 {
  afterhours::Entity *american1 = nullptr;
  afterhours::Entity *american2 = nullptr;
  afterhours::Entity *american3 = nullptr;
  afterhours::Entity *american4 = nullptr;
  afterhours::Entity *american5 = nullptr;
  afterhours::Entity *american6 = nullptr;
};

struct TestDishesNoSynergy {
  afterhours::Entity *dish1 = nullptr;
  afterhours::Entity *dish2 = nullptr;
  afterhours::Entity *thai_dish = nullptr;
};

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

static TestDishes setup_american_2_piece(TestApp &app) {
  log_info("SET_BONUS_TEST: Setting up American 2-piece bonus test");

  reset_test_state();

  navigate_to_battle_screen(app);

  // Clear existing battle dishes so we only test with our dishes
  clear_existing_battle_dishes();

  // Create 2 American dishes
  auto &american1 = create_test_dish_with_cuisine(
      CuisineTagType::American, DishBattleState::TeamSide::Player, 0,
      DishBattleState::Phase::InQueue);
  auto &american2 = create_test_dish_with_cuisine(
      CuisineTagType::American, DishBattleState::TeamSide::Player, 1,
      DishBattleState::Phase::InQueue);

  // Create a non-American dish (should not get bonus)
  auto &non_american = create_test_dish_without_cuisine(
      DishBattleState::TeamSide::Player, 2, DishBattleState::Phase::InQueue);

  afterhours::EntityHelper::merge_entity_arrays();

  // Trigger synergy systems to count and apply bonuses for our test dishes
  trigger_synergy_systems();

  return {&american1, &american2, &non_american};
}

static bool validate_american_2_piece(const TestDishes &dishes) {
  // Validate synergy count
  afterhours::RefEntity counts_entity = get_or_create_battle_synergy_counts();
  const BattleSynergyCounts &counts =
      counts_entity.get().get<BattleSynergyCounts>();
  bool count_valid = validate_synergy_count(counts, CuisineTagType::American, 2,
                                            DishBattleState::TeamSide::Player);
  if (!count_valid) {
    log_error("SET_BONUS_TEST: Synergy count validation FAILED");
    return false;
  }

  // Validate bonuses applied
  // American 2-piece bonus is "+1 Body to all your dishes"
  // (TargetScope::AllAllies) So ALL player dishes get the bonus, not just
  // American ones
  bool american1_valid =
      validate_persistent_modifier(*dishes.american1, 0, 1); // +1 Body
  bool american2_valid =
      validate_persistent_modifier(*dishes.american2, 0, 1); // +1 Body
  bool non_american_valid = validate_persistent_modifier(
      *dishes.non_american, 0, 1); // +1 Body (bonus applies to all allies)

  return american1_valid && american2_valid && non_american_valid;
}

static TestDishes4 setup_american_4_piece(TestApp &app) {
  log_info("SET_BONUS_TEST: Setting up American 4-piece bonus test");

  reset_test_state();

  navigate_to_battle_screen(app);

  // Clear existing battle dishes so we only test with our dishes
  clear_existing_battle_dishes();

  // Create 4 American dishes
  auto &american1 = create_test_dish_with_cuisine(
      CuisineTagType::American, DishBattleState::TeamSide::Player, 0,
      DishBattleState::Phase::InQueue);
  auto &american2 = create_test_dish_with_cuisine(
      CuisineTagType::American, DishBattleState::TeamSide::Player, 1,
      DishBattleState::Phase::InQueue);
  auto &american3 = create_test_dish_with_cuisine(
      CuisineTagType::American, DishBattleState::TeamSide::Player, 2,
      DishBattleState::Phase::InQueue);
  auto &american4 = create_test_dish_with_cuisine(
      CuisineTagType::American, DishBattleState::TeamSide::Player, 3,
      DishBattleState::Phase::InQueue);

  afterhours::EntityHelper::merge_entity_arrays();

  // Trigger synergy systems to count and apply bonuses for our test dishes
  trigger_synergy_systems();

  return {&american1, &american2, &american3, &american4};
}

static bool validate_american_4_piece(const TestDishes4 &dishes) {
  // Validate synergy count
  afterhours::RefEntity counts_entity = get_or_create_battle_synergy_counts();
  const BattleSynergyCounts &counts =
      counts_entity.get().get<BattleSynergyCounts>();
  bool count_valid = validate_synergy_count(counts, CuisineTagType::American, 4,
                                            DishBattleState::TeamSide::Player);
  if (!count_valid) {
    log_error("SET_BONUS_TEST: Synergy count validation FAILED");
    return false;
  }

  // Validate bonuses applied (2-piece + 4-piece = +1 +2 = +3 Body total)
  bool american1_valid =
      validate_persistent_modifier(*dishes.american1, 0, 3); // +3 Body (2+4)
  bool american2_valid =
      validate_persistent_modifier(*dishes.american2, 0, 3); // +3 Body (2+4)
  bool american3_valid =
      validate_persistent_modifier(*dishes.american3, 0, 3); // +3 Body (2+4)
  bool american4_valid =
      validate_persistent_modifier(*dishes.american4, 0, 3); // +3 Body (2+4)

  return american1_valid && american2_valid && american3_valid &&
         american4_valid;
}

static TestDishes6 setup_american_6_piece(TestApp &app) {
  log_info("SET_BONUS_TEST: Setting up American 6-piece bonus test");

  reset_test_state();

  navigate_to_battle_screen(app);

  // Clear existing battle dishes so we only test with our dishes
  clear_existing_battle_dishes();

  // Create 6 American dishes
  auto &american1 = create_test_dish_with_cuisine(
      CuisineTagType::American, DishBattleState::TeamSide::Player, 0,
      DishBattleState::Phase::InQueue);
  auto &american2 = create_test_dish_with_cuisine(
      CuisineTagType::American, DishBattleState::TeamSide::Player, 1,
      DishBattleState::Phase::InQueue);
  auto &american3 = create_test_dish_with_cuisine(
      CuisineTagType::American, DishBattleState::TeamSide::Player, 2,
      DishBattleState::Phase::InQueue);
  auto &american4 = create_test_dish_with_cuisine(
      CuisineTagType::American, DishBattleState::TeamSide::Player, 3,
      DishBattleState::Phase::InQueue);
  auto &american5 = create_test_dish_with_cuisine(
      CuisineTagType::American, DishBattleState::TeamSide::Player, 4,
      DishBattleState::Phase::InQueue);
  auto &american6 = create_test_dish_with_cuisine(
      CuisineTagType::American, DishBattleState::TeamSide::Player, 5,
      DishBattleState::Phase::InQueue);

  afterhours::EntityHelper::merge_entity_arrays();

  // Trigger synergy systems to count and apply bonuses for our test dishes
  trigger_synergy_systems();

  return {&american1, &american2, &american3,
          &american4, &american5, &american6};
}

static bool validate_american_6_piece(const TestDishes6 &dishes) {
  // Validate synergy count
  afterhours::RefEntity counts_entity = get_or_create_battle_synergy_counts();
  const BattleSynergyCounts &counts =
      counts_entity.get().get<BattleSynergyCounts>();
  bool count_valid = validate_synergy_count(counts, CuisineTagType::American, 6,
                                            DishBattleState::TeamSide::Player);
  if (!count_valid) {
    log_error("SET_BONUS_TEST: Synergy count validation FAILED");
    return false;
  }

  // Validate bonuses applied (2-piece + 4-piece + 6-piece = +1 +2 +3 = +6 Body
  // total)
  bool american1_valid =
      validate_persistent_modifier(*dishes.american1, 0, 6); // +6 Body (2+4+6)
  bool american2_valid =
      validate_persistent_modifier(*dishes.american2, 0, 6); // +6 Body (2+4+6)
  bool american3_valid =
      validate_persistent_modifier(*dishes.american3, 0, 6); // +6 Body (2+4+6)
  bool american4_valid =
      validate_persistent_modifier(*dishes.american4, 0, 6); // +6 Body (2+4+6)
  bool american5_valid =
      validate_persistent_modifier(*dishes.american5, 0, 6); // +6 Body (2+4+6)
  bool american6_valid =
      validate_persistent_modifier(*dishes.american6, 0, 6); // +6 Body (2+4+6)

  return american1_valid && american2_valid && american3_valid &&
         american4_valid && american5_valid && american6_valid;
}

static TestDishesNoSynergy setup_no_synergy(TestApp &app) {
  log_info("SET_BONUS_TEST: Setting up no synergy test");

  reset_test_state();

  navigate_to_battle_screen(app);

  // Clear existing battle dishes so we only test with our dishes
  clear_existing_battle_dishes();

  // Create dishes without matching cuisine tags
  auto &dish1 = create_test_dish_without_cuisine(
      DishBattleState::TeamSide::Player, 0, DishBattleState::Phase::InQueue);
  auto &dish2 = create_test_dish_without_cuisine(
      DishBattleState::TeamSide::Player, 1, DishBattleState::Phase::InQueue);

  // Create dish with different cuisine (Thai, not American)
  auto &thai_dish = create_test_dish_with_cuisine(
      CuisineTagType::Thai, DishBattleState::TeamSide::Player, 2,
      DishBattleState::Phase::InQueue);

  afterhours::EntityHelper::merge_entity_arrays();

  // Trigger synergy systems to count (should find 0 American)
  trigger_synergy_systems();

  return {&dish1, &dish2, &thai_dish};
}

static bool validate_no_synergy(const TestDishesNoSynergy &dishes) {
  // Validate no American synergy counted
  afterhours::RefEntity counts_entity = get_or_create_battle_synergy_counts();
  const BattleSynergyCounts &counts =
      counts_entity.get().get<BattleSynergyCounts>();
  bool no_american_count = validate_synergy_count(
      counts, CuisineTagType::American, 0, DishBattleState::TeamSide::Player);
  if (!no_american_count) {
    log_error("SET_BONUS_TEST: Should have 0 American count");
    return false;
  }

  // Validate no bonuses applied
  bool dish1_valid = validate_persistent_modifier(*dishes.dish1, 0, 0);
  bool dish2_valid = validate_persistent_modifier(*dishes.dish2, 0, 0);
  bool thai_dish_valid = validate_persistent_modifier(*dishes.thai_dish, 0, 0);

  return dish1_valid && dish2_valid && thai_dish_valid;
}

} // namespace ValidateSetBonusSystemTestHelpers

TEST(validate_set_bonus_american_2_piece) {
  using namespace ValidateSetBonusSystemTestHelpers;

  app.launch_game();
  auto dishes = setup_american_2_piece(app);
  app.wait_for_frames(5); // Let systems run
  app.expect_true(validate_american_2_piece(dishes),
                  "American 2-piece bonus test");
}

TEST(validate_set_bonus_american_4_piece) {
  using namespace ValidateSetBonusSystemTestHelpers;

  app.launch_game();
  auto dishes = setup_american_4_piece(app);
  app.wait_for_frames(5); // Let systems run
  app.expect_true(validate_american_4_piece(dishes),
                  "American 4-piece bonus test");
}

TEST(validate_set_bonus_american_6_piece) {
  using namespace ValidateSetBonusSystemTestHelpers;

  app.launch_game();
  auto dishes = setup_american_6_piece(app);
  app.wait_for_frames(5); // Let systems run
  app.expect_true(validate_american_6_piece(dishes),
                  "American 6-piece bonus test");
}

TEST(validate_set_bonus_no_synergy) {
  using namespace ValidateSetBonusSystemTestHelpers;

  app.launch_game();
  auto dishes = setup_no_synergy(app);
  app.wait_for_frames(5); // Let systems run
  app.expect_true(validate_no_synergy(dishes), "No synergy no bonus test");
}
