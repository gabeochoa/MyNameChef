#pragma once

#include "../components/animation_event.h"
#include "../components/combat_stats.h"
#include "../components/pending_combat_mods.h"
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
    // Only apply if the entity has CombatStats
    if (!e.has<CombatStats>()) {
      return;
    }

    // Only process if there are actual modifications to apply
    if (pending.zingDelta == 0 && pending.bodyDelta == 0) {
      return;
    }

    // Create animation event before applying mods
    auto &animEvent = make_animation_event(AnimationEventType::StatBoost, true);
    auto &animEventData = animEvent.get<AnimationEvent>().data;
    animEventData.targetEntityId = e.id;
    animEventData.zingDelta = pending.zingDelta;
    animEventData.bodyDelta = pending.bodyDelta;

    auto &cs = e.get<CombatStats>();

    // Apply the pending modifications
    cs.currentZing += pending.zingDelta;
    cs.currentBody += pending.bodyDelta;

    // Clear the pending modifications (they've been applied)
    e.removeComponent<PendingCombatMods>();
  }
};
