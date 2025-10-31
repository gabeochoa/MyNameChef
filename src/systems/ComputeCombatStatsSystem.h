#pragma once

#include "../components/combat_stats.h"
#include "../components/deferred_flavor_mods.h"
#include "../components/dish_battle_state.h"
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
    FlavorStats flavor = dish_info.flavor;

    // Apply deferred flavor modifications if present
    if (e.has<DeferredFlavorMods>()) {
      const auto &def = e.get<DeferredFlavorMods>();
      flavor.satiety += def.satiety;
      flavor.sweetness += def.sweetness;
      flavor.spice += def.spice;
      flavor.acidity += def.acidity;
      flavor.umami += def.umami;
      flavor.richness += def.richness;
      flavor.freshness += def.freshness;

      // Clear consumed modifiers (only when entering combat)
      if (e.has<DishBattleState>()) {
        const auto &dbs = e.get<DishBattleState>();
        if (dbs.phase == DishBattleState::Phase::Entering ||
            dbs.phase == DishBattleState::Phase::InCombat) {
          e.removeComponent<DeferredFlavorMods>();
        }
      }
    }

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
    // Do not overwrite current values if already in combat
    bool in_combat =
        e.has<DishBattleState>() &&
        e.get<DishBattleState>().phase == DishBattleState::Phase::InCombat;
    cs.baseZing = std::max(1, zing + pre.zingDelta);
    cs.baseBody = std::max(0, body + pre.bodyDelta);
    if (!in_combat) {
      cs.currentZing = cs.baseZing;
      cs.currentBody = cs.baseBody;
    }

    // (quiet)
  }
};
