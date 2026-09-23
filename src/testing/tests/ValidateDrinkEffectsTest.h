#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/combat_stats.h"
#include "../../components/deferred_flavor_mods.h"
#include "../../components/dish_battle_state.h"
#include "../../components/drink_effects.h"
#include "../../components/drink_pairing.h"
#include "../../components/is_drink_shop_item.h"
#include "../../components/is_inventory_item.h"
#include "../../components/persistent_combat_modifiers.h"
#include "../../dish_types.h"
#include "../../drink_types.h"
#include "../../game_state_manager.h"
#include "../../query.h"
#include "../../systems/ApplyDrinkPairingEffects.h"
#include "../test_app.h"
#include "../test_macros.h"
#include <iterator>

namespace ValidateDrinkEffectsTestHelpers {

static int inventory_dish_id(int slot) {
  afterhours::OptEntity opt = afterhours::EntityQuery({.force_merge = true})
                                  .whereHasComponent<IsInventoryItem>()
                                  .whereHasComponent<IsDish>()
                                  .whereLambda([slot](const afterhours::Entity &e) {
                                    return e.get<IsInventoryItem>().slot == slot;
                                  })
                                  .gen_first();
  return opt.has_value() ? static_cast<int>(opt.asE().id) : -1;
}

static void ensure_battle_load_request_exists() {
  if (afterhours::EntityHelper::has_singleton<BattleLoadRequest>()) {
    return;
  }
  afterhours::Entity &request_entity = afterhours::EntityHelper::createEntity();
  BattleLoadRequest request;
  request.loaded = false;
  request_entity.addComponent<BattleLoadRequest>(std::move(request));
  afterhours::EntityHelper::registerSingleton<BattleLoadRequest>(
      request_entity);
}

static void move_dish_to_battle(afterhours::EntityID dish_id, int slot) {
  afterhours::OptEntity dish_opt =
      afterhours::EntityQuery({.force_merge = true}).whereID(dish_id).gen_first();
  if (!dish_opt.has_value()) {
    return;
  }
  afterhours::Entity &dish = dish_opt.asE();
  dish.removeComponentIfExists<IsInventoryItem>();
  DishBattleState &dbs = dish.addComponent<DishBattleState>();
  dbs.team_side = DishBattleState::TeamSide::Player;
  dbs.queue_index = slot;
  dbs.phase = DishBattleState::Phase::InQueue;
  if (!dish.has<CombatStats>()) {
    dish.addComponent<CombatStats>();
  }
  if (dish.has<DrinkPairing>()) {
    ApplyDrinkPairingEffects apply_effects;
    apply_effects.for_each_with(dish, dish.get<IsDish>(), dbs,
                                dish.get<DrinkPairing>(), 0.0f);
  }
}

static void start_manual_battle(TestApp &app, int opponent_slots) {
  app.once([&] { ensure_battle_load_request_exists(); });
  for (int slot = 0; slot < opponent_slots; ++slot) {
    app.create_dish(DishType::Potato)
        .on_team(DishBattleState::TeamSide::Opponent)
        .at_slot(slot)
        .in_phase(DishBattleState::Phase::InQueue)
        .with_combat_stats()
        .commit();
  }
  app.setup_battle();
  app.wait_for_frames(1);
}

static void expect_persistent_mods(TestApp &app, afterhours::EntityID dish_id,
                                   int zing, int body) {
  afterhours::Entity *entity = app.find_entity_by_id(dish_id);
  app.expect_true(entity != nullptr, "dish entity still exists");
  int actual_zing = 0;
  int actual_body = 0;
  if (entity->has<PersistentCombatModifiers>()) {
    actual_zing = entity->get<PersistentCombatModifiers>().zingDelta;
    actual_body = entity->get<PersistentCombatModifiers>().bodyDelta;
  }
  app.expect_eq(actual_zing, zing, "persistent zing modifier");
  app.expect_eq(actual_body, body, "persistent body modifier");
}

static void expect_flavor_and_combat(TestApp &app, afterhours::EntityID dish_id,
                                     const DeferredFlavorMods &flavor,
                                     int zing, int body) {
  afterhours::Entity *entity = app.find_entity_by_id(dish_id);
  app.expect_true(entity != nullptr, "dish entity still exists");
  if (entity->has<DeferredFlavorMods>()) {
    app.expect_flavor_mods(dish_id, flavor);
    if (zing != 0 || body != 0) {
      expect_persistent_mods(app, dish_id, zing, body);
    }
    return;
  }
  FlavorStats base = get_dish_info(entity->get<IsDish>().type).flavor;
  FlavorStats modified = base;
  modified.applyMod(flavor);
  expect_persistent_mods(app, dish_id, zing + modified.zing() - base.zing(),
                         body + modified.body() - base.body());
}

static void expect_drink_applied(TestApp &app, afterhours::EntityID dish_id) {
  afterhours::Entity *entity = app.find_entity_by_id(dish_id);
  app.expect_true(entity != nullptr, "dish entity still exists");
  app.expect_true(entity->has<DrinkPairing>(),
                  "DrinkPairing component should exist");
  app.expect_true(entity->has<DrinkEffects>(),
                  "DrinkEffects component should be added by "
                  "ApplyDrinkPairingEffects");
}

static void setup_shop_with_drink(TestApp &app, DrinkType drink_type) {
  app.launch_game();
  app.once([&] {
    for (afterhours::Entity &entity :
         afterhours::EntityQuery({.force_merge = true})
             .whereHasComponent<IsDrinkShopItem>()
             .gen()) {
      entity.cleanup = true;
    }
  });
  app.wait_for_frames(1);
  app.set_drink_shop_override({drink_type, drink_type, drink_type, drink_type});
  app.wait_for_ui_exists("Play", 5.0f);
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_frames(10);
}

static afterhours::EntityID setup_battle_with_drink(TestApp &app,
                                                    DishType dish_type,
                                                    int slot,
                                                    DrinkType drink_type) {
  setup_shop_with_drink(app, drink_type);

  app.create_inventory_item(dish_type, slot);
  app.wait_for_frames(5);

  app.apply_drink_to_dish(slot, drink_type);
  app.wait_for_frames(5);
  app.clear_drink_shop_override();

  const int dish_id = app.remember_int("dish_id", inventory_dish_id(slot));
  app.expect_true(dish_id != -1, "dish was created");

  app.once([&] { move_dish_to_battle(dish_id, slot); });
  start_manual_battle(app, 1);
  return dish_id;
}

static void setup_battle_with_two_dishes(TestApp &app, DrinkType drink_type) {
  setup_shop_with_drink(app, drink_type);

  app.create_inventory_item(DishType::Potato, 0);
  app.create_inventory_item(DishType::Potato, 1);
  app.wait_for_frames(5);

  app.apply_drink_to_dish(0, drink_type);
  app.wait_for_frames(5);
  app.clear_drink_shop_override();

  const int source_id = app.remember_int("source_id", inventory_dish_id(0));
  const int other_id = app.remember_int("other_id", inventory_dish_id(1));
  app.expect_true(source_id != -1, "source dish was created");
  app.expect_true(other_id != -1, "second dish was created");

  app.once([&] {
    move_dish_to_battle(source_id, 0);
    move_dish_to_battle(other_id, 1);
  });
  start_manual_battle(app, 2);
}

static void test_water_effect(TestApp &app) {
  afterhours::EntityID dish_id =
      setup_battle_with_drink(app, DishType::Potato, 0, DrinkType::Water);
  app.wait_for_frames(30);

  afterhours::Entity *dish = app.find_entity_by_id(dish_id);
  app.expect_true(dish != nullptr, "dish entity still exists");
  app.expect_false(dish->has<DeferredFlavorMods>(), "Water has no flavor effect");
  expect_persistent_mods(app, dish_id, 0, 0);
}

static void test_orange_juice_effect(TestApp &app) {
  afterhours::EntityID dish_id =
      setup_battle_with_drink(app, DishType::Potato, 0, DrinkType::OrangeJuice);
  expect_drink_applied(app, dish_id);
  app.wait_for_frames(60);

  DeferredFlavorMods expected;
  expected.freshness = 1;
  expect_flavor_and_combat(app, dish_id, expected, 0, 0);
}

static void test_coffee_effect(TestApp &app) {
  afterhours::EntityID dish_id =
      setup_battle_with_drink(app, DishType::Potato, 0, DrinkType::Coffee);
  expect_drink_applied(app, dish_id);
  app.fire_trigger(TriggerHook::OnStartBattle, dish_id, 0,
                   DishBattleState::TeamSide::Player);
  app.wait_for_frames(60);
  expect_persistent_mods(app, dish_id, 2, 0);
}

static void test_red_soda_effect(TestApp &app) {
  afterhours::EntityID dish_id =
      setup_battle_with_drink(app, DishType::Potato, 0, DrinkType::RedSoda);
  app.fire_trigger(TriggerHook::OnCourseComplete, dish_id, 0,
                   DishBattleState::TeamSide::Player);
  app.wait_for_frames(60);
  expect_persistent_mods(app, dish_id, 1, 0);
}

static void test_blue_soda_effect(TestApp &app) {
  afterhours::EntityID dish_id =
      setup_battle_with_drink(app, DishType::Potato, 0, DrinkType::BlueSoda);
  app.fire_trigger(TriggerHook::OnCourseComplete, dish_id, 0,
                   DishBattleState::TeamSide::Player);
  app.wait_for_frames(60);
  expect_persistent_mods(app, dish_id, 0, 1);
}

static void test_watermelon_juice_effect(TestApp &app) {
  afterhours::EntityID dish_id = setup_battle_with_drink(
      app, DishType::Potato, 0, DrinkType::WatermelonJuice);
  app.fire_trigger(TriggerHook::OnCourseComplete, dish_id, 0,
                   DishBattleState::TeamSide::Player);
  app.wait_for_frames(60);

  DeferredFlavorMods expected_flavor;
  expected_flavor.freshness = 1;
  expect_flavor_and_combat(app, dish_id, expected_flavor, 0, 1);
}

static void test_yellow_soda_effect(TestApp &app) {
  afterhours::EntityID dish_id =
      setup_battle_with_drink(app, DishType::Potato, 0, DrinkType::YellowSoda);
  app.wait_for_frames(60);
  afterhours::Entity *dish = app.find_entity_by_id(dish_id);
  app.expect_true(dish != nullptr, "dish entity still exists");
  app.expect_true(dish->has<PersistentCombatModifiers>(),
                  "Yellow Soda added combat modifiers");
  app.expect_true(dish->get<PersistentCombatModifiers>().zingDelta >= 1,
                  "Yellow Soda gave at least +1 Zing per bite");
  app.expect_eq(dish->get<PersistentCombatModifiers>().bodyDelta, 0,
                "Yellow Soda body modifier");
}

static void test_green_soda_effect(TestApp &app) {
  afterhours::EntityID dish_id =
      setup_battle_with_drink(app, DishType::Potato, 0, DrinkType::GreenSoda);
  app.wait_for_frames(60);
  expect_persistent_mods(app, dish_id, 2, -1);
}

static void test_white_wine_effect(TestApp &app) {
  setup_battle_with_two_dishes(app, DrinkType::WhiteWine);
  app.fire_trigger(TriggerHook::OnStartBattle,
                   app.get_test_int("source_id").value(), 0,
                   DishBattleState::TeamSide::Player);
  app.wait_for_frames(60);
  afterhours::Entity *ally =
      app.find_entity_by_id(app.get_test_int("other_id").value());
  app.expect_true(ally != nullptr, "ally dish still exists");
  app.expect_true(ally->has<PersistentCombatModifiers>(),
                  "White Wine added combat modifiers to ally");
  app.expect_eq(ally->get<PersistentCombatModifiers>().zingDelta, 1,
                "White Wine gave ally +1 Zing");
}

static void test_red_wine_effect(TestApp &app) {
  setup_battle_with_two_dishes(app, DrinkType::RedWine);
  app.wait_for_frames(60);

  for (const char *key : {"source_id", "other_id"}) {
    afterhours::Entity *dish = app.find_entity_by_id(app.get_test_int(key).value());
    app.expect_true(dish != nullptr, std::string(key) + " still exists");
    bool richness_applied =
        (dish->has<DeferredFlavorMods>() &&
         dish->get<DeferredFlavorMods>().richness >= 1) ||
        (dish->has<PersistentCombatModifiers>() &&
         dish->get<PersistentCombatModifiers>().bodyDelta +
                 dish->get<PersistentCombatModifiers>().zingDelta >=
             1);
    app.expect_true(richness_applied,
                    std::string("Red Wine richness applied to ") + key);
  }
}

} // namespace ValidateDrinkEffectsTestHelpers

TEST(validate_drink_effects) {
  using namespace ValidateDrinkEffectsTestHelpers;
  using Step = void (*)(TestApp &);
  static const Step steps[] = {
      test_water_effect,        test_orange_juice_effect,
      test_coffee_effect,       test_red_soda_effect,
      test_blue_soda_effect,    test_watermelon_juice_effect,
      test_yellow_soda_effect,  test_green_soda_effect,
      test_white_wine_effect,   test_red_wine_effect,
  };
  static int phase = 0;

  for (; phase < static_cast<int>(std::size(steps)); ++phase) {
    log_info("DRINK_TEST: phase {}", phase);
    steps[phase](app);
    app.completed_operations.clear();
    app.created_entities.clear();
    app.test_int_data.clear();
  }
  log_info("DRINK_TEST: All tests completed");
}
