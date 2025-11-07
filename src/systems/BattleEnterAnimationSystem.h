#pragma once

#include "../components/dish_battle_state.h"
#include "../components/pairing_clash_modifiers.h"
#include "../components/persistent_combat_modifiers.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../render_backend.h"
#include "../shop.h"
#include <afterhours/ah.h>

struct BattleEnterAnimationSystem : afterhours::System<DishBattleState> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Battle) {
      return false;
    }
    if (isReplayPaused()) {
      return false;
    }
    return true;
  }

  void for_each_with(afterhours::Entity &e, DishBattleState &dbs,
                     float dt) override {
    if (dbs.phase != DishBattleState::Phase::Entering)
      return;

    // TODO: Replace headless mode bypass with --disable-animation flag that
    // calls into vendor library (afterhours::animation) to properly disable
    // animations at the framework level instead of bypassing timers here
    if (render_backend::is_headless_mode) {
      dbs.enter_progress = 1.0f;
      // Skip the rest of the function; the >= 1.0f check below will transition
      // to InCombat
    } else {
      // Ensure forward progress even if dt is zero due to timing anomalies
      const float kFallbackDt = 1.0f / 60.0f;
      float effective_dt = dt > 0.0f ? dt : kFallbackDt;

      const float enter_duration = 0.45f; // seconds
      float scaled_enter_duration = enter_duration / render_backend::timing_speed_scale;
      // Support a start delay by allowing enter_progress to begin negative; we
      // count it up to 0 before progressing the visible animation 0..1.
      if (dbs.enter_progress < 0.0f) {
        dbs.enter_progress = std::min(0.0f, dbs.enter_progress + effective_dt);
        return;
      }
      dbs.enter_progress =
          std::min(1.0f, dbs.enter_progress + effective_dt / scaled_enter_duration);
    }

    if (dbs.enter_progress >= 1.0f) {
      // Log modifiers at Entering -> InCombat boundary
      if (e.has<PairingClashModifiers>()) {
        const auto &pcm = e.get<PairingClashModifiers>();
        log_info("PHASE_TRANSITION: Dish {} Entering->InCombat - PairingClash: "
                 "bodyDelta={}, zingDelta={}",
                 e.id, pcm.bodyDelta, pcm.zingDelta);
      }
      if (e.has<PersistentCombatModifiers>()) {
        const auto &pm = e.get<PersistentCombatModifiers>();
        log_info("PHASE_TRANSITION: Dish {} Entering->InCombat - Persistent: "
                 "bodyDelta={}, zingDelta={}",
                 e.id, pm.bodyDelta, pm.zingDelta);
      }
      // Snap transform to the animation's end position so there is no visual
      // jump when we stop applying the draw-time offset.
      if (e.has<Transform>()) {
        Transform &tr = e.get<Transform>();
        float judge_center_y = 360.0f; // must match renderers
        float delta_y = (judge_center_y - tr.position.y);
        tr.position.y += delta_y;
      }

      dbs.phase = DishBattleState::Phase::InCombat;
      dbs.bite_timer = 0.0f;
      dbs.first_bite_decided = false;
    }
  }
};
