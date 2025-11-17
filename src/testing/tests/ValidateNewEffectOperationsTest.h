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

static void test_swap_stats_effect(TestApp &app) {
  log_info("EFFECT_OP_TEST: Testing SwapStats operation");

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
  auto &dish = dish_opt.asE();

  auto &stats = dish.addComponentIfMissing<CombatStats>();
  stats.baseZing = 5;
  stats.baseBody = 3;
  stats.currentZing = 5;
  stats.currentBody = 3;

  auto &drink_effects = dish.addComponent<DrinkEffects>();
  DishEffect swap_effect(TriggerHook::OnServe, EffectOperation::SwapStats,
                         TargetScope::Self, 0);
  drink_effects.effects.push_back(swap_effect);

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnServe, dish_id, 0,
                  DishBattleState::TeamSide::Player);

  app.wait_for_frames(10);

  app.expect_true(dish.has<CombatStats>(), "CombatStats still exists");
  auto &stats_after = dish.get<CombatStats>();
  app.expect_eq(stats_after.baseZing, 3, "baseZing swapped to 3");
  app.expect_eq(stats_after.baseBody, 5, "baseBody swapped to 5");
  app.expect_eq(stats_after.currentZing, 3, "currentZing swapped to 3");
  app.expect_eq(stats_after.currentBody, 5, "currentBody swapped to 5");

  log_info("EFFECT_OP_TEST: SwapStats effect PASSED");
}

static void test_multiply_damage_effect(TestApp &app) {
  log_info("EFFECT_OP_TEST: Testing MultiplyDamage operation");

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
  auto &dish = dish_opt.asE();

  auto &drink_effects = dish.addComponent<DrinkEffects>();
  DishEffect multiply_effect(TriggerHook::OnServe,
                             EffectOperation::MultiplyDamage, TargetScope::Self,
                             2);
  drink_effects.effects.push_back(multiply_effect);

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnServe, dish_id, 0,
                  DishBattleState::TeamSide::Player);

  app.wait_for_frames(10);

  app.expect_true(dish.has<NextDamageEffect>(),
                  "NextDamageEffect component exists");
  auto &next_effect = dish.get<NextDamageEffect>();
  app.expect_eq(next_effect.multiplier, 2.0f, "multiplier is 2.0");
  app.expect_eq(next_effect.count, 1, "count is 1");

  log_info("EFFECT_OP_TEST: MultiplyDamage effect PASSED");
}

static void test_prevent_all_damage_effect(TestApp &app) {
  log_info("EFFECT_OP_TEST: Testing PreventAllDamage operation");

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
  auto &dish = dish_opt.asE();

  auto &drink_effects = dish.addComponent<DrinkEffects>();
  DishEffect prevent_effect(TriggerHook::OnServe,
                            EffectOperation::PreventAllDamage,
                            TargetScope::Self, 2);
  drink_effects.effects.push_back(prevent_effect);

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnServe, dish_id, 0,
                  DishBattleState::TeamSide::Player);

  app.wait_for_frames(10);

  app.expect_true(dish.has<NextDamageEffect>(),
                  "NextDamageEffect component exists");
  auto &next_effect = dish.get<NextDamageEffect>();
  app.expect_eq(next_effect.multiplier, 0.0f, "multiplier is 0.0");
  app.expect_eq(next_effect.count, 2, "count is 2");

  log_info("EFFECT_OP_TEST: PreventAllDamage effect PASSED");
}

} // namespace ValidateNewEffectOperationsTestHelpers

TEST(validate_new_effect_operations) {
  using namespace ValidateNewEffectOperationsTestHelpers;

  log_info("EFFECT_OP_TEST: Starting new effect operations validation");

  test_swap_stats_effect(app);
  test_multiply_damage_effect(app);
  test_prevent_all_damage_effect(app);

  log_info("EFFECT_OP_TEST: All tests completed");
}
