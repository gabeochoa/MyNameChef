#pragma once

#include "../components/battle_team_tags.h"
#include "../components/combat_stats.h"
#include "../components/deferred_flavor_mods.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_effect.h"
#include "../components/dish_level.h"
#include "../components/drink_effects.h"
#include "../components/has_tooltip.h"
#include "../components/is_dish.h"
#include "../components/next_damage_effect.h"
#include "../components/pending_combat_mods.h"
#include "../components/render_order.h"
#include "../components/status_effects.h"
#include "../components/synergy_bonus_effects.h"
#include "../components/transform.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../render_backend.h"
#include "../render_constants.h"
#include "../seeded_rng.h"
#include "../shop.h"
#include "../tooltip.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/texture_manager.h>
#include <magic_enum/magic_enum.hpp>
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

    // For CopyEffect, handle specially - it needs the source entity, not
    // targets
    if (effect.operation == EffectOperation::CopyEffect) {
      auto src_opt = EQ({.ignore_temp_warning = true})
                         .whereID(ev.sourceEntityId)
                         .gen_first();
      if (src_opt && src_opt->has<IsDish>()) {
        apply_to_target(*src_opt.value(), effect, ev);
      }
      return;
    }

    auto targets = get_targets(effect.targetScope, ev);

    for (afterhours::Entity &target : targets) {
      apply_to_target(target, effect, ev);
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

    case TargetScope::RandomAlly: {
      auto allies = EQ({.force_merge = true})
                        .whereHasComponent<IsDish>()
                        .whereHasComponent<DishBattleState>()
                        .whereTeamSide(src_team_side)
                        .whereNotID(ev.sourceEntityId)
                        .gen();
      std::vector<afterhours::Entity *> ally_vec;
      for (afterhours::Entity &e : allies) {
        ally_vec.push_back(&e);
      }
      if (!ally_vec.empty()) {
        size_t random_index = SeededRng::get().gen_index(ally_vec.size());
        targets.push_back(*ally_vec[random_index]);
      }
      break;
    }

    case TargetScope::RandomOpponent: {
      auto opponents = EQ({.force_merge = true})
                           .whereHasComponent<IsDish>()
                           .whereHasComponent<DishBattleState>()
                           .whereTeamSide(opposite_side)
                           .gen();
      // TODO no pointers please
      std::vector<afterhours::Entity *> opponent_vec;
      for (afterhours::Entity &e : opponents) {
        opponent_vec.push_back(&e);
      }
      if (!opponent_vec.empty()) {
        size_t random_index = SeededRng::get().gen_index(opponent_vec.size());
        targets.push_back(*opponent_vec[random_index]);
      }
      break;
    }

    case TargetScope::RandomDish: {
      auto all_dishes = EQ({.force_merge = true})
                            .whereHasComponent<IsDish>()
                            .whereHasComponent<DishBattleState>()
                            .whereNotID(ev.sourceEntityId)
                            .gen();
      std::vector<afterhours::Entity *> dish_vec;
      for (afterhours::Entity &e : all_dishes) {
        dish_vec.push_back(&e);
      }
      if (!dish_vec.empty()) {
        size_t random_index = SeededRng::get().gen_index(dish_vec.size());
        targets.push_back(*dish_vec[random_index]);
      }
      break;
    }

    case TargetScope::RandomOtherAlly: {
      // Same as RandomAlly, explicit version
      auto allies = EQ({.force_merge = true})
                        .whereHasComponent<IsDish>()
                        .whereHasComponent<DishBattleState>()
                        .whereTeamSide(src_team_side)
                        .whereNotID(ev.sourceEntityId)
                        .gen();
      std::vector<afterhours::Entity *> ally_vec;
      for (afterhours::Entity &e : allies) {
        ally_vec.push_back(&e);
      }
      if (!ally_vec.empty()) {
        size_t random_index = SeededRng::get().gen_index(ally_vec.size());
        targets.push_back(*ally_vec[random_index]);
      }
      break;
    }
    }

    return targets;
  }

  void apply_to_target(afterhours::Entity &target, const DishEffect &effect,
                       const TriggerEvent &ev) {
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

    case EffectOperation::CopyEffect: {
      // CopyEffect: target parameter is the source dish (the one copying)
      // The effect's targetScope determines which dish to copy from
      // Create a new event with source as the target for get_targets
      TriggerEvent copy_ev(effect.triggerHook, target.id, ev.slotIndex,
                           ev.teamSide);
      afterhours::RefEntities copy_targets =
          get_targets(effect.targetScope, copy_ev);

      // Validate that exactly one target exists
      if (copy_targets.size() != 1) {
        log_error("EFFECT: CopyEffect requires exactly one target, got {}",
                  copy_targets.size());
        break;
      }

      afterhours::Entity &copy_from = copy_targets[0];

      // Collect all effects from the target dish
      std::vector<DishEffect> copied_effects;

      // 1. Dish effects from get_dish_info().effects
      if (copy_from.has<IsDish>()) {
        const auto &copy_dish = copy_from.get<IsDish>();
        int copy_level = 1;
        if (copy_from.has<DishLevel>()) {
          copy_level = copy_from.get<DishLevel>().level;
        }
        const auto &copy_info = get_dish_info(copy_dish.type, copy_level);
        for (const auto &dish_effect : copy_info.effects) {
          DishEffect copied = dish_effect;
          copied.is_copied = true;
          copied_effects.push_back(copied);
        }
      }

      // 2. Drink effects from DrinkEffects component
      if (copy_from.has<DrinkEffects>()) {
        const auto &drink_effects = copy_from.get<DrinkEffects>();
        for (const auto &drink_effect : drink_effects.effects) {
          DishEffect copied = drink_effect;
          copied.is_copied = true;
          copied_effects.push_back(copied);
        }
      }

      // 3. Synergy effects from SynergyBonusEffects component
      if (copy_from.has<SynergyBonusEffects>()) {
        const auto &synergy_effects = copy_from.get<SynergyBonusEffects>();
        for (const auto &synergy_effect : synergy_effects.effects) {
          DishEffect copied = synergy_effect;
          copied.is_copied = true;
          copied_effects.push_back(copied);
        }
      }

      // Store copied effects on source dish (target parameter)
      auto &source_drink_effects = target.addComponentIfMissing<DrinkEffects>();
      for (const auto &copied : copied_effects) {
        source_drink_effects.effects.push_back(copied);
      }

      log_info("EFFECT: Copied {} effects from entity {} to entity {}",
               copied_effects.size(), copy_from.id, target.id);
      break;
    }

    case EffectOperation::SummonDish: {
      // SummonDish: Create a new dish entity in the same slot as the source
      if (!effect.summonDishType.has_value()) {
        log_error("EFFECT: SummonDish requires summonDishType to be set");
        break;
      }

      DishType dish_to_summon = effect.summonDishType.value();

      // Get source dish's slot and team info
      if (!target.has<DishBattleState>()) {
        log_error(
            "EFFECT: SummonDish requires DishBattleState on source entity");
        break;
      }

      const auto &source_dbs = target.get<DishBattleState>();
      int slot = source_dbs.queue_index;
      bool isPlayer =
          (source_dbs.team_side == DishBattleState::TeamSide::Player);

      // Create new dish entity
      auto &summoned_entity = afterhours::EntityHelper::createEntity();

      // Calculate position (same as InstantiateBattleTeamSystem)
      float x, y;
      if (isPlayer) {
        x = 120.0f + slot * 100.0f;
        y = 150.0f;
      } else {
        x = 120.0f + slot * 100.0f;
        y = 500.0f;
      }

      summoned_entity.addComponent<Transform>(afterhours::vec2{x, y},
                                              afterhours::vec2{80.0f, 80.0f});
      summoned_entity.addComponent<IsDish>(dish_to_summon);
      add_dish_tags(summoned_entity, dish_to_summon);
      summoned_entity.addComponent<DishLevel>(1); // Summoned dishes are level 1

      auto &summoned_dbs = summoned_entity.addComponent<DishBattleState>();
      summoned_dbs.queue_index = slot;
      summoned_dbs.team_side = source_dbs.team_side;
      // If source is in combat, summoned dish enters combat; otherwise InQueue
      if (source_dbs.phase == DishBattleState::Phase::InCombat ||
          source_dbs.phase == DishBattleState::Phase::Entering) {
        summoned_dbs.phase = DishBattleState::Phase::InCombat;
      } else {
        summoned_dbs.phase = DishBattleState::Phase::InQueue;
      }
      summoned_dbs.enter_progress = 1.0f; // Already entered
      summoned_dbs.bite_timer = 0.0f;

      summoned_entity.addComponent<HasRenderOrder>(RenderOrder::BattleTeams,
                                                   RenderScreen::Battle |
                                                       RenderScreen::Results);

      // TODO id prefer to not have this and then just ignore it later on
      if (!render_backend::is_headless_mode) {
        auto dish_info = get_dish_info(dish_to_summon);
        const auto frame = afterhours::texture_manager::idx_to_sprite_frame(
            dish_info.sprite.i, dish_info.sprite.j);
        summoned_entity.addComponent<afterhours::texture_manager::HasSprite>(
            afterhours::vec2{x, y}, afterhours::vec2{80.0f, 80.0f}, 0.f, frame,
            render_constants::kDishSpriteScale,
            raylib::Color{255, 255, 255, 255});
      }

      if (isPlayer) {
        summoned_entity.addComponent<IsPlayerTeamItem>();
      } else {
        summoned_entity.addComponent<IsOpponentTeamItem>();
      }

      summoned_entity.addComponent<HasTooltip>(
          generate_dish_tooltip(dish_to_summon));

      log_info("EFFECT: Summoned dish {} (entity {}) in slot {} for team {}",
               magic_enum::enum_name(dish_to_summon), summoned_entity.id, slot,
               isPlayer ? "Player" : "Opponent");
      break;
    }

    case EffectOperation::ApplyStatus: {
      // ApplyStatus: Add a status effect (debuff/buff) to the target
      // amount field encodes zingDelta
      // TODO: Support bodyDelta via separate parameter or extend DishEffect
      // struct
      auto &status_effects = target.addComponentIfMissing<StatusEffects>();
      StatusEffect status;
      status.zingDelta = effect.amount; // Use amount for zingDelta
      status.bodyDelta = 0;             // TODO: Add support for bodyDelta
      status.duration = 0;              // 0 = permanent for now
      status_effects.effects.push_back(status);

      log_info("EFFECT: Applied status effect to entity {} - zingDelta: {}, "
               "bodyDelta: {}",
               target.id, status.zingDelta, status.bodyDelta);
      break;
    }
    }
  }
};
