#pragma once

#include "components/animation_event.h"
#include "components/dish_effect.h"
#include "dish_types.h"
#include <vector>

template <TriggerHook Hook> class TriggerEffectBuilder {
protected:
  DishEffect effect_;

public:
  TriggerEffectBuilder() { effect_.triggerHook = Hook; }

  class TargetEffectBuilder {
    TriggerEffectBuilder &parent_;

  public:
    TargetEffectBuilder(TriggerEffectBuilder &parent, TargetScope target)
        : parent_(parent) {
      parent_.effect_.targetScope = target;
    }

    TriggerEffectBuilder &add_zing(int amount) {
      parent_.effect_.operation = EffectOperation::AddCombatZing;
      parent_.effect_.amount = amount;
      return parent_;
    }

    TriggerEffectBuilder &remove_zing(int amount = 1) {
      return add_zing(-amount);
    }

    TriggerEffectBuilder &add_body(int amount) {
      parent_.effect_.operation = EffectOperation::AddCombatBody;
      parent_.effect_.amount = amount;
      return parent_;
    }

    TriggerEffectBuilder &remove_body(int amount = 1) {
      return add_body(-amount);
    }

    TriggerEffectBuilder &add_flavor_stat(FlavorStatType stat, int amount) {
      parent_.effect_.operation = EffectOperation::AddFlavorStat;
      parent_.effect_.flavorStatType = stat;
      parent_.effect_.amount = amount;
      return parent_;
    }

    TriggerEffectBuilder &remove_flavor_stat(FlavorStatType stat,
                                             int amount = 1) {
      return add_flavor_stat(stat, -amount);
    }

    TriggerEffectBuilder &if_adjacent_has(FlavorStatType stat) {
      parent_.effect_.conditional = true;
      parent_.effect_.adjacentCheckStat = stat;
      return parent_;
    }

    TriggerEffectBuilder &with_animation(AnimationEventType animType) {
      if (animType == AnimationEventType::FreshnessChain) {
        parent_.effect_.playFreshnessChainAnimation = true;
      }
      return parent_;
    }

    TriggerEffectBuilder &copy_effect() {
      parent_.effect_.operation = EffectOperation::CopyEffect;
      parent_.effect_.amount = 0;
      return parent_;
    }

    TriggerEffectBuilder &summon_dish(DishType dish_type) {
      parent_.effect_.operation = EffectOperation::SummonDish;
      parent_.effect_.summonDishType = dish_type;
      return parent_;
    }

    TriggerEffectBuilder &apply_status(int zingDelta, int bodyDelta = 0) {
      parent_.effect_.operation = EffectOperation::ApplyStatus;
      parent_.effect_.amount = zingDelta;
      parent_.effect_.statusBodyDelta = bodyDelta;
      return parent_;
    }

    TriggerEffectBuilder &swap_stats() {
      parent_.effect_.operation = EffectOperation::SwapStats;
      parent_.effect_.amount = 0;
      return parent_;
    }

    TriggerEffectBuilder &multiply_damage(float multiplier) {
      parent_.effect_.operation = EffectOperation::MultiplyDamage;
      parent_.effect_.amount = static_cast<int>(multiplier);
      return parent_;
    }

    TriggerEffectBuilder &prevent_all_damage(int count = 1) {
      parent_.effect_.operation = EffectOperation::PreventAllDamage;
      parent_.effect_.amount = count;
      return parent_;
    }
  };

  TargetEffectBuilder with_target(TargetScope target) {
    return TargetEffectBuilder(*this, target);
  }

  TriggerEffectBuilder &if_adjacent_has(FlavorStatType stat) {
    effect_.conditional = true;
    effect_.adjacentCheckStat = stat;
    return *this;
  }

  TriggerEffectBuilder &with_animation(AnimationEventType animType) {
    if (animType == AnimationEventType::FreshnessChain) {
      effect_.playFreshnessChainAnimation = true;
    }
    return *this;
  }

  TriggerEffectBuilder &add_zing(int amount) {
    effect_.operation = EffectOperation::AddCombatZing;
    effect_.amount = amount;
    return *this;
  }

  TriggerEffectBuilder &remove_zing(int amount = 1) {
    return add_zing(-amount);
  }

  TriggerEffectBuilder &add_body(int amount) {
    effect_.operation = EffectOperation::AddCombatBody;
    effect_.amount = amount;
    return *this;
  }

  TriggerEffectBuilder &remove_body(int amount = 1) {
    return add_body(-amount);
  }

  TriggerEffectBuilder &add_flavor_stat(FlavorStatType stat, int amount) {
    effect_.operation = EffectOperation::AddFlavorStat;
    effect_.flavorStatType = stat;
    effect_.amount = amount;
    return *this;
  }

  TriggerEffectBuilder &remove_flavor_stat(FlavorStatType stat,
                                           int amount = 1) {
    return add_flavor_stat(stat, -amount);
  }

  TriggerEffectBuilder &copy_effect() {
    effect_.operation = EffectOperation::CopyEffect;
    effect_.amount = 0;
    return *this;
  }

  TriggerEffectBuilder &summon_dish(DishType dish_type) {
    effect_.operation = EffectOperation::SummonDish;
    effect_.summonDishType = dish_type;
    return *this;
  }

  TriggerEffectBuilder &apply_status(int zingDelta, int bodyDelta = 0) {
    effect_.operation = EffectOperation::ApplyStatus;
    effect_.amount = zingDelta;
    effect_.statusBodyDelta = bodyDelta;
    return *this;
  }

  TriggerEffectBuilder &swap_stats() {
    effect_.operation = EffectOperation::SwapStats;
    effect_.amount = 0;
    return *this;
  }

  TriggerEffectBuilder &multiply_damage(float multiplier) {
    effect_.operation = EffectOperation::MultiplyDamage;
    effect_.amount = static_cast<int>(multiplier);
    return *this;
  }

  TriggerEffectBuilder &prevent_all_damage(int count = 1) {
    effect_.operation = EffectOperation::PreventAllDamage;
    effect_.amount = count;
    return *this;
  }

  DishEffect get_effect() && { return std::move(effect_); }
};

