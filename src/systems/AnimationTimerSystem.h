#pragma once

#include "../components/animation_event.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

// System to handle simple timer-based animations
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
      e.removeComponent<AnimationEvent>();
      e.removeComponent<AnimationEventScheduled>();
      e.removeComponent<AnimationTimer>();
      e.removeComponent<IsBlockingAnimationEvent>();
    }
  }
};
