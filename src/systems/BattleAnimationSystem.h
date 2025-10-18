#pragma once

#include "../components/animation_event.h"
#include "../components/battle_anim_keys.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/animation.h>

// Runs at most one blocking animation event per frame window.
struct BattleAnimationSystem
    : afterhours::System<AnimationEvent, IsBlockingAnimationEvent> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &e, AnimationEvent &ev,
                     IsBlockingAnimationEvent &, float) override {
    log_info("ANIMATION: Processing event type={} entity={}",
             ev.type == AnimationEventType::SlideIn ? "SlideIn" : "Unknown",
             e.id);

    // Only process each event once - immediately remove the blocking component
    // so it doesn't get processed again next frame
    e.removeComponent<IsBlockingAnimationEvent>();

    // Play and remove the event; only handle one per tick to keep it serialized
    switch (ev.type) {
    case AnimationEventType::SlideIn: {
      log_info("ANIMATION: Starting SlideIn animation for entity={}", e.id);
      // Drive a global hold for the slide-in duration; no per-entity visuals
      // here
      afterhours::animation::anim(BattleAnimKey::SlideIn, /*index=*/0)
          .from(0.0f)
          .hold(0.27f)
          .on_complete([id = e.id]() {
            log_info("ANIMATION: SlideIn completed for entity={}, cleaning up",
                     id);
            if (auto ent = afterhours::EntityQuery().whereID(id).gen_first()) {
              ent->removeComponent<AnimationEvent>();
              log_info("ANIMATION: Cleaned up animation event entity={}", id);
            } else {
              log_warn("ANIMATION: Could not find entity={} for cleanup", id);
            }
          });
      break;
    }
    }

    // Only run one event per update; bail after scheduling
    return;
  }
};
