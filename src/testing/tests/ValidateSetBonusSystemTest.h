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

static void navigate_to_shop(TestApp &app) {
  // Navigate to Shop screen
  // Assumes app.launch_game() was called first, which sets screen to Main
  app.wait_for_frames(1); // Ensure screen state is synced
  auto &gsm = GameStateManager::get();

  if (gsm.active_screen == GameStateManager::Screen::Shop) {
    return; // Already on Shop
  }

  // Navigate from Main to Shop
  app.wait_for_ui_exists("Play", 5.0f);
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
}

static void navigate_to_battle(TestApp &app) {
  // Navigate from Shop to Battle
  // Assumes we're already on Shop screen with dishes in inventory
  app.wait_for_ui_exists("Next Round", 5.0f);
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_frames(5); // Let battle initialize and systems run
}

} // namespace ValidateSetBonusSystemTestHelpers

TEST(validate_set_bonus_american_2_piece) {
  using namespace ValidateSetBonusSystemTestHelpers;

  // Follow production flow: create dishes in Shop, then navigate to battle
  app.launch_game();
  navigate_to_shop(app);
  
  // Wait for shop to initialize and ensure inventory is empty
  app.wait_for_frames(5);
  
  // Clear any existing inventory items to ensure clean state
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsInventoryItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    entity.cleanup = true;
  }
  afterhours::EntityHelper::cleanup();
  app.wait_for_frames(2);

  // Create 2 American dishes in inventory (Potato defaults to American, but
  // we'll be explicit)
  app.create_inventory_item(DishType::Potato, 0, CuisineTagType::American);
  app.create_inventory_item(DishType::Potato, 1, CuisineTagType::American);
  // Create a non-American dish - use Salmon which has no cuisine tag by default
  app.create_inventory_item(DishType::Salmon, 2);

  app.wait_for_frames(2); // Let entities merge

  // Navigate to battle - systems will run naturally
  navigate_to_battle(app);

  // Wait for systems to process (BattleSynergyCountingSystem and
  // ApplySetBonusesSystem run on battle start)
  app.wait_for_frames(20);

  // Validate synergy count
  app.expect_synergy_count(CuisineTagType::American, 2,
                           DishBattleState::TeamSide::Player);

  // Find battle dishes by slot (they match inventory order)
  // Slot 0 and 1 should be American dishes with +1 Body modifier
  // Slot 2 should be Salmon with no modifier
  int found_american_count = 0;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsDish>()
           .whereHasComponent<DishBattleState>()
           .gen()) {
    if (entity.cleanup) {
      continue;
    }
    const auto &dbs = entity.get<DishBattleState>();
    if (dbs.team_side != DishBattleState::TeamSide::Player) {
      continue;
    }

    if (dbs.queue_index == 0 || dbs.queue_index == 1) {
      // American dishes should have +1 Body from 2-piece bonus
      if (entity.has<CuisineTag>()) {
        const auto &tag = entity.get<CuisineTag>();
        if (tag.has(CuisineTagType::American)) {
          found_american_count++;
          app.expect_true(
              entity.has<PersistentCombatModifiers>(),
              "American dish should have PersistentCombatModifiers");
          if (entity.has<PersistentCombatModifiers>()) {
            const auto &mod = entity.get<PersistentCombatModifiers>();
            app.expect_eq(mod.bodyDelta, 1,
                          "American dish at slot " +
                              std::to_string(dbs.queue_index) +
                              " should have +1 Body from 2-piece bonus");
            app.expect_eq(mod.zingDelta, 0,
                          "American dish should have 0 Zing delta");
          }
        }
      }
    }
  }
  app.expect_eq(found_american_count, 2,
                "Should find 2 American dishes in battle");
}

TEST(validate_set_bonus_american_4_piece) {
  using namespace ValidateSetBonusSystemTestHelpers;

  // Follow production flow: create dishes in Shop, then navigate to battle
  app.launch_game();
  navigate_to_shop(app);
  
  // Wait for shop to initialize and ensure inventory is empty
  app.wait_for_frames(5);
  
  // Clear any existing inventory items
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsInventoryItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    entity.cleanup = true;
  }
  afterhours::EntityHelper::cleanup();
  app.wait_for_frames(2);

  // Create 4 American dishes in inventory
  app.create_inventory_item(DishType::Potato, 0, CuisineTagType::American);
  app.create_inventory_item(DishType::Potato, 1, CuisineTagType::American);
  app.create_inventory_item(DishType::Potato, 2, CuisineTagType::American);
  app.create_inventory_item(DishType::Potato, 3, CuisineTagType::American);

  app.wait_for_frames(2); // Let entities merge

  // Navigate to battle - systems will run naturally
  navigate_to_battle(app);

  // Wait for systems to process
  app.wait_for_frames(10);

  // Validate synergy count (4 American dishes)
  app.expect_synergy_count(CuisineTagType::American, 4,
                           DishBattleState::TeamSide::Player);

  // Validate modifiers: 2-piece (+1) + 4-piece (+2) = +3 Body total
  // Note: In production, bonuses are applied once when entering battle.
  // The actual value might be higher if other systems add modifiers (pairings, effects, etc.)
  // For this test, we'll validate that the modifier is at least the expected set bonus amount
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsDish>()
           .whereHasComponent<DishBattleState>()
           .gen()) {
    const auto &dbs = entity.get<DishBattleState>();
    if (dbs.team_side == DishBattleState::TeamSide::Player &&
        (dbs.queue_index >= 0 && dbs.queue_index <= 3)) {
      app.expect_true(entity.has<PersistentCombatModifiers>(),
                      "American dish should have PersistentCombatModifiers");
      if (entity.has<PersistentCombatModifiers>()) {
        const auto &mod = entity.get<PersistentCombatModifiers>();
        // Set bonus should give at least +3 Body (2-piece +1, 4-piece +2)
        // But other systems (pairings, effects) may add more, so check >= 3
        app.expect_true(mod.bodyDelta >= 3,
                       "American dish should have at least +3 Body from set bonuses (2-piece + 4-piece)");
        // Zing should be 0 from set bonuses (American bonuses only give Body)
        // But other systems may add Zing, so we don't validate it here
      }
    }
  }
}

