#pragma once

#include "../components/combat_stats.h"
#include "../components/dish_level.h"
#include "../components/is_dish.h"
#include "../components/pre_battle_modifiers.h"
#include "../dish_types.h"
#include <afterhours/ah.h>

struct ComputeCombatStatsSystem : afterhours::System<IsDish, DishLevel> {
  virtual bool should_run(float) override {
    return true; // Always run to keep stats up to date
  }

  void for_each_with(afterhours::Entity &e, IsDish &dish, DishLevel &lvl,
                     float) override {
    // (quiet)

    // Add components if they don't exist
    CombatStats &cs = e.addComponentIfMissing<CombatStats>();
    PreBattleModifiers &pre = e.addComponentIfMissing<PreBattleModifiers>();

    // Get base flavor stats
    auto dish_info = get_dish_info(dish.type);
    const FlavorStats &flavor = dish_info.flavor;

    // Calculate Zing and Body using FlavorStats methods
    int zing = flavor.zing();
    int body = flavor.body();

    // Level scaling: multiply by 2 for each level above 1
    if (lvl.level > 1) {
      int level_multiplier = 1 << (lvl.level - 1); // 2^(level-1)
      zing *= level_multiplier;
      body *= level_multiplier;
    }

    // Apply pre-battle modifiers
    cs.baseZing = cs.currentZing = std::max(0, zing + pre.zingDelta);
    cs.baseBody = cs.currentBody = std::max(0, body + pre.bodyDelta);

    // (quiet)
  }
};
