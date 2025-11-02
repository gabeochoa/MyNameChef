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
