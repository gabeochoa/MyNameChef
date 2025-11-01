#pragma once

#include "../components/combat_stats.h"
#include "../components/deferred_flavor_mods.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_effect.h"
#include "../components/is_dish.h"
#include "../components/pending_combat_mods.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../query.h"
#include <afterhours/ah.h>
#include <vector>

struct EffectResolutionSystem : afterhours::System<TriggerQueue> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &, TriggerQueue &queue,
                     float) override {
    if (queue.empty()) {
      return;
    }

    for (const auto &ev : queue.events) {
      process_trigger_event(ev);
    }

    queue.clear();
  }

private:
  void process_trigger_event(const TriggerEvent &ev) {
    auto src_opt = EQ().whereID(ev.sourceEntityId).gen_first();
    if (!src_opt || !src_opt->has<IsDish>()) {
      return;
    }

    const auto &dish = src_opt->get<IsDish>();
    const auto &info = get_dish_info(dish.type);

    for (const auto &effect : info.effects) {
      if (effect.triggerHook == ev.hook) {
        apply_effect(effect, ev);
      }
    }
  }

  void apply_effect(const DishEffect &effect, const TriggerEvent &ev) {
    auto targets = get_targets(effect.targetScope, ev);

    for (afterhours::Entity *target : targets) {
      if (target) {
        apply_to_target(*target, effect);
      }
    }
  }

  std::vector<afterhours::Entity *> get_targets(TargetScope scope,
                                                const TriggerEvent &ev) {
    std::vector<afterhours::Entity *> targets;

    auto src_opt = EQ().whereID(ev.sourceEntityId).gen_first();
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
        targets.push_back(src_opt.value());
      }
      break;
    }

    case TargetScope::Opponent: {
      auto opponent = EQ().whereHasComponent<IsDish>()
                          .whereHasComponent<DishBattleState>()
                          .whereTeamSide(opposite_side)
                          .whereInSlotIndex(src_queue_index)
                          .gen_first();
      if (opponent.has_value()) {
        targets.push_back(opponent.value());
      }
      break;
    }

    case TargetScope::AllAllies: {
      auto allies = EQ().whereHasComponent<IsDish>()
                        .whereHasComponent<DishBattleState>()
                        .whereTeamSide(src_team_side)
                        .whereNotID(ev.sourceEntityId)
                        .gen();
      for (afterhours::Entity &e : allies) {
        targets.push_back(&e);
      }
      break;
    }

    case TargetScope::AllOpponents: {
      auto opponents = EQ().whereHasComponent<IsDish>()
                           .whereHasComponent<DishBattleState>()
                           .whereTeamSide(opposite_side)
                           .gen();
      for (afterhours::Entity &e : opponents) {
        targets.push_back(&e);
      }
      break;
    }

    case TargetScope::DishesAfterSelf: {
      auto after =
          EQ().whereHasComponent<IsDish>()
              .whereHasComponent<DishBattleState>()
              .whereTeamSide(src_team_side)
              .whereLambda([src_queue_index](const afterhours::Entity &e) {
                const DishBattleState &dbs = e.get<DishBattleState>();
                return dbs.queue_index > src_queue_index;
              })
              .gen();
      for (afterhours::Entity &e : after) {
        targets.push_back(&e);
      }
      break;
    }

    case TargetScope::FutureAllies: {
      auto future = EQ().whereHasComponent<IsDish>()
                        .whereHasComponent<DishBattleState>()
                        .whereTeamSide(src_team_side)
                        .whereLambda([](const afterhours::Entity &e) {
                          const DishBattleState &dbs = e.get<DishBattleState>();
                          return dbs.phase == DishBattleState::Phase::InQueue;
                        })
                        .gen();
      for (afterhours::Entity &e : future) {
        targets.push_back(&e);
      }
      break;
    }

    case TargetScope::FutureOpponents: {
      auto future = EQ().whereHasComponent<IsDish>()
                        .whereHasComponent<DishBattleState>()
                        .whereTeamSide(opposite_side)
                        .whereLambda([](const afterhours::Entity &e) {
                          const DishBattleState &dbs = e.get<DishBattleState>();
                          return dbs.phase == DishBattleState::Phase::InQueue;
                        })
                        .gen();
      for (afterhours::Entity &e : future) {
        targets.push_back(&e);
      }
      break;
    }

    case TargetScope::Previous: {
      auto prev =
          EQ().whereHasComponent<IsDish>()
              .whereHasComponent<DishBattleState>()
              .whereTeamSide(src_team_side)
              .whereLambda([src_queue_index](const afterhours::Entity &e) {
                const DishBattleState &dbs = e.get<DishBattleState>();
                return dbs.queue_index == src_queue_index - 1;
              })
              .gen_first();
      if (prev.has_value()) {
        targets.push_back(prev.value());
      }
      break;
    }

    case TargetScope::Next: {
      auto next =
          EQ().whereHasComponent<IsDish>()
              .whereHasComponent<DishBattleState>()
              .whereTeamSide(src_team_side)
              .whereLambda([src_queue_index](const afterhours::Entity &e) {
                const DishBattleState &dbs = e.get<DishBattleState>();
                return dbs.queue_index == src_queue_index + 1;
              })
              .gen_first();
      if (next.has_value()) {
        targets.push_back(next.value());
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
    }
  }
};
