#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/combat_stats.h"
#include "../../components/dish_battle_state.h"
#include "../../components/drink_effects.h"
#include "../../components/is_dish.h"
#include "../../components/status_effects.h"
#include "../../components/trigger_event.h"
#include "../../components/trigger_queue.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../query.h"
#include "../test_app.h"
#include "../test_macros.h"

namespace ValidateWagyuSteakTestHelpers {

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

static void test_wagyu_steak_applies_debuff(TestApp &app) {
  log_info(
      "WAGYU_TEST: Testing Wagyu Steak applies -1 Zing debuff to opponent");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  // Create Wagyu Steak dish
  auto wagyu_id = app.create_dish(DishType::WagyuSteak)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(0)
                      .in_phase(DishBattleState::Phase::InCombat)
                      .commit();

  // Create opponent dish
  auto opponent_id = app.create_dish(DishType::Potato)
                         .on_team(DishBattleState::TeamSide::Opponent)
                         .at_slot(0)
                         .in_phase(DishBattleState::Phase::InCombat)
                         .commit();

  app.wait_for_frames(5);

  // Get initial opponent stats
  afterhours::OptEntity opponent_before_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(opponent_id)
          .gen_first();
  app.expect_true(opponent_before_opt.has_value(), "opponent exists");

  int initial_zing = 0;
  if (opponent_before_opt.asE().has<CombatStats>()) {
    initial_zing = opponent_before_opt.asE().get<CombatStats>().baseZing;
  }

  // Trigger OnBiteTaken on Wagyu (when it takes damage)
  auto &tq_entity = get_or_create_trigger_queue();
  auto &tq = tq_entity.get<TriggerQueue>();
  TriggerEvent bite_event;
  bite_event.hook = TriggerHook::OnBiteTaken;
  bite_event.sourceEntityId = wagyu_id;
  tq.events.push_back(bite_event);

  app.wait_for_frames(10);

  // Check that opponent has StatusEffects with -1 Zing
  afterhours::OptEntity opponent_after_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(opponent_id)
          .gen_first();
  app.expect_true(opponent_after_opt.has_value(), "opponent still exists");

  bool has_status = opponent_after_opt.asE().has<StatusEffects>();
  app.expect_true(has_status,
                  "opponent has StatusEffects after Wagyu takes damage");

  if (has_status) {
    const auto &status = opponent_after_opt.asE().get<StatusEffects>();
    bool found_debuff = false;
    for (const auto &effect : status.effects) {
      if (effect.zingDelta == -1) {
        found_debuff = true;
        break;
      }
    }
    app.expect_true(found_debuff,
                    "opponent has -1 Zing debuff from Wagyu Steak");
  }

  log_info("WAGYU_TEST: Wagyu Steak applies debuff PASSED");
}

static void test_apply_status_with_both_stats(TestApp &app) {
  log_info("WAGYU_TEST: Testing ApplyStatus with both zingDelta and bodyDelta");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  // Create source dish
  auto source_id = app.create_dish(DishType::Potato)
                       .on_team(DishBattleState::TeamSide::Player)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::InQueue)
                       .commit();

  // Create target dish
  auto target_id = app.create_dish(DishType::Salmon)
                       .on_team(DishBattleState::TeamSide::Opponent)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::InQueue)
                       .commit();

  app.wait_for_frames(5);

  // Add ApplyStatus effect with both zingDelta and bodyDelta
  afterhours::OptEntity source_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(source_id)
          .gen_first();
  app.expect_true(source_opt.has_value(), "source dish exists");

  auto &source = source_opt.asE();
  auto &drink_effects = source.addComponentIfMissing<DrinkEffects>();
  DishEffect status_effect(TriggerHook::OnServe, EffectOperation::ApplyStatus,
                           TargetScope::Opponent, -2);
  status_effect.statusBodyDelta = -1;
  drink_effects.effects.push_back(status_effect);

  // Trigger OnServe
  auto &tq_entity = get_or_create_trigger_queue();
  auto &tq = tq_entity.get<TriggerQueue>();
  TriggerEvent serve_event;
  serve_event.hook = TriggerHook::OnServe;
  serve_event.sourceEntityId = source_id;
  tq.events.push_back(serve_event);

  app.wait_for_frames(10);

  // Check that target has StatusEffects with both zingDelta and bodyDelta
  afterhours::OptEntity target_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(target_id)
          .gen_first();
  app.expect_true(target_opt.has_value(), "target dish exists");

  bool has_status = target_opt.asE().has<StatusEffects>();
  app.expect_true(has_status, "target has StatusEffects after ApplyStatus");

  if (has_status) {
    const auto &status = target_opt.asE().get<StatusEffects>();
    bool found_both_stats = false;
    for (const auto &effect : status.effects) {
      if (effect.zingDelta == -2 && effect.bodyDelta == -1) {
        found_both_stats = true;
        break;
      }
    }
    app.expect_true(
        found_both_stats,
        "target has status effect with both zingDelta -2 and bodyDelta -1");
  }

  log_info("WAGYU_TEST: ApplyStatus with both stats PASSED");
}

} // namespace ValidateWagyuSteakTestHelpers

REGISTER_TEST(validate_wagyu_steak_applies_debuff,
              ValidateWagyuSteakTestHelpers::test_wagyu_steak_applies_debuff);

REGISTER_TEST(validate_apply_status_with_both_stats,
              ValidateWagyuSteakTestHelpers::test_apply_status_with_both_stats);
