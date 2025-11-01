#pragma once

#include "../components/animation_event.h"
#include "../components/combat_stats.h"
#include "../components/pending_combat_mods.h"
#include "../components/persistent_combat_modifiers.h"
#include "../game_state_manager.h"
#include "../shop.h"
#include <afterhours/ah.h>

struct ApplyPendingCombatModsSystem : afterhours::System<PendingCombatMods> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle &&
           !hasActiveAnimation();
  }

  void for_each_with(afterhours::Entity &e, PendingCombatMods &pending,
                     float) override {

    // Only process if there are actual modifications to apply
    if (pending.zingDelta == 0 && pending.bodyDelta == 0) {
      return;
    }

    // Create animation event before applying mods
    auto &animEvent = make_animation_event(AnimationEventType::StatBoost, true);
    auto &animEventData = animEvent.get<AnimationEvent>();
    animEventData.data =
        StatBoostData{e.id, pending.zingDelta, pending.bodyDelta};

    // Persist effect-driven mods
    auto &persist = e.addComponentIfMissing<PersistentCombatModifiers>();
    int oldZingDelta = persist.zingDelta;
    int oldBodyDelta = persist.bodyDelta;
    persist.zingDelta += pending.zingDelta;
    persist.bodyDelta += pending.bodyDelta;

    log_info("COMBAT_MOD: Applying PERSISTENT modifiers to entity {} - "
             "zingDelta: {} -> {}, bodyDelta: {} -> {}",
             e.id, oldZingDelta, persist.zingDelta, oldBodyDelta,
             persist.bodyDelta);

    // NOTE: We do NOT modify currentZing/currentBody directly here.
    // Instead, we add modifiers to PersistentCombatModifiers, and let
    // ComputeCombatStatsSystem include them in base stats.

    // Clear the pending modifications (they've been applied)
    e.removeComponent<PendingCombatMods>();
  }
};
