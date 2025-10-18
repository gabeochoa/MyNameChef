#pragma once

#include "../components/animation_event.h"
#include "../components/battle_anim_keys.h"
#include "../game_state_manager.h"
#include "../query.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/animation.h>
#include <magic_enum/magic_enum.hpp>

// Runs at most one blocking animation event per frame window.
struct BattleAnimationSystem
    : afterhours::System<AnimationEvent, IsBlockingAnimationEvent> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
    // Note: This system should NOT check hasActiveAnimation() because it needs
    // to run to advance the animation timeline and complete the blocking events
  }

  void for_each_with(afterhours::Entity &e, AnimationEvent &ev,
                     IsBlockingAnimationEvent &, float) override {

    // Schedule each animation exactly once per event
    if (e.has<AnimationEventScheduled>()) {
      return; // already scheduled; wait for on_complete to clean up
    }
    e.addComponent<AnimationEventScheduled>();

    // Play and remove the event; only handle one per tick to keep it serialized
    switch (ev.type) {
    case AnimationEventType::SlideIn: {
      // Use a simple timer instead of the complex animation system
      log_info("ANIM schedule: SlideIn (event id={})", e.id);
      e.addComponent<AnimationTimer>();
      auto &timer = e.get<AnimationTimer>();
      timer.duration = 0.27f;
      timer.elapsed = 0.0f;
      break;
    }
    case AnimationEventType::StatBoost: {
      // Get the StatBoostAnimation component if it exists
      if (!e.has<StatBoostAnimation>()) {
        break;
      }
      // Drive a global hold for the stat boost overlay duration
      log_info("ANIM schedule: StatBoost (event id={})", e.id);
      afterhours::animation::anim(BattleAnimKey::StatBoost,
                                  /*index=*/(size_t)e.id)
          .from(0.0f)
          .to(1.0f, 1.5f, afterhours::animation::EasingType::Linear)
          .on_complete([id = e.id]() {
            if (auto ent = afterhours::EntityQuery().whereID(id).gen_first()) {
              log_info("ANIM complete: StatBoost (event id={})", id);
              ent->removeComponent<AnimationEvent>();
              ent->removeComponent<StatBoostAnimation>();
              ent->removeComponent<AnimationEventScheduled>();
              ent->removeComponent<IsBlockingAnimationEvent>();
            }
          });
      break;
    }
    case AnimationEventType::FreshnessChain: {
      // Get the FreshnessChainAnimation component if it exists
      if (!e.has<FreshnessChainAnimation>()) {
        break;
      }
      // Drive a global hold for the freshness chain animation duration
      log_info("ANIM schedule: FreshnessChain (event id={})", e.id);
      afterhours::animation::anim(BattleAnimKey::FreshnessChain,
                                  /*index=*/(size_t)e.id)
          .from(0.0f)
          .to(1.0f, 2.0f, afterhours::animation::EasingType::EaseOutQuad)
          .on_complete([id = e.id]() {
            if (auto ent = afterhours::EntityQuery().whereID(id).gen_first()) {
              log_info("ANIM complete: FreshnessChain (event id={})", id);
              ent->removeComponent<AnimationEvent>();
              ent->removeComponent<FreshnessChainAnimation>();
              ent->removeComponent<AnimationEventScheduled>();
              ent->removeComponent<IsBlockingAnimationEvent>();
            }
          });
      break;
    }
    }

    // Only run one event per update; bail after scheduling
    return;
  }
};
