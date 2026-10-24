#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/combat_stats.h"
#include "../../components/dish_battle_state.h"
#include "../../components/dish_effect.h"
#include "../../components/drink_effects.h"
#include "../../components/status_effects.h"
#include "../../components/trigger_event.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../../query.h"
#include "../test_app.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

namespace ValidateEffectChainsTestHelpers {
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
} // namespace ValidateEffectChainsTestHelpers

TEST(validate_effect_chain_garlic_bread) {
  ValidateEffectChainsTestHelpers::ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  auto garlic_id = app.create_dish(DishType::GarlicBread)
                       .on_team(DishBattleState::TeamSide::Player)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::Entering)
                       .commit();

  auto wagyu_id = app.create_dish(DishType::WagyuSteak)
                      .on_team(DishBattleState::TeamSide::Opponent)
                      .at_slot(0)
                      .in_phase(DishBattleState::Phase::InQueue)
                      .commit();

  app.wait_for_frames(5);

  app.fire_trigger(TriggerHook::OnServe, garlic_id, 0,
                   DishBattleState::TeamSide::Player);
  app.wait_for_frames(10);

  auto *wagyu = app.find_entity_by_id(wagyu_id);
  app.expect_true(wagyu != nullptr, "Wagyu still exists after OnServe");
  if (wagyu != nullptr) {
    app.expect_true(wagyu->has<StatusEffects>(),
                    "Wagyu has StatusEffects after GarlicBread OnServe");
    if (wagyu->has<StatusEffects>()) {
      const auto &effects = wagyu->get<StatusEffects>().effects;
      app.expect_eq(static_cast<int>(effects.size()), 1,
                    "one status effect: chained effect adds no status");
      if (!effects.empty()) {
        app.expect_true(effects[0].type == StatusEffectType::Fatigue,
                        "parent effect applied Fatigue");
      }
    }
    app.expect_true(wagyu->has<CombatStats>(), "Wagyu has CombatStats");
    if (wagyu->has<CombatStats>()) {
      app.expect_eq(wagyu->get<CombatStats>().baseZing, 2,
                    "chained effect reduces Wagyu base Zing from 3 to 2");
    }
  }
}

TEST(validate_effect_chain_conditional_blocked) {
  ValidateEffectChainsTestHelpers::ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  auto wagyu_id = app.create_dish(DishType::WagyuSteak)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(0)
                      .in_phase(DishBattleState::Phase::InCombat)
                      .commit();

  auto potato_id = app.create_dish(DishType::Potato)
                       .on_team(DishBattleState::TeamSide::Opponent)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::InCombat)
                       .commit();

  app.wait_for_frames(5);

  app.fire_trigger(TriggerHook::OnBiteTaken, wagyu_id, 0,
                   DishBattleState::TeamSide::Player);
  app.wait_for_frames(10);

  auto *potato = app.find_entity_by_id(potato_id);
  app.expect_true(potato != nullptr, "Potato still exists after OnBiteTaken");
  if (potato != nullptr) {
    app.expect_true(potato->has<StatusEffects>(),
                    "Potato has StatusEffects after Wagyu OnBiteTaken");
    if (potato->has<StatusEffects>()) {
      const auto &effects = potato->get<StatusEffects>().effects;
      app.expect_eq(static_cast<int>(effects.size()), 1,
                    "one status effect from parent");
      if (!effects.empty()) {
        app.expect_eq(effects[0].zingDelta, -1,
                      "parent effect applies -1 Zing");
      }
    }
    app.expect_true(potato->has<CombatStats>(), "Potato has CombatStats");
    if (potato->has<CombatStats>()) {
      app.expect_eq(potato->get<CombatStats>().baseBody, 1,
                    "chained Body loss blocked: Potato has no Fatigue");
    }
  }
}

TEST(validate_effect_chain_conditional_fires_with_status) {
  ValidateEffectChainsTestHelpers::ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  auto garlic_id = app.create_dish(DishType::GarlicBread)
                       .on_team(DishBattleState::TeamSide::Player)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::Entering)
                       .commit();

  auto wagyu_id = app.create_dish(DishType::WagyuSteak)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(1)
                      .in_phase(DishBattleState::Phase::InCombat)
                      .commit();

  auto potato_id = app.create_dish(DishType::Potato)
                       .on_team(DishBattleState::TeamSide::Opponent)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::InCombat)
                       .commit();

  app.wait_for_frames(5);

  app.fire_trigger(TriggerHook::OnServe, garlic_id, 0,
                   DishBattleState::TeamSide::Player);
  app.wait_for_frames(5);

  app.fire_trigger(TriggerHook::OnBiteTaken, wagyu_id, 1,
                   DishBattleState::TeamSide::Player);
  app.wait_for_frames(10);

  auto *potato = app.find_entity_by_id(potato_id);
  app.expect_true(potato != nullptr, "Potato still exists after chain");
  if (potato != nullptr) {
    app.expect_true(potato->has<CombatStats>(), "Potato has CombatStats");
    if (potato->has<CombatStats>()) {
      app.expect_eq(potato->get<CombatStats>().baseBody, 0,
                    "chained Body loss fires: Potato has Fatigue");
    }
  }
}

TEST(validate_effect_chain_depth_limit) {
  ValidateEffectChainsTestHelpers::ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  auto source_id = app.create_dish(DishType::Potato)
                       .on_team(DishBattleState::TeamSide::Player)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::Entering)
                       .commit();

  app.wait_for_frames(5);

  app.run_once("depth_inject", [&app, source_id]() {
    afterhours::OptEntity source_opt =
        afterhours::EntityQuery({.force_merge = true})
            .whereID(source_id)
            .gen_first();
    app.expect_true(source_opt.has_value(), "source dish exists");
    if (source_opt.has_value()) {
      DishEffect chain(TriggerHook::OnBiteTaken,
                       EffectOperation::AddCombatZing, TargetScope::Self, 1);
      for (int i = 0; i < 14; i++) {
        DishEffect parent(TriggerHook::OnBiteTaken,
                          EffectOperation::AddCombatZing, TargetScope::Self, 1);
        parent.chainedEffects.push_back(chain);
        chain = parent;
      }
      auto &source = source_opt.asE();
      auto &drink_effects = source.addComponentIfMissing<DrinkEffects>();
      drink_effects.effects.push_back(chain);
    }
  });

  app.fire_trigger(TriggerHook::OnBiteTaken, source_id, 0,
                   DishBattleState::TeamSide::Player);
  app.wait_for_frames(10);

  auto *source_after = app.find_entity_by_id(source_id);
  app.expect_true(source_after != nullptr, "source still exists after chain");
  if (source_after != nullptr) {
    app.expect_true(source_after->has<CombatStats>(),
                    "source has CombatStats");
    if (source_after->has<CombatStats>()) {
      app.expect_eq(source_after->get<CombatStats>().baseZing, 11,
                    "15-deep chain stops at depth limit: 1 parent + 10 chained");
    }
  }
}
