#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/combat_stats.h"
#include "../../components/deferred_flavor_mods.h"
#include "../../components/dish_battle_state.h"
#include "../../components/drink_effects.h"
#include "../../components/drink_pairing.h"
#include "../../components/is_drink_shop_item.h"
#include "../../components/is_inventory_item.h"
#include "../../components/pending_combat_mods.h"
#include "../../components/trigger_event.h"
#include "../../components/trigger_queue.h"
#include "../../dish_types.h"
#include "../../drink_types.h"
#include "../../game_state_manager.h"
#include "../../query.h"
#include "../test_app.h"
#include "../test_macros.h"

namespace ValidateDrinkEffectsTestHelpers {

// Helper to set up battle state with a dish that has a drink applied
// This simulates: shop → apply drink → battle
static afterhours::EntityID setup_battle_with_drink(TestApp &app,
                                                    DishType dish_type,
                                                    int slot,
                                                    DrinkType drink_type) {
  // Navigate to shop first
  app.launch_game();

  // Use test override API to ensure the desired drink is available
  // Set the override after launch_game but before navigating to shop
  app.set_drink_shop_override({drink_type, drink_type, drink_type, drink_type});

  app.navigate_to_shop();
  app.wait_for_frames(10); // Wait for shop systems to initialize

  // Wait for drinks to be generated
  app.wait_for_frames(10);

  // Create dish in inventory
  app.create_inventory_item(dish_type, slot);
  app.wait_for_frames(5);

  // Apply drink via drag-and-drop simulation
  app.apply_drink_to_dish(slot, drink_type);
  app.wait_for_frames(5);

  // Clear the override after use
  app.clear_drink_shop_override();

  // Get the dish entity ID from inventory
  afterhours::EntityID dish_id = -1;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsInventoryItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    if (entity.get<IsInventoryItem>().slot == slot) {
      dish_id = entity.id;
      break;
    }
  }

  if (dish_id == -1) {
    app.fail("Dish not found in inventory slot " + std::to_string(slot));
    return -1;
  }

  // Transition to battle state
  // For ECS tests, we'll manually set up battle state
  // TODO: Migrate to full game flow (shop → click "Next Round" → battle)
  app.setup_battle();
  app.wait_for_frames(5);

  // Move dish from inventory to battle state
  afterhours::OptEntity dish_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(dish_id)
          .gen_first();
  if (dish_opt.has_value()) {
    afterhours::Entity &dish = dish_opt.asE();
    // Remove inventory item component, add battle state
    if (dish.has<IsInventoryItem>()) {
      dish.removeComponent<IsInventoryItem>();
    }
    auto &dbs = dish.addComponent<DishBattleState>();
    dbs.team_side = DishBattleState::TeamSide::Player;
    dbs.queue_index = slot;
    dbs.phase = DishBattleState::Phase::InQueue;
    if (!dish.has<CombatStats>()) {
      dish.addComponent<CombatStats>();
    }
  }

  app.wait_for_frames(5);

  return dish_id;
}

static void test_water_effect(TestApp &app) {
  log_info("DRINK_TEST: Testing Water (no effect baseline)");

  // Set up battle with dish that has Water applied
  auto dish_id =
      setup_battle_with_drink(app, DishType::Potato, 0, DrinkType::Water);
  app.expect_true(dish_id != -1, "dish was created");

  // Wait for battle systems to run
  app.wait_for_frames(30);

  // Read state to verify no effects (read-only)
  afterhours::OptEntity dish_after_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(dish_id)
          .gen_first();
  app.expect_true(dish_after_opt.has_value(), "dish entity still exists");
  auto &dish_after = dish_after_opt.asE();
  app.expect_false(dish_after.has<DeferredFlavorMods>(),
                   "Water has no flavor effect");
  app.expect_false(dish_after.has<PendingCombatMods>(),
                   "Water has no combat effect");

  log_info("DRINK_TEST: Water effect PASSED");
}

