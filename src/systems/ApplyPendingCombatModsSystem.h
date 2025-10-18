#pragma once

#include "../components/combat_stats.h"
#include "../components/pending_combat_mods.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

struct ApplyPendingCombatModsSystem : afterhours::System<PendingCombatMods> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &e, PendingCombatMods &pending,
                     float) override {
    // Only apply if the entity has CombatStats
    if (!e.has<CombatStats>()) {
      return;
    }

    // Only process if there are actual modifications to apply
    if (pending.zingDelta == 0 && pending.bodyDelta == 0) {
      return;
    }

    auto &cs = e.get<CombatStats>();

    // Store values before applying
    int zingDelta = pending.zingDelta;
    int bodyDelta = pending.bodyDelta;

    // Apply the pending modifications
    cs.currentZing += pending.zingDelta;
    cs.currentBody += pending.bodyDelta;

    // Clear the pending modifications (they've been applied)
    e.removeComponent<PendingCombatMods>();

    log_info("APPLIED PENDING MODS: entity={} zingDelta={} bodyDelta={} -> "
             "currentZing={} currentBody={}",
             e.id, zingDelta, bodyDelta, cs.currentZing, cs.currentBody);
  }
};
