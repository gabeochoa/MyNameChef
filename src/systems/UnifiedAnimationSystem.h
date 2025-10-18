#pragma once

#include "../components/animation_event.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

// Unified system to handle all animation scheduling and timing
struct UnifiedAnimationSystem
    : afterhours::System<AnimationEvent, IsBlockingAnimationEvent> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &e, AnimationEvent &ev,
                     IsBlockingAnimationEvent &, float) override {

    // Schedule each animation exactly once per event
    if (e.has<AnimationEventScheduled>()) {
      return; // already scheduled; wait for timer to complete
    }
    e.addComponent<AnimationEventScheduled>();

    // Schedule animation using consistent timer-based approach
    switch (ev.type) {
    case AnimationEventType::SlideIn: {
      log_info("ANIM schedule: SlideIn (event id={})", e.id);
      e.addComponent<AnimationTimer>();
      auto &timer = e.get<AnimationTimer>();
      timer.duration = 0.27f;
      timer.elapsed = 0.0f;
      break;
    }
    case AnimationEventType::StatBoost: {
      if (!e.has<AnimationData>()) {
        break;
      }
      log_info("ANIM schedule: StatBoost (event id={})", e.id);
      e.addComponent<AnimationTimer>();
      auto &timer = e.get<AnimationTimer>();
      timer.duration = 1.5f;
      timer.elapsed = 0.0f;
      break;
    }
    case AnimationEventType::FreshnessChain: {
      if (!e.has<AnimationData>()) {
        break;
      }
      log_info("ANIM schedule: FreshnessChain (event id={})", e.id);
      e.addComponent<AnimationTimer>();
      auto &timer = e.get<AnimationTimer>();
      timer.duration = 2.0f;
      timer.elapsed = 0.0f;
      break;
    }
    }

    // Only run one event per update; bail after scheduling
    return;
  }
};

// System to handle animation timer updates and completion
struct AnimationTimerSystem : afterhours::System<AnimationTimer> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &e, AnimationTimer &timer,
                     float dt) override {
    timer.elapsed += dt;

    if (timer.elapsed >= timer.duration) {
      log_info("ANIM complete: Timer-based animation (event id={})", e.id);

      // Clean up animation components
      e.removeComponent<AnimationEvent>();
      e.removeComponent<AnimationEventScheduled>();
      e.removeComponent<AnimationTimer>();
      e.removeComponent<IsBlockingAnimationEvent>();

      // Clean up animation-specific components
      if (e.has<AnimationData>()) {
        e.removeComponent<AnimationData>();
      }
    }
  }
};
