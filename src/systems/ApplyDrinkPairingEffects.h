#pragma once

#include "../components/dish_battle_state.h"
#include "../components/dish_effect.h"
#include "../components/drink_effects.h"
#include "../components/drink_pairing.h"
#include "../components/is_dish.h"
#include "../drink_types.h"
#include "../game_state_manager.h"
#include "../log.h"
#include "../query.h"
#include <afterhours/ah.h>
#include <vector>

struct ApplyDrinkPairingEffects
    : afterhours::System<IsDish, DishBattleState, DrinkPairing> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &e, IsDish &, DishBattleState &dbs,
                     DrinkPairing &drink_pairing, float) override {
    if (dbs.phase != DishBattleState::Phase::InQueue) {
      return;
    }

    if (e.has<DrinkEffects>()) {
      return;
    }

    if (!drink_pairing.drink.has_value()) {
      return;
    }

    DrinkType drink_type = drink_pairing.drink.value();
    std::vector<DishEffect> effects = get_drink_effects(drink_type);
    auto &drink_effects = e.addComponent<DrinkEffects>();
    drink_effects.effects = std::move(effects);
  }

private:
  std::vector<DishEffect> get_drink_effects(DrinkType type) {
    switch (type) {
    case DrinkType::Water:
      return {};
    case DrinkType::OrangeJuice:
      return {DishEffect(TriggerHook::OnServe, EffectOperation::AddFlavorStat,
                         TargetScope::Self, 1, FlavorStatType::Freshness)};
    case DrinkType::Coffee:
      return {DishEffect(TriggerHook::OnStartBattle,
                         EffectOperation::AddCombatZing, TargetScope::Self, 2)};
    case DrinkType::RedSoda:
      return {DishEffect(TriggerHook::OnCourseComplete,
                         EffectOperation::AddCombatZing, TargetScope::Self, 1)};
    case DrinkType::HotCocoa:
      return {DishEffect(TriggerHook::OnServe, EffectOperation::AddFlavorStat,
                         TargetScope::Self, 1, FlavorStatType::Sweetness),
              DishEffect(TriggerHook::OnServe, EffectOperation::AddFlavorStat,
                         TargetScope::Self, 1, FlavorStatType::Richness)};
    case DrinkType::RedWine:
      return {DishEffect(TriggerHook::OnServe, EffectOperation::AddFlavorStat,
                         TargetScope::Self, 1, FlavorStatType::Richness),
              DishEffect(TriggerHook::OnServe, EffectOperation::AddFlavorStat,
                         TargetScope::Next, 1, FlavorStatType::Richness)};
    case DrinkType::GreenSoda:
      return {DishEffect(TriggerHook::OnServe, EffectOperation::AddCombatZing,
                         TargetScope::Self, 2),
              DishEffect(TriggerHook::OnServe, EffectOperation::AddCombatBody,
                         TargetScope::Self, -1)};
    case DrinkType::BlueSoda:
      return {DishEffect(TriggerHook::OnCourseComplete,
                         EffectOperation::AddCombatBody, TargetScope::Self, 1)};
    case DrinkType::WhiteWine:
      return {DishEffect(TriggerHook::OnStartBattle,
                         EffectOperation::AddCombatZing, TargetScope::AllAllies,
                         1)};
    case DrinkType::WatermelonJuice:
      return {DishEffect(TriggerHook::OnCourseComplete,
                         EffectOperation::AddFlavorStat, TargetScope::Self, 1,
                         FlavorStatType::Freshness),
              DishEffect(TriggerHook::OnCourseComplete,
                         EffectOperation::AddCombatBody, TargetScope::Self, 1)};
    case DrinkType::YellowSoda:
      return {DishEffect(TriggerHook::OnBiteTaken,
                         EffectOperation::AddCombatZing, TargetScope::Self, 1)};
    default:
      log_error("ApplyDrinkPairingEffects: Unhandled drink type");
      return {};
    }
  }
};
