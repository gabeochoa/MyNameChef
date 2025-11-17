#pragma once

#include "../components/combat_stats.h"
#include "../components/deferred_flavor_mods.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_effect.h"
#include "../components/drink_effects.h"
#include "../components/is_dish.h"
#include "../components/next_damage_effect.h"
#include "../components/pending_combat_mods.h"
#include "../components/synergy_bonus_effects.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../shop.h"
#include <afterhours/ah.h>
#include <optional>
#include <vector>

struct EffectResolutionSystem : afterhours::System<TriggerQueue> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Battle) {
      return false;
    }
    if (isReplayPaused()) {
      return false;
    }
    return true;
  }

  void for_each_with(afterhours::Entity &, TriggerQueue &queue,
                     float) override {
    if (queue.empty()) {
      return;
    }

    for (const auto &ev : queue.events) {
      process_trigger_event(ev);
    }

    // Clear queue after processing to prevent duplicate processing
    // TriggerDispatchSystem will need to be updated to work with cleared queue
    queue.clear();
  }

private:
  void process_trigger_event(const TriggerEvent &ev) {
    auto src_opt = EQ({.ignore_temp_warning = true})
                       .whereID(ev.sourceEntityId)
                       .gen_first();
    if (!src_opt || !src_opt->has<IsDish>()) {
      return;
    }

    const auto &dish = src_opt->get<IsDish>();
    int level = 1;
    if (src_opt->has<DishLevel>()) {
      level = src_opt->get<DishLevel>().level;
    }
    const auto &info = get_dish_info(dish.type, level);

    for (const auto &effect : info.effects) {
      if (effect.triggerHook == ev.hook) {
        apply_effect(effect, ev);
      }
    }

    if (src_opt->has<DrinkEffects>()) {
      const auto &drink_effects = src_opt->get<DrinkEffects>();
      for (const auto &effect : drink_effects.effects) {
        if (effect.triggerHook == ev.hook) {
          apply_effect(effect, ev);
        }
      }
    }

    if (src_opt->has<SynergyBonusEffects>()) {
      const auto &synergy_effects = src_opt->get<SynergyBonusEffects>();
      for (const auto &effect : synergy_effects.effects) {
        if (effect.triggerHook == ev.hook) {
          apply_effect(effect, ev);
        }
      }
    }
  }

  void apply_effect(const DishEffect &effect, const TriggerEvent &ev) {
    if (effect.conditional) {
      if (!check_conditional(effect, ev)) {
        return;
      }
    }

    auto targets = get_targets(effect.targetScope, ev);

    for (afterhours::Entity &target : targets) {
      apply_to_target(target, effect);
    }

    if (effect.playFreshnessChainAnimation) {
      trigger_freshness_chain_animation(ev, targets);
    }
  }

  void
  trigger_freshness_chain_animation(const TriggerEvent &ev,
                                    const afterhours::RefEntities &targets) {
    // TODO: Come back and think more about how to make a more robust system for
    // this. Currently this is hardcoded for FreshnessChain animation and
    // requires a boolean flag per animation type, which doesn't scale well.
    // Consider making animations declarative and part of the effect definition.
    int sourceEntityId = ev.sourceEntityId;
    int previousEntityId = -1;
    int nextEntityId = -1;

    auto src_opt = EQ({.ignore_temp_warning = true})
                       .whereID(ev.sourceEntityId)
                       .gen_first();
    if (!src_opt || !src_opt->has<DishBattleState>()) {
      return;
    }

    const auto &src_dbs = src_opt->get<DishBattleState>();
    const int src_queue_index = src_dbs.queue_index;

    for (afterhours::Entity &target : targets) {
      if (!target.has<DishBattleState>()) {
        continue;
      }
      const auto &dbs = target.get<DishBattleState>();
      if (dbs.queue_index == src_queue_index - 1) {
        previousEntityId = target.id;
      } else if (dbs.queue_index == src_queue_index + 1) {
        nextEntityId = target.id;
      }
    }

    make_freshness_chain_animation(sourceEntityId, previousEntityId,
                                   nextEntityId);
  }

  bool check_conditional(const DishEffect &effect, const TriggerEvent &ev) {
    if (!effect.conditional) {
      return true;
    }

    auto src_opt = EQ({.ignore_temp_warning = true})
                       .whereID(ev.sourceEntityId)
                       .gen_first();
    if (!src_opt || !src_opt->has<DishBattleState>()) {
      return false;
    }

    const auto &src_dbs = src_opt->get<DishBattleState>();
    const int src_queue_index = src_dbs.queue_index;

    auto prevDish = find_previous_dish_in_queue(src_dbs, src_queue_index);
    if (prevDish.has_value()) {
      const auto &dish = prevDish.value()->get<IsDish>();
      const auto &dish_info = get_dish_info(dish.type);
      if (get_flavor_stat_value(dish_info.flavor, effect.adjacentCheckStat) >
          0) {
        return true;
      }
    }

    auto nextDish = find_next_dish_in_queue(src_dbs, src_queue_index);
    if (nextDish.has_value()) {
      const auto &dish = nextDish.value()->get<IsDish>();
      const auto &dish_info = get_dish_info(dish.type);
      if (get_flavor_stat_value(dish_info.flavor, effect.adjacentCheckStat) >
          0) {
        return true;
      }
    }

    return false;
  }

  int get_flavor_stat_value(const FlavorStats &stats, FlavorStatType stat) {
    switch (stat) {
    case FlavorStatType::Satiety:
      return stats.satiety;
    case FlavorStatType::Sweetness:
      return stats.sweetness;
    case FlavorStatType::Spice:
      return stats.spice;
    case FlavorStatType::Acidity:
      return stats.acidity;
    case FlavorStatType::Umami:
      return stats.umami;
    case FlavorStatType::Richness:
      return stats.richness;
    case FlavorStatType::Freshness:
      return stats.freshness;
    }
    return 0;
  }

  std::optional<afterhours::Entity *>
  find_previous_dish_in_queue(const DishBattleState &src_dbs,
                              int src_queue_index) {
    auto prev = EQ({.force_merge = true})
                    .whereHasComponent<IsDish>()
                    .whereHasComponent<DishBattleState>()
                    .whereLambda([&src_dbs, src_queue_index](
                                     const afterhours::Entity &e) {
                      const DishBattleState &dbs = e.get<DishBattleState>();
                      return dbs.team_side == src_dbs.team_side &&
                             dbs.queue_index == src_queue_index - 1 &&
                             dbs.phase == DishBattleState::Phase::InQueue;
                    })
                    .orderByLambda(
                        [](const afterhours::Entity &a,
                           const afterhours::Entity &b) { return a.id > b.id; })
                    .gen_first();
    if (prev.has_value()) {
      return prev.value();
    }
    return std::nullopt;
  }

  std::optional<afterhours::Entity *>
  find_next_dish_in_queue(const DishBattleState &src_dbs, int src_queue_index) {
    auto next = EQ({.force_merge = true})
                    .whereHasComponent<IsDish>()
                    .whereHasComponent<DishBattleState>()
                    .whereLambda([&src_dbs, src_queue_index](
                                     const afterhours::Entity &e) {
                      const DishBattleState &dbs = e.get<DishBattleState>();
                      return dbs.team_side == src_dbs.team_side &&
                             dbs.queue_index == src_queue_index + 1 &&
                             dbs.phase == DishBattleState::Phase::InQueue;
                    })
                    .orderByLambda(
                        [](const afterhours::Entity &a,
                           const afterhours::Entity &b) { return a.id > b.id; })
                    .gen_first();
    if (next.has_value()) {
      return next.value();
    }
    return std::nullopt;
  }

  afterhours::RefEntities get_targets(TargetScope scope,
                                      const TriggerEvent &ev) {
    afterhours::RefEntities targets;

    auto src_opt = EQ({.ignore_temp_warning = true})
                       .whereID(ev.sourceEntityId)
                       .gen_first();
    if (!src_opt || !src_opt->has<DishBattleState>()) {
      return targets;
    }

    const auto &src_dbs = src_opt->get<DishBattleState>();
    const int src_queue_index = src_dbs.queue_index;
    const auto src_team_side = src_dbs.team_side;
    const auto opposite_side =
        (src_team_side == DishBattleState::TeamSide::Player)
            ? DishBattleState::TeamSide::Opponent
            : DishBattleState::TeamSide::Player;

    switch (scope) {
    case TargetScope::Self: {
      if (src_opt.has_value()) {
        targets.push_back(*src_opt.value());
      }
      break;
    }

    case TargetScope::Opponent: {
      auto opponent = EQ({.force_merge = true})
                          .whereHasComponent<IsDish>()
                          .whereHasComponent<DishBattleState>()
                          .whereTeamSide(opposite_side)
                          .whereInSlotIndex(src_queue_index)
                          .gen_first();
      if (opponent.has_value()) {
        targets.push_back(*opponent.value());
      }
      break;
    }

    case TargetScope::AllAllies: {
      auto allies = EQ({.force_merge = true})
                        .whereHasComponent<IsDish>()
                        .whereHasComponent<DishBattleState>()
                        .whereTeamSide(src_team_side)
                        .whereNotID(ev.sourceEntityId)
                        .gen();
      for (afterhours::Entity &e : allies) {
        targets.push_back(e);
      }
      break;
    }

    case TargetScope::AllOpponents: {
      auto opponents = EQ({.force_merge = true})
                           .whereHasComponent<IsDish>()
                           .whereHasComponent<DishBattleState>()
                           .whereTeamSide(opposite_side)
                           .gen();
      for (afterhours::Entity &e : opponents) {
        targets.push_back(e);
      }
      break;
    }

    case TargetScope::DishesAfterSelf: {
      auto after =
          EQ({.ignore_temp_warning = true})
              .whereHasComponent<IsDish>()
              .whereHasComponent<DishBattleState>()
              .whereTeamSide(src_team_side)
              .whereLambda([src_queue_index](const afterhours::Entity &e) {
                const DishBattleState &dbs = e.get<DishBattleState>();
                return dbs.queue_index > src_queue_index;
              })
              .gen();
      for (afterhours::Entity &e : after) {
        targets.push_back(e);
      }
      break;
    }

    case TargetScope::FutureAllies: {
      auto future = EQ({.force_merge = true})
                        .whereHasComponent<IsDish>()
                        .whereHasComponent<DishBattleState>()
                        .whereTeamSide(src_team_side)
                        .whereLambda([](const afterhours::Entity &e) {
                          const DishBattleState &dbs = e.get<DishBattleState>();
                          return dbs.phase == DishBattleState::Phase::InQueue;
                        })
                        .gen();
      for (afterhours::Entity &e : future) {
        targets.push_back(e);
      }
      break;
    }

    case TargetScope::FutureOpponents: {
      auto future = EQ({.force_merge = true})
                        .whereHasComponent<IsDish>()
                        .whereHasComponent<DishBattleState>()
                        .whereTeamSide(opposite_side)
                        .whereLambda([](const afterhours::Entity &e) {
                          const DishBattleState &dbs = e.get<DishBattleState>();
                          return dbs.phase == DishBattleState::Phase::InQueue;
                        })
                        .gen();
      for (afterhours::Entity &e : future) {
        targets.push_back(e);
      }
      break;
    }

    case TargetScope::Previous: {
      auto prev = find_previous_dish_in_queue(src_dbs, src_queue_index);
      if (prev.has_value()) {
        targets.push_back(**prev);
      }
      break;
    }

    case TargetScope::Next: {
      auto next = find_next_dish_in_queue(src_dbs, src_queue_index);
      if (next.has_value()) {
        targets.push_back(**next);
      }
      break;
    }

    case TargetScope::SelfAndAdjacent: {
      targets.push_back(*src_opt.value());

      auto prev = find_previous_dish_in_queue(src_dbs, src_queue_index);
      if (prev.has_value()) {
        targets.push_back(**prev);
      }

      auto next = find_next_dish_in_queue(src_dbs, src_queue_index);
      if (next.has_value()) {
        targets.push_back(**next);
      }
      break;
    }
    }

    return targets;
  }

  void apply_to_target(afterhours::Entity &target, const DishEffect &effect) {
    switch (effect.operation) {
    case EffectOperation::AddFlavorStat: {
      auto &def = target.addComponentIfMissing<DeferredFlavorMods>();
      switch (effect.flavorStatType) {
      case FlavorStatType::Satiety:
        def.satiety += effect.amount;
        break;
      case FlavorStatType::Sweetness:
        def.sweetness += effect.amount;
        break;
      case FlavorStatType::Spice:
        def.spice += effect.amount;
        break;
      case FlavorStatType::Acidity:
        def.acidity += effect.amount;
        break;
      case FlavorStatType::Umami:
        def.umami += effect.amount;
        break;
      case FlavorStatType::Richness:
        def.richness += effect.amount;
        break;
      case FlavorStatType::Freshness:
        def.freshness += effect.amount;
        break;
      }
      log_info("EFFECT: Added {} to flavor stat {} for entity {}",
               effect.amount, static_cast<int>(effect.flavorStatType),
               target.id);
      break;
    }

    case EffectOperation::AddCombatZing: {
      // Always add PendingCombatMods - CombatStats will be created by
      // ComputeCombatStatsSystem if needed
      auto &pending = target.addComponentIfMissing<PendingCombatMods>();
      pending.zingDelta += effect.amount;
      log_info("EFFECT: Added {} Zing (combat) to entity {}", effect.amount,
               target.id);
      break;
    }

    case EffectOperation::AddCombatBody: {
      // Always add PendingCombatMods - CombatStats will be created by
      // ComputeCombatStatsSystem if needed
      auto &pending = target.addComponentIfMissing<PendingCombatMods>();
      pending.bodyDelta += effect.amount;
      log_info("EFFECT: Added {} Body (combat) to entity {}", effect.amount,
               target.id);
      break;
    }

    case EffectOperation::SwapStats: {
      if (!target.has<CombatStats>()) {
        log_error(
            "EFFECT: SwapStats requires CombatStats component on entity {}",
            target.id);
        break;
      }
      auto &stats = target.get<CombatStats>();
      std::swap(stats.baseZing, stats.baseBody);
      std::swap(stats.currentZing, stats.currentBody);
      log_info("EFFECT: Swapped Zing and Body stats for entity {}", target.id);
      break;
    }

    case EffectOperation::MultiplyDamage: {
      auto &next_effect = target.addComponentIfMissing<NextDamageEffect>();
      next_effect.multiplier =
          effect.amount > 0 ? static_cast<float>(effect.amount) : 2.0f;
      next_effect.flatModifier = 0;
      next_effect.count = 1;
      log_info("EFFECT: Added MultiplyDamage ({}x) to entity {}",
               next_effect.multiplier, target.id);
      break;
    }

    case EffectOperation::PreventAllDamage: {
      auto &next_effect = target.addComponentIfMissing<NextDamageEffect>();
      next_effect.multiplier = 0.0f;
      next_effect.flatModifier = 0;
      next_effect.count = effect.amount > 0 ? effect.amount : 1;
      log_info("EFFECT: Added PreventAllDamage ({} uses) to entity {}",
               next_effect.count, target.id);
      break;
    }
    }
  }
};
