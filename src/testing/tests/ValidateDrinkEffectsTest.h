#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/combat_stats.h"
#include "../../components/deferred_flavor_mods.h"
#include "../../components/dish_battle_state.h"
#include "../../components/drink_effects.h"
#include "../../components/drink_pairing.h"
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

static afterhours::Entity &get_or_create_trigger_queue() {
  afterhours::RefEntity tq_ref =
      afterhours::EntityHelper::get_singleton<TriggerQueue>();
  afterhours::Entity &tq_entity = tq_ref.get();
  if (!tq_entity.has<TriggerQueue>()) {
    tq_entity.addComponent<TriggerQueue>();
    afterhours::EntityHelper::registerSingleton<TriggerQueue>(tq_entity);
  }
  return tq_entity;
}

static void ensure_battle_load_request_exists() {
  const auto componentId =
      afterhours::components::get_type_id<BattleLoadRequest>();
  auto &singletonMap = afterhours::EntityHelper::get().singletonMap;
  if (singletonMap.contains(componentId)) {
    return;
  }
  auto &requestEntity = afterhours::EntityHelper::createEntity();
  BattleLoadRequest request;
  request.playerJsonPath = "";
  request.opponentJsonPath = "";
  request.loaded = false;
  requestEntity.addComponent<BattleLoadRequest>(std::move(request));
  afterhours::EntityHelper::registerSingleton<BattleLoadRequest>(requestEntity);
}

static void test_water_effect(TestApp &app) {
  log_info("DRINK_TEST: Testing Water (no effect baseline)");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  auto dish_id = app.create_dish(DishType::Potato)
                     .on_team(DishBattleState::TeamSide::Player)
                     .at_slot(0)
                     .in_phase(DishBattleState::Phase::InQueue)
                     .commit();

  app.wait_for_frames(5);

  afterhours::OptEntity dish_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(dish_id)
          .gen_first();
  app.expect_true(dish_opt.has_value(), "dish entity exists");
  dish_opt.asE().addComponent<DrinkPairing>(DrinkType::Water);

  app.wait_for_frames(5);

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

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  auto dish_id = app.create_dish(DishType::Potato)
                     .on_team(DishBattleState::TeamSide::Player)
                     .at_slot(0)
                     .in_phase(DishBattleState::Phase::InQueue)
                     .commit();

  app.wait_for_frames(5);

  afterhours::OptEntity dish_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(dish_id)
          .gen_first();
  app.expect_true(dish_opt.has_value(), "dish entity exists");
  dish_opt.asE().addComponent<DrinkPairing>(DrinkType::OrangeJuice);

  app.wait_for_frames(20);

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

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnServe, dish_id, 0,
                  DishBattleState::TeamSide::Player);

  app.wait_for_frames(10);

  DeferredFlavorMods expected;
  expected.freshness = 1;
  app.expect_flavor_mods(dish_id, expected);

  log_info("DRINK_TEST: Orange Juice effect PASSED");
}

static void test_coffee_effect(TestApp &app) {
  log_info("DRINK_TEST: Testing Coffee (OnStartBattle +2 Zing to Self)");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  auto dish_id = app.create_dish(DishType::Potato)
                     .on_team(DishBattleState::TeamSide::Player)
                     .at_slot(0)
                     .in_phase(DishBattleState::Phase::InQueue)
                     .with_combat_stats()
                     .commit();

  app.wait_for_frames(5);

  afterhours::OptEntity dish_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(dish_id)
          .gen_first();
  app.expect_true(dish_opt.has_value(), "dish entity exists");
  dish_opt.asE().addComponent<DrinkPairing>(DrinkType::Coffee);

  app.wait_for_frames(20);

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

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnStartBattle, dish_id, 0,
                  DishBattleState::TeamSide::Player);

  app.wait_for_frames(10);

  PendingCombatMods expected;
  expected.zingDelta = 2;
  expected.bodyDelta = 0;
  app.expect_combat_mods(dish_id, expected);

  log_info("DRINK_TEST: Coffee effect PASSED");
}

} // namespace ValidateDrinkEffectsTestHelpers

TEST(validate_drink_effects) {
  using namespace ValidateDrinkEffectsTestHelpers;

  log_info("DRINK_TEST: Starting drink effects validation");

  test_water_effect(app);
  test_orange_juice_effect(app);
  test_coffee_effect(app);

  log_info("DRINK_TEST: All tests completed");
}
