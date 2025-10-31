#pragma once

#include "trigger_event.h"
#include <afterhours/ah.h>

enum struct EffectOperation { AddFlavorStat, AddCombatZing, AddCombatBody };

enum struct TargetScope {
  Self,
  Opponent,
  AllAllies,
  AllOpponents,
  DishesAfterSelf,
  FutureAllies,
  FutureOpponents,
  Previous,
  Next
};

enum struct FlavorStatType {
  Satiety,
  Sweetness,
  Spice,
  Acidity,
  Umami,
  Richness,
  Freshness
};

struct DishEffect {
  TriggerHook triggerHook = TriggerHook::OnServe;
  EffectOperation operation = EffectOperation::AddFlavorStat;
  TargetScope targetScope = TargetScope::Self;
  int amount = 0;
  FlavorStatType flavorStatType = FlavorStatType::Satiety;

  bool conditional = false;

  DishEffect() = default;

  DishEffect(TriggerHook hook, EffectOperation op, TargetScope scope, int amt)
      : triggerHook(hook), operation(op), targetScope(scope), amount(amt) {}

  DishEffect(TriggerHook hook, EffectOperation op, TargetScope scope, int amt,
             FlavorStatType statType)
      : triggerHook(hook), operation(op), targetScope(scope), amount(amt),
        flavorStatType(statType) {}
};
