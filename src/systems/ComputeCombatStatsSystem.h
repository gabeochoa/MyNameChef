#pragma once

#include "../components/combat_stats.h"
#include "../components/deferred_flavor_mods.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_level.h"
#include "../components/is_dish.h"
#include "../components/pairing_clash_modifiers.h"
#include "../components/persistent_combat_modifiers.h"
#include "../components/pre_battle_modifiers.h"
#include "../dish_types.h"
#include <afterhours/ah.h>
#include <map>
#include <unordered_set>

struct ComputeCombatStatsSystem : afterhours::System<IsDish, DishLevel> {
  virtual bool should_run(float) override {
    return true; // Always run to keep stats up to date
  }

  void for_each_with(afterhours::Entity &e, IsDish &dish, DishLevel &lvl,
                     float) override {
    // (quiet)

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
        // Persist the net stat effects into PersistentCombatModifiers and clear
        // deferred
        int persistZDelta = zingWithDef - zingNoDef;
        int persistBDelta = bodyWithDef - bodyNoDef;
        if (persistZDelta != 0 || persistBDelta != 0) {
          auto &persist = e.addComponentIfMissing<PersistentCombatModifiers>();
          int oldZ = persist.zingDelta;
          int oldB = persist.bodyDelta;
          persist.zingDelta += persistZDelta;
          persist.bodyDelta += persistBDelta;
          log_info("DEF_TO_PERSIST: Entity {} converted DeferredFlavorMods to "
                   "Persistent - add (z={}, b={}) -> persist (z: {} -> {}, b: "
                   "{} -> {})",
                   e.id, persistZDelta, persistBDelta, oldZ, persist.zingDelta,
                   oldB, persist.bodyDelta);
        }
        log_info("DEF_CONSUME: Entity {} removing DeferredFlavorMods in phase "
                 "{} (satiety={}, sweetness={}, spice={}, acidity={}, "
                 "umami={}, richness={}, freshness={})",
                 e.id,
                 e.get<DishBattleState>().phase ==
                         DishBattleState::Phase::Entering
                     ? "Entering"
                     : "InCombat",
                 def.satiety, def.sweetness, def.spice, def.acidity, def.umami,
                 def.richness, def.freshness);
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
    int body_before_level = body;

    // Level scaling: multiply by 2 for each level above 1
    if (lvl.level > 1) {
      int level_multiplier = 1 << (lvl.level - 1); // 2^(level-1)
      zing *= level_multiplier;
      body *= level_multiplier;
      log_info("COMBAT_STATS: Entity {} - level={}, multiplier={}, body before "
               "level: {}, after level: {}",
               e.id, lvl.level, level_multiplier, body_before_level, body);
    }

    // Apply pre-battle modifiers
    // Check if dish is entering combat (was not in combat before, now is)
    bool in_combat =
        e.has<DishBattleState>() &&
        e.get<DishBattleState>().phase == DishBattleState::Phase::InCombat;

    // Track if we just entered combat this frame by tracking previous phase
    // This is more reliable than a static map which can get stale
    static std::map<int, DishBattleState::Phase> previous_phase;
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
    // Calculate total modifiers from source components ONLY
    // Do NOT include PreBattleModifiers in the calculation (feedback loop
    // prevention)
    int totalZ = pairingZ + persistZ;
    int totalB = pairingB + persistB;

    cs.baseZing = std::max(1, zing + totalZ);
    cs.baseBody = std::max(0, body + totalB);

    // Mirror totals into PreBattleModifiers for backward compatibility
    // This is now write-only - we never read from PreBattleModifiers
    PreBattleModifiers &pre = e.addComponentIfMissing<PreBattleModifiers>();
    pre.zingDelta = totalZ;
    pre.bodyDelta = totalB;

    log_info("COMBAT_STATS: Entity {} - level={}, flavor zing={} body={}, "
             "bodyAfterLevel={}, mods(z={},b={}) = pairing(z={},b={}) + "
             "persistent(z={},b={}), baseZing: {} -> {}, baseBody: {} -> {}",
             e.id, lvl.level, flavor.zing(), body_before_level, body, totalZ,
             totalB, pairingZ, pairingB, persistZ, persistB, oldBaseZing,
             cs.baseZing, oldBaseBody, cs.baseBody);

    // Sync currentBody to baseBody only when:
    // 1. NOT in combat (always sync for non-combat dishes)
    // 2. Just entered combat (initialize to baseBody)
    // 3. baseBody changed (modifiers updated, need to reflect the change)
    // We do NOT sync every frame when in combat because ResolveCombatTickSystem
    // applies damage by modifying currentBody, and we don't want to reset
    // damage
    int oldCurrentZing = cs.currentZing;
    int oldCurrentBody = cs.currentBody;
    bool baseChanged =
        (oldBaseZing != cs.baseZing || oldBaseBody != cs.baseBody);

    if (!in_combat) {
      // Not in combat: always sync to baseBody
      cs.currentZing = cs.baseZing;
      cs.currentBody = cs.baseBody;
      log_info("COMBAT_STATS: Entity {} NOT in combat - current stats synced: "
               "zing {} -> {}, body {} -> {}",
               e.id, oldCurrentZing, cs.currentZing, oldCurrentBody,
               cs.currentBody);
    } else {
      // In combat: only sync if just entered or baseBody changed
      if (just_entered_combat || baseChanged) {
        cs.currentZing = cs.baseZing;
        cs.currentBody = cs.baseBody;
        if (just_entered_combat) {
          log_info("COMBAT_STATS: Entity {} JUST ENTERED combat - syncing "
                   "current to base: zing {} -> {}, body {} -> {}",
                   e.id, oldCurrentZing, cs.currentZing, oldCurrentBody,
                   cs.currentBody);
        } else {
          log_info("COMBAT_STATS: Entity {} IN combat - baseBody changed, "
                   "syncing current to base: zing {} -> {}, body {} -> {}",
                   e.id, oldCurrentZing, cs.currentZing, oldCurrentBody,
                   cs.currentBody);
        }
      } else {
        // In combat and baseBody unchanged - don't sync (damage might have been
        // applied) Only log periodically to confirm we're not resetting damage
        static std::map<int, int> frame_count;
        frame_count[e.id] = (frame_count.find(e.id) == frame_count.end())
                                ? 0
                                : frame_count[e.id] + 1;
        if (frame_count[e.id] % 60 == 0) { // Log every 60 frames
          log_info("COMBAT_STATS: Entity {} IN combat - currentBody={}, "
                   "baseBody={} (preserving damage)",
                   e.id, cs.currentBody, cs.baseBody);
        }
      }
    }

    // (quiet)
  }
};