static void test_orange_juice_effect(TestApp &app) {
  log_info("DRINK_TEST: Testing Orange Juice (OnServe +1 Freshness to Self)");

  // Set up battle with dish that has Orange Juice applied
  auto dish_id =
      setup_battle_with_drink(app, DishType::Potato, 0, DrinkType::OrangeJuice);
  app.expect_true(dish_id != -1, "dish was created");

  // Verify DrinkPairing and DrinkEffects were applied
  afterhours::OptEntity dish_check_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(dish_id)
          .gen_first();
  app.expect_true(dish_check_opt.has_value(), "dish entity still exists");
  auto &dish_check = dish_check_opt.asE();
  app.expect_true(dish_check.has<DrinkPairing>(),
                  "DrinkPairing component should exist");
  app.expect_true(
      dish_check.has<DrinkEffects>(),
      "DrinkEffects component should be added by ApplyDrinkPairingEffects");

  // Wait for battle systems to run and process OnServe trigger naturally
  // The battle should progress and trigger OnServe automatically
  app.wait_for_frames(60);

  // Read state to verify effect was applied (read-only)
  DeferredFlavorMods expected;
  expected.freshness = 1;
  app.expect_flavor_mods(dish_id, expected);

  log_info("DRINK_TEST: Orange Juice effect PASSED");
}

static void test_coffee_effect(TestApp &app) {
  log_info("DRINK_TEST: Testing Coffee (OnStartBattle +2 Zing to Self)");

  // Set up battle with dish that has Coffee applied
  auto dish_id =
      setup_battle_with_drink(app, DishType::Potato, 0, DrinkType::Coffee);
  app.expect_true(dish_id != -1, "dish was created");

  // Verify DrinkPairing and DrinkEffects were applied
  afterhours::OptEntity dish_check_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(dish_id)
          .gen_first();
  app.expect_true(dish_check_opt.has_value(), "dish entity still exists");
  auto &dish_check = dish_check_opt.asE();
  app.expect_true(dish_check.has<DrinkPairing>(),
                  "DrinkPairing component should exist");
  app.expect_true(
      dish_check.has<DrinkEffects>(),
      "DrinkEffects component should be added by ApplyDrinkPairingEffects");

  // Wait for battle systems to run and process OnStartBattle trigger naturally
  app.wait_for_frames(60);

  // Read state to verify effect was applied (read-only)
  PendingCombatMods expected;
  expected.zingDelta = 2;
  expected.bodyDelta = 0;
  app.expect_combat_mods(dish_id, expected);

  log_info("DRINK_TEST: Coffee effect PASSED");
}

static void test_red_soda_effect(TestApp &app) {
  log_info("DRINK_TEST: Testing Red Soda (OnCourseComplete +1 Zing to Self)");

  // Set up battle with dish that has Red Soda applied
  auto dish_id =
      setup_battle_with_drink(app, DishType::Potato, 0, DrinkType::RedSoda);
  app.expect_true(dish_id != -1, "dish was created");

  // Wait for battle systems to run and process OnCourseComplete trigger
  // naturally Note: OnCourseComplete triggers when a course completes
  app.wait_for_frames(60);

  // Read state to verify effect was applied (read-only)
  PendingCombatMods expected;
  expected.zingDelta = 1;
  expected.bodyDelta = 0;
  app.expect_combat_mods(dish_id, expected);

  log_info("DRINK_TEST: Red Soda effect PASSED");
}

static void test_blue_soda_effect(TestApp &app) {
  log_info("DRINK_TEST: Testing Blue Soda (OnCourseComplete +1 Body to Self)");

  // Set up battle with dish that has Blue Soda applied
  auto dish_id =
      setup_battle_with_drink(app, DishType::Potato, 0, DrinkType::BlueSoda);
  app.expect_true(dish_id != -1, "dish was created");

  // Wait for battle systems to run and process OnCourseComplete trigger
  // naturally
  app.wait_for_frames(60);

  // Read state to verify effect was applied (read-only)
  PendingCombatMods expected;
  expected.zingDelta = 0;
  expected.bodyDelta = 1;
  app.expect_combat_mods(dish_id, expected);

  log_info("DRINK_TEST: Blue Soda effect PASSED");
}

