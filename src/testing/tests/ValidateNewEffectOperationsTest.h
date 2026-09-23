#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/combat_stats.h"
#include "../../components/dish_battle_state.h"
#include "../../components/dish_effect.h"
#include "../../components/drink_effects.h"
#include "../../components/next_damage_effect.h"
#include "../../components/trigger_queue.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../test_app.h"
#include "../test_macros.h"

namespace ValidateNewEffectOperationsTestHelpers {

static void ensure_battle_load_request_exists() {
  if (afterhours::EntityHelper::has_singleton<BattleLoadRequest>()) {
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

static void test_swap_stats_effect(TestApp &app) {
  if (app.has_test_int("swap_stats")) {
    return;
  }
  log_info("EFFECT_OP_TEST: Testing SwapStats operation");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  auto dish_id = app.create_dish(DishType::WagyuSteak)
                     .on_team(DishBattleState::TeamSide::Player)
                     .at_slot(0)
                     .in_phase(DishBattleState::Phase::InQueue)
                     .with_combat_stats()
                     .with_onserve_fired()
                     .commit();

  app.wait_for_frames(5);

  afterhours::OptEntity dish_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(dish_id)
          .gen_first();
  app.expect_true(dish_opt.has_value(), "dish entity exists");
  auto &dish = dish_opt.asE();

  if (!app.has_test_int("swap_zing_before")) {
    CombatStats &stats_before = dish.get<CombatStats>();
    app.set_test_int("swap_zing_before", stats_before.baseZing);
    app.set_test_int("swap_body_before", stats_before.baseBody);
  }
  int zing_before = app.get_test_int("swap_zing_before").value();
  int body_before = app.get_test_int("swap_body_before").value();
  app.expect_true(zing_before != body_before, "dish has asymmetric stats");

  auto &drink_effects = dish.addComponentIfMissing<DrinkEffects>();
  if (drink_effects.effects.empty()) {
    DishEffect swap_effect(TriggerHook::OnServe, EffectOperation::SwapStats,
                           TargetScope::Self, 0);
    drink_effects.effects.push_back(swap_effect);
  }

  app.fire_trigger(TriggerHook::OnServe, dish_id, 0,
                   DishBattleState::TeamSide::Player);

  app.wait_for_frames(10);

  app.expect_true(dish.has<CombatStats>(), "CombatStats still exists");
  auto &stats_after = dish.get<CombatStats>();
  app.expect_eq(stats_after.baseZing, body_before, "baseZing swapped");
  app.expect_eq(stats_after.baseBody, zing_before, "baseBody swapped");
  app.expect_eq(stats_after.currentZing, body_before, "currentZing swapped");
  app.expect_eq(stats_after.currentBody, zing_before, "currentBody swapped");

  app.set_test_int("swap_stats", 1);
  log_info("EFFECT_OP_TEST: SwapStats effect PASSED");
}

static void test_multiply_damage_effect(TestApp &app) {
  if (app.has_test_int("multiply_damage")) {
    return;
  }
  log_info("EFFECT_OP_TEST: Testing MultiplyDamage operation");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  auto dish_id = app.create_dish(DishType::Potato)
                     .on_team(DishBattleState::TeamSide::Player)
                     .at_slot(0)
                     .in_phase(DishBattleState::Phase::InQueue)
                     .with_combat_stats()
                     .with_onserve_fired()
                     .commit();

  app.wait_for_frames(5);

  afterhours::OptEntity dish_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(dish_id)
          .gen_first();
  app.expect_true(dish_opt.has_value(), "dish entity exists");
  auto &dish = dish_opt.asE();

  auto &drink_effects = dish.addComponentIfMissing<DrinkEffects>();
  if (drink_effects.effects.empty()) {
    DishEffect multiply_effect(TriggerHook::OnServe,
                               EffectOperation::MultiplyDamage,
                               TargetScope::Self, 2);
    drink_effects.effects.push_back(multiply_effect);
  }

  app.fire_trigger(TriggerHook::OnServe, dish_id, 0,
                  DishBattleState::TeamSide::Player);

  app.wait_for_frames(10);

  app.expect_true(dish.has<NextDamageEffect>(),
                  "NextDamageEffect component exists");
  auto &next_effect = dish.get<NextDamageEffect>();
  app.expect_eq(next_effect.multiplier, 2.0f, "multiplier is 2.0");
  app.expect_eq(next_effect.count, 1, "count is 1");

  app.set_test_int("multiply_damage", 1);
  log_info("EFFECT_OP_TEST: MultiplyDamage effect PASSED");
}

static void test_prevent_all_damage_effect(TestApp &app) {
  if (app.has_test_int("prevent_all_damage")) {
    return;
  }
  log_info("EFFECT_OP_TEST: Testing PreventAllDamage operation");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  auto dish_id = app.create_dish(DishType::Potato)
                     .on_team(DishBattleState::TeamSide::Player)
                     .at_slot(0)
                     .in_phase(DishBattleState::Phase::InQueue)
                     .with_combat_stats()
                     .with_onserve_fired()
                     .commit();

  app.wait_for_frames(5);

  afterhours::OptEntity dish_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(dish_id)
          .gen_first();
  app.expect_true(dish_opt.has_value(), "dish entity exists");
  auto &dish = dish_opt.asE();

  auto &drink_effects = dish.addComponentIfMissing<DrinkEffects>();
  if (drink_effects.effects.empty()) {
    DishEffect prevent_effect(TriggerHook::OnServe,
                              EffectOperation::PreventAllDamage,
                              TargetScope::Self, 2);
    drink_effects.effects.push_back(prevent_effect);
  }

  app.fire_trigger(TriggerHook::OnServe, dish_id, 0,
                  DishBattleState::TeamSide::Player);

  app.wait_for_frames(10);

  app.expect_true(dish.has<NextDamageEffect>(),
                  "NextDamageEffect component exists");
  auto &next_effect = dish.get<NextDamageEffect>();
  app.expect_eq(next_effect.multiplier, 0.0f, "multiplier is 0.0");
  app.expect_eq(next_effect.count, 2, "count is 2");

  app.set_test_int("prevent_all_damage", 1);
  log_info("EFFECT_OP_TEST: PreventAllDamage effect PASSED");
}

} // namespace ValidateNewEffectOperationsTestHelpers

TEST(validate_new_effect_operations) {
  using namespace ValidateNewEffectOperationsTestHelpers;

  log_info("EFFECT_OP_TEST: Starting new effect operations validation");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);
  app.create_dish(DishType::Potato)
      .on_team(DishBattleState::TeamSide::Player)
      .at_slot(6)
      .in_phase(DishBattleState::Phase::InQueue)
      .with_combat_stats()
      .with_onserve_fired()
      .commit();
  app.create_dish(DishType::Potato)
      .on_team(DishBattleState::TeamSide::Opponent)
      .at_slot(6)
      .in_phase(DishBattleState::Phase::InQueue)
      .with_combat_stats()
      .with_onserve_fired()
      .commit();
  app.wait_for_frames(1);

  test_swap_stats_effect(app);
  test_multiply_damage_effect(app);
  test_prevent_all_damage_effect(app);

  log_info("EFFECT_OP_TEST: All tests completed");
}