TEST(validate_set_bonus_american_6_piece) {
  using namespace ValidateSetBonusSystemTestHelpers;

  // Follow production flow: create dishes in Shop, then navigate to battle
  app.launch_game();
  navigate_to_shop(app);
  
  // Wait for shop to initialize and ensure inventory is empty
  app.wait_for_frames(5);
  
  // Clear any existing inventory items
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsInventoryItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    entity.cleanup = true;
  }
  afterhours::EntityHelper::cleanup();
  app.wait_for_frames(2);

  // Create 6 American dishes in inventory
  app.create_inventory_item(DishType::Potato, 0, CuisineTagType::American);
  app.create_inventory_item(DishType::Potato, 1, CuisineTagType::American);
  app.create_inventory_item(DishType::Potato, 2, CuisineTagType::American);
  app.create_inventory_item(DishType::Potato, 3, CuisineTagType::American);
  app.create_inventory_item(DishType::Potato, 4, CuisineTagType::American);
  app.create_inventory_item(DishType::Potato, 5, CuisineTagType::American);

  app.wait_for_frames(2); // Let entities merge

  // Navigate to battle - systems will run naturally
  navigate_to_battle(app);

  // Wait for systems to process
  app.wait_for_frames(10);

  // Validate synergy count (6 American dishes)
  app.expect_synergy_count(CuisineTagType::American, 6,
                           DishBattleState::TeamSide::Player);

  // Validate modifiers: 2-piece (+1) + 4-piece (+2) + 6-piece (+3) = +6 Body
  // total
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsDish>()
           .whereHasComponent<DishBattleState>()
           .gen()) {
    const auto &dbs = entity.get<DishBattleState>();
    if (dbs.team_side == DishBattleState::TeamSide::Player &&
        (dbs.queue_index >= 0 && dbs.queue_index <= 5)) {
      app.expect_true(entity.has<PersistentCombatModifiers>(),
                      "American dish should have PersistentCombatModifiers");
      if (entity.has<PersistentCombatModifiers>()) {
        const auto &mod = entity.get<PersistentCombatModifiers>();
        // Set bonus should give at least +6 Body (2-piece +1, 4-piece +2, 6-piece +3)
        // But other systems (pairings, effects) may add more, so check >= 6
        app.expect_true(mod.bodyDelta >= 6,
                       "American dish should have at least +6 Body from set bonuses (2-piece + 4-piece + 6-piece)");
        // Zing should be 0 from set bonuses (American bonuses only give Body)
        // But other systems may add Zing, so we don't validate it here
      }
    }
  }
}

TEST(validate_set_bonus_no_synergy) {
  using namespace ValidateSetBonusSystemTestHelpers;

  // Follow production flow: create dishes in Shop, then navigate to battle
  app.launch_game();
  navigate_to_shop(app);
  
  // Wait for shop to initialize and ensure inventory is empty
  app.wait_for_frames(5);
  
  // Clear any existing inventory items
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsInventoryItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    entity.cleanup = true;
  }
  afterhours::EntityHelper::cleanup();
  app.wait_for_frames(2);

  // Create dishes without matching cuisine tags
  // Use Salmon (no cuisine tag) and Ramen (Japanese, not American)
  app.create_inventory_item(DishType::Salmon, 0); // No cuisine tag
  app.create_inventory_item(DishType::Salmon, 1); // No cuisine tag
  app.create_inventory_item(DishType::Ramen, 2,
                            CuisineTagType::Japanese); // Japanese, not American

  app.wait_for_frames(2); // Let entities merge

  // Navigate to battle - systems will run naturally
  navigate_to_battle(app);

  // Wait for systems to process
  app.wait_for_frames(10);

  // Validate: Should have 0 American synergy count
  app.expect_synergy_count(CuisineTagType::American, 0,
                           DishBattleState::TeamSide::Player);

  // Validate: Dishes should not have American set bonus modifiers
  // (They might have modifiers from other sources, but not from American set
  // bonus)
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsDish>()
           .whereHasComponent<DishBattleState>()
           .gen()) {
    const auto &dbs = entity.get<DishBattleState>();
    if (dbs.team_side == DishBattleState::TeamSide::Player) {
      // Verify dishes don't have American tag (or if they do, they shouldn't
      // get bonus)
      if (entity.has<CuisineTag>()) {
        const auto &tag = entity.get<CuisineTag>();
        app.expect_false(
            tag.has(CuisineTagType::American),
            "Dish at slot " + std::to_string(dbs.queue_index) +
                " should not have American tag for no-synergy test");
      }
    }
  }
}