static void test_watermelon_juice_effect(TestApp &app) {
  log_info("DRINK_TEST: Testing Watermelon Juice (OnCourseComplete +1 "
           "Freshness and +1 Body to Self)");

  // Set up battle with dish that has Watermelon Juice applied
  auto dish_id = setup_battle_with_drink(app, DishType::Potato, 0,
                                         DrinkType::WatermelonJuice);
  app.expect_true(dish_id != -1, "dish was created");

  // Wait for battle systems to run and process OnCourseComplete trigger
  // naturally
  app.wait_for_frames(60);

  // Read state to verify effects were applied (read-only)
  DeferredFlavorMods expected_flavor;
  expected_flavor.freshness = 1;
  app.expect_flavor_mods(dish_id, expected_flavor);

  PendingCombatMods expected_combat;
  expected_combat.zingDelta = 0;
  expected_combat.bodyDelta = 1;
  app.expect_combat_mods(dish_id, expected_combat);

  log_info("DRINK_TEST: Watermelon Juice effect PASSED");
}

static void test_yellow_soda_effect(TestApp &app) {
  log_info("DRINK_TEST: Testing Yellow Soda (OnBiteTaken +1 Zing to Self)");

  // Set up battle with dish that has Yellow Soda applied
  auto dish_id =
      setup_battle_with_drink(app, DishType::Potato, 0, DrinkType::YellowSoda);
  app.expect_true(dish_id != -1, "dish was created");

  // Wait for battle systems to run and process OnBiteTaken trigger naturally
  app.wait_for_frames(60);

  // Read state to verify effect was applied (read-only)
  PendingCombatMods expected;
  expected.zingDelta = 1;
  expected.bodyDelta = 0;
  app.expect_combat_mods(dish_id, expected);

  log_info("DRINK_TEST: Yellow Soda effect PASSED");
}

static void test_green_soda_effect(TestApp &app) {
  log_info("DRINK_TEST: Testing Green Soda (OnServe +2 Zing, -1 Body to Self)");

  // Set up battle with dish that has Green Soda applied
  auto dish_id =
      setup_battle_with_drink(app, DishType::Potato, 0, DrinkType::GreenSoda);
  app.expect_true(dish_id != -1, "dish was created");

  // Wait for battle systems to run and process OnServe trigger naturally
  app.wait_for_frames(60);

  // Read state to verify effect was applied (read-only)
  PendingCombatMods expected;
  expected.zingDelta = 2;
  expected.bodyDelta = -1;
  app.expect_combat_mods(dish_id, expected);

  log_info("DRINK_TEST: Green Soda effect PASSED");
}

static void test_white_wine_effect(TestApp &app) {
  log_info(
      "DRINK_TEST: Testing White Wine (OnStartBattle +1 Zing to AllAllies)");

  // Set up shop state
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_frames(10);

  // Create two dishes in inventory
  app.create_inventory_item(DishType::Potato, 0);
  app.create_inventory_item(DishType::Potato, 1);
  app.wait_for_frames(5);

  // Apply drink to source dish (slot 0)
  app.apply_drink_to_dish(0, DrinkType::WhiteWine);
  app.wait_for_frames(5);

  // Get dish IDs
  afterhours::EntityID source_dish_id = -1;
  afterhours::EntityID ally_dish_id = -1;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsInventoryItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    int slot = entity.get<IsInventoryItem>().slot;
    if (slot == 0) {
      source_dish_id = entity.id;
    } else if (slot == 1) {
      ally_dish_id = entity.id;
    }
  }

  app.expect_true(source_dish_id != -1, "source dish was created");
  app.expect_true(ally_dish_id != -1, "ally dish was created");

  // Transition to battle state
  app.setup_battle();
  app.wait_for_frames(5);

  // Move dishes from inventory to battle state
  for (afterhours::EntityID dish_id : {source_dish_id, ally_dish_id}) {
    afterhours::OptEntity dish_opt =
        afterhours::EntityQuery({.force_merge = true})
            .whereID(dish_id)
            .gen_first();
    if (dish_opt.has_value()) {
      afterhours::Entity &dish = dish_opt.asE();
      if (dish.has<IsInventoryItem>()) {
        int slot = dish.get<IsInventoryItem>().slot;
        dish.removeComponent<IsInventoryItem>();
        auto &dbs = dish.addComponent<DishBattleState>();
        dbs.team_side = DishBattleState::TeamSide::Player;
        dbs.queue_index = slot;
        dbs.phase = DishBattleState::Phase::InQueue;
        if (!dish.has<CombatStats>()) {
          dish.addComponent<CombatStats>();
        }
      }
    }
  }

  app.wait_for_frames(5);

  // Wait for battle systems to run and process OnStartBattle trigger naturally
  app.wait_for_frames(60);

  // Read state to verify effect was applied to ally (read-only)
  PendingCombatMods expected;
  expected.zingDelta = 1;
  expected.bodyDelta = 0;
  app.expect_combat_mods(ally_dish_id, expected);

  log_info("DRINK_TEST: White Wine effect PASSED");
}