using ServeEffect = TriggerEffectBuilder<TriggerHook::OnServe>;
using OnBiteEffect = TriggerEffectBuilder<TriggerHook::OnBiteTaken>;
using OnDishFinishedEffect = TriggerEffectBuilder<TriggerHook::OnDishFinished>;
using OnCourseStartEffect = TriggerEffectBuilder<TriggerHook::OnCourseStart>;
using OnStartBattleEffect = TriggerEffectBuilder<TriggerHook::OnStartBattle>;
using OnCourseCompleteEffect =
    TriggerEffectBuilder<TriggerHook::OnCourseComplete>;

class DishBuilder {
  DishInfo dish_;
  std::vector<DishEffect> effects_;
  bool name_set_ = false;
  bool flavor_set_ = false;
  bool sprite_set_ = false;
  bool tier_set_ = false;

public:
  DishBuilder &with_name(const char *name) {
    dish_.name = name;
    name_set_ = true;
    return *this;
  }

  DishBuilder &with_flavor(FlavorStats stats) {
    dish_.flavor = stats;
    flavor_set_ = true;
    return *this;
  }

  DishBuilder &with_sprite(SpriteLocation loc) {
    dish_.sprite = loc;
    sprite_set_ = true;
    return *this;
  }

  DishBuilder &with_tier(int tier) {
    dish_.tier = tier;
    tier_set_ = true;
    return *this;
  }

  template <TriggerHook Hook>
  DishBuilder &register_trigger(TriggerEffectBuilder<Hook> &&builder) {
    effects_.push_back(std::move(builder).get_effect());
    return *this;
  }

  template <TriggerHook Hook>
  DishBuilder &register_trigger(TriggerEffectBuilder<Hook> &builder) {
    effects_.push_back(std::move(builder).get_effect());
    return *this;
  }

  DishBuilder &register_on_serve(ServeEffect &&builder) {
    return register_trigger(std::move(builder));
  }

  DishBuilder &register_on_serve(ServeEffect &builder) {
    return register_trigger(builder);
  }

  DishBuilder &register_on_bite_taken(OnBiteEffect &&builder) {
    return register_trigger(std::move(builder));
  }

  DishBuilder &register_on_bite_taken(OnBiteEffect &builder) {
    return register_trigger(builder);
  }

  DishBuilder &register_on_dish_finished(OnDishFinishedEffect &&builder) {
    return register_trigger(std::move(builder));
  }

  DishBuilder &register_on_dish_finished(OnDishFinishedEffect &builder) {
    return register_trigger(builder);
  }

  DishBuilder &register_on_course_start(OnCourseStartEffect &&builder) {
    return register_trigger(std::move(builder));
  }

  DishBuilder &register_on_course_start(OnCourseStartEffect &builder) {
    return register_trigger(builder);
  }

  DishBuilder &register_on_start_battle(OnStartBattleEffect &&builder) {
    return register_trigger(std::move(builder));
  }

  DishBuilder &register_on_start_battle(OnStartBattleEffect &builder) {
    return register_trigger(builder);
  }

  DishBuilder &register_on_course_complete(OnCourseCompleteEffect &&builder) {
    return register_trigger(std::move(builder));
  }

  DishBuilder &register_on_course_complete(OnCourseCompleteEffect &builder) {
    return register_trigger(builder);
  }

  DishInfo build();

  static constexpr int kUnifiedDishPrice = 3;
};

static inline DishBuilder dish() { return DishBuilder(); }
