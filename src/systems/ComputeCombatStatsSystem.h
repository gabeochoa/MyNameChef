#pragma once

#include "../components/combat_stats.h"
#include "../components/deferred_flavor_mods.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_level.h"
#include "../components/is_dish.h"
#include "../components/pairing_clash_modifiers.h"
#include "../components/persistent_combat_modifiers.h"
#include "../components/pre_battle_modifiers.h"
#include "../components/status_effects.h"
#include "../dish_types.h"
#include <afterhours/ah.h>
#include <algorithm>
#include <climits>
#include <map>
#include <unordered_set>

struct ComputeCombatStatsSystem : afterhours::System<IsDish, DishLevel> {
  virtual bool should_run(float) override {
    return true; // Always run to keep stats up to date
  }

  void for_each_with(afterhours::Entity &e, IsDish &dish, DishLevel &lvl,
                     float) override {

    // Add components if they don't exist
    CombatStats &cs = e.addComponentIfMissing<CombatStats>();

    // PreBattleModifiers is now a derived mirror - we should NOT read from it
    // as it would create a feedback loop. Check if it exists and has wrong
    // values.
    if (e.has<PreBattleModifiers>()) {
      const auto &preCheck = e.get<PreBattleModifiers>();
      if (preCheck.bodyDelta != 0 || preCheck.zingDelta != 0) {
        // This might be stale data from before the refactor - we'll overwrite
        // it Log only first time per entity to avoid spam
        static std::unordered_set<int> warned;
        if (warned.find(e.id) == warned.end()) {
          warned.insert(e.id);
          log_info("COMBAT_STATS: Entity {} - PreBattleModifiers has non-zero "
                   "values (z={}, b={}), "
                   "will be overwritten with calculated sum",
                   e.id, preCheck.zingDelta, preCheck.bodyDelta);
        }
      }
    }

    // Get base flavor stats
    auto dish_info = get_dish_info(dish.type);
    FlavorStats flavor = dish_info.flavor;

    // Apply or convert deferred flavor modifications if present
    if (e.has<DeferredFlavorMods>()) {
      const auto &def = e.get<DeferredFlavorMods>();

      // Compute with and without deferred to derive combat deltas
      FlavorStats flavorWithDef = flavor;
      flavorWithDef.applyMod(def);

      int zingNoDef = flavor.zing();
      int bodyNoDef = flavor.body();
      int zingWithDef = flavorWithDef.zing();
      int bodyWithDef = flavorWithDef.body();

      bool in_enter_or_combat = false;
      if (e.has<DishBattleState>()) {
        const auto &dbsPhase = e.get<DishBattleState>();
        in_enter_or_combat =
            (dbsPhase.phase == DishBattleState::Phase::Entering ||
             dbsPhase.phase == DishBattleState::Phase::InCombat);
      }

      if (in_enter_or_combat) {
        // Issue 52: persist at the same level scaling the preview used.
        int mult = 1; for (int i = 1; i < lvl.level; ++i) mult *= 2;
        int persistZDelta = (zingWithDef - zingNoDef) * mult;
        int persistBDelta = (bodyWithDef - bodyNoDef) * mult;
        if (persistZDelta != 0 || persistBDelta != 0) {
          auto &persist = e.addComponentIfMissing<PersistentCombatModifiers>();
          persist.zingDelta += persistZDelta;
          persist.bodyDelta += persistBDelta;
        }
        e.removeComponent<DeferredFlavorMods>();
        // Keep flavor as base (without deferred); persistent modifiers will
        // carry the effect
      } else {
        // Not yet entering combat: preview with deferred applied
        flavor = flavorWithDef;
      }
    }

    // Calculate Zing and Body using FlavorStats methods
    int zing = flavor.zing();
    int body = flavor.body();

    // Level scaling (issue 16: capped, checked - no overflow loop)
    { int capped = std::clamp(lvl.level, 1, MAX_DISH_LEVEL); long long m = 1LL << (capped - 1); zing = static_cast<int>(std::min<long long>(zing * m, INT_MAX / 2)); body = static_cast<int>(std::min<long long>(body * m, INT_MAX / 2)); }

    // Apply pre-battle modifiers
    // Check if dish is entering combat (was not in combat before, now is)
    bool is_finished = false;
    bool in_combat = false;
    if (e.has<DishBattleState>()) {
      const auto &dbs = e.get<DishBattleState>();
      is_finished = dbs.phase == DishBattleState::Phase::Finished;
      in_combat = dbs.phase == DishBattleState::Phase::InCombat;
    }

    // Track if we just entered combat this frame by tracking previous phase
    static std::map<int, DishBattleState::Phase> previous_phase;
    if (is_finished) { previous_phase.erase(e.id); return; } // issue 53 purge
    bool just_entered_combat = false;
    if (e.has<DishBattleState>()) {
      const auto &dbs = e.get<DishBattleState>();
      if (in_combat) {
        // Check if previous phase was NOT InCombat
        auto it = previous_phase.find(e.id);
        if (it == previous_phase.end() ||
            it->second != DishBattleState::Phase::InCombat) {
          just_entered_combat = true;
        }
        previous_phase[e.id] = dbs.phase;
      } else {
        previous_phase[e.id] = dbs.phase;
      }
    }

    int oldBaseZing = cs.baseZing;
    int oldBaseBody = cs.baseBody;

    int pairingZ = 0, pairingB = 0;
    if (e.has<PairingClashModifiers>()) {
      const auto &pcm = e.get<PairingClashModifiers>();
      pairingZ = pcm.zingDelta;
      pairingB = pcm.bodyDelta;
    }
    int persistZ = 0, persistB = 0;
    if (e.has<PersistentCombatModifiers>()) {
      const auto &pm = e.get<PersistentCombatModifiers>();
      persistZ = pm.zingDelta;
      persistB = pm.bodyDelta;
    }
    int statusZ = 0, statusB = 0;
    if (e.has<StatusEffects>()) {
      const auto &se = e.get<StatusEffects>();
      for (const auto &status : se.effects) {
        statusZ += status.zingDelta;
        statusB += status.bodyDelta;
      }
    }
    // Calculate total modifiers from source components ONLY
    // Do NOT include PreBattleModifiers in the calculation (feedback loop
    // prevention)
    int totalZ = pairingZ + persistZ + statusZ;
    int totalB = pairingB + persistB + statusB;

    cs.baseZing = std::max(1, zing + totalZ);
    cs.baseBody = std::max(0, body + totalB);

    // Mirror totals into PreBattleModifiers for backward compatibility
    // This is now write-only - we never read from PreBattleModifiers
    PreBattleModifiers &pre = e.addComponentIfMissing<PreBattleModifiers>();
    pre.zingDelta = totalZ;
    pre.bodyDelta = totalB;

    // quiet summary

    // Sync currentBody to baseBody only when:
    // 1. NOT in combat (always sync for non-combat dishes)
    // 2. Just entered combat (initialize to baseBody)
    // 3. baseBody changed (modifiers updated, need to reflect the change)
    // We do NOT sync every frame when in combat because ResolveCombatTickSystem
    // applies damage by modifying currentBody, and we don't want to reset
    // damage
    bool baseChanged =
        (oldBaseZing != cs.baseZing || oldBaseBody != cs.baseBody);

    if (!in_combat) {
      cs.currentZing = cs.baseZing;
      cs.currentBody = cs.baseBody;
    } else {
      if (just_entered_combat) {
        cs.currentZing = cs.baseZing; cs.currentBody = cs.baseBody;
      } else if (baseChanged) {
        // Issue 51: apply only the delta - a modifier must not heal damage.
        cs.currentZing += cs.baseZing - oldBaseZing;
        cs.currentBody += cs.baseBody - oldBaseBody;
        if (cs.currentBody < 0) cs.currentBody = 0;
      }
      // else: in combat, unchanged - preserve damage (issue 51/53, no statics)
    }

    // (quiet)
  }
};