static void test_red_wine_effect(TestApp &app) {
  log_info(
      "DRINK_TEST: Testing Red Wine (OnServe +1 Richness to Self and Next)");

  // Set up shop state
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_frames(10);

  // Create two dishes in inventory
  app.create_inventory_item(DishType::Potato, 0);
  app.create_inventory_item(DishType::Potato, 1);
  app.wait_for_frames(5);

  // Apply drink to source dish (slot 0)
  app.apply_drink_to_dish(0, DrinkType::RedWine);
  app.wait_for_frames(5);

  // Get dish IDs
  afterhours::EntityID source_dish_id = -1;
  afterhours::EntityID next_dish_id = -1;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsInventoryItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    int slot = entity.get<IsInventoryItem>().slot;
    if (slot == 0) {
      source_dish_id = entity.id;
    } else if (slot == 1) {
      next_dish_id = entity.id;
    }
  }

  app.expect_true(source_dish_id != -1, "source dish was created");
  app.expect_true(next_dish_id != -1, "next dish was created");

  // Transition to battle state
  app.setup_battle();
  app.wait_for_frames(5);

  // Move dishes from inventory to battle state
  for (afterhours::EntityID dish_id : {source_dish_id, next_dish_id}) {
    afterhours::OptEntity dish_opt =
        afterhours::EntityQuery({.force_merge = true})
            .whereID(dish_id)
            .gen_first();
    if (dish_opt.has_value()) {
      afterhours::Entity &dish = dish_opt.asE();
      if (dish.has<IsInventoryItem>()) {
        int slot = dish.get<IsInventoryItem>().slot;
        dish.removeComponent<IsInventoryItem>();
        auto &dbs = dish.addComponent<DishBattleState>();
        dbs.team_side = DishBattleState::TeamSide::Player;
        dbs.queue_index = slot;
        dbs.phase = DishBattleState::Phase::InQueue;
      }
    }
  }

  app.wait_for_frames(5);

  // Wait for battle systems to run and process OnServe trigger naturally
  app.wait_for_frames(60);

  // Read state to verify effects were applied (read-only)
  DeferredFlavorMods expected;
  expected.richness = 1;
  app.expect_flavor_mods(source_dish_id, expected);
  app.expect_flavor_mods(next_dish_id, expected);

  log_info("DRINK_TEST: Red Wine effect PASSED");
}

} // namespace ValidateDrinkEffectsTestHelpers

TEST(validate_drink_effects) {
  using namespace ValidateDrinkEffectsTestHelpers;

  log_info("DRINK_TEST: Starting drink effects validation");

  test_water_effect(app);
  test_orange_juice_effect(app);
  test_coffee_effect(app);
  test_red_soda_effect(app);
  test_blue_soda_effect(app);
  test_watermelon_juice_effect(app);
  test_yellow_soda_effect(app);
  test_green_soda_effect(app);
  test_white_wine_effect(app);
  test_red_wine_effect(app);

  log_info("DRINK_TEST: All tests completed");
}
