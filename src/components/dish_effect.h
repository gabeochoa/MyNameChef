#pragma once

#include "trigger_event.h"
#include <afterhours/ah.h>
#include <optional>

// Forward declaration to avoid circular dependency with dish_types.h
enum struct DishType;

enum struct EffectOperation {
  AddFlavorStat,
  AddCombatZing,
  AddCombatBody,
  SwapStats,
  MultiplyDamage,
  PreventAllDamage,
  CopyEffect,
  SummonDish,
  ApplyStatus
};

enum struct TargetScope {
  Self,
  Opponent,
  AllAllies,
  AllOpponents,
  DishesAfterSelf,
  FutureAllies,
  FutureOpponents,
  Previous,
  Next,
  SelfAndAdjacent,
  RandomAlly,
  RandomOpponent,
  RandomDish,
  RandomOtherAlly
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
  FlavorStatType adjacentCheckStat = FlavorStatType::Freshness;
  bool playFreshnessChainAnimation = false;
  bool is_copied = false; // True if this effect was copied from another dish
  std::optional<DishType>
      summonDishType; // Dish type to summon (for SummonDish operation)
  int statusBodyDelta =
      0; // Body delta for ApplyStatus operation (amount is zingDelta)

  DishEffect() = default;

  DishEffect(TriggerHook hook, EffectOperation op, TargetScope scope, int amt)
      : triggerHook(hook), operation(op), targetScope(scope), amount(amt) {}

  DishEffect(TriggerHook hook, EffectOperation op, TargetScope scope, int amt,
             FlavorStatType statType)
      : triggerHook(hook), operation(op), targetScope(scope), amount(amt),
        flavorStatType(statType) {}
};
