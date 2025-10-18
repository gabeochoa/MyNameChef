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
  }

  void for_each_with(afterhours::Entity &e, AnimationEvent &ev,
                     IsBlockingAnimationEvent &, float) override {

    // Only process each event once - immediately remove the blocking component
    // so it doesn't get processed again next frame
    e.removeComponent<IsBlockingAnimationEvent>();

    // Play and remove the event; only handle one per tick to keep it serialized
    switch (ev.type) {
    case AnimationEventType::SlideIn: {
      // Drive a global hold for the slide-in duration; no per-entity visuals
      // here
      afterhours::animation::anim(BattleAnimKey::SlideIn, /*index=*/0)
          .from(0.0f)
          .hold(0.27f)
          .on_complete([id = e.id]() {
            if (auto ent = afterhours::EntityQuery().whereID(id).gen_first()) {
              ent->removeComponent<AnimationEvent>();
            }
          });
      break;
    }
    case AnimationEventType::StatBoost: {
      // Get the StatBoostAnimation component if it exists
      if (!e.has<StatBoostAnimation>()) {
        break;
      }
      // Drive a global hold for the stat boost overlay duration
      afterhours::animation::anim(BattleAnimKey::StatBoost,
                                  /*index=*/(size_t)e.id)
          .from(0.0f)
          .to(1.0f, 1.5f, afterhours::animation::EasingType::Linear)
          .on_complete([id = e.id]() {
            if (auto ent = afterhours::EntityQuery().whereID(id).gen_first()) {
              ent->removeComponent<AnimationEvent>();
              ent->removeComponent<StatBoostAnimation>();
            }
          });
      break;
    }
    }

    // Only run one event per update; bail after scheduling
    return;
  }
};
