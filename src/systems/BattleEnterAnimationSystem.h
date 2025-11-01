#pragma once

#include "../components/deferred_flavor_mods.h"
#include "../components/dish_battle_state.h"
#include "../components/pairing_clash_modifiers.h"
#include "../components/persistent_combat_modifiers.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

struct BattleEnterAnimationSystem : afterhours::System<DishBattleState> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &e, DishBattleState &dbs,
                     float dt) override {
    if (dbs.phase != DishBattleState::Phase::Entering)
      return;

    const float enter_duration = 0.45f; // seconds
    // Support a start delay by allowing enter_progress to begin negative; we
    // count it up to 0 before progressing the visible animation 0..1.
    if (dbs.enter_progress < 0.0f) {
      dbs.enter_progress = std::min(0.0f, dbs.enter_progress + dt);
      return;
    }
    dbs.enter_progress =
        std::min(1.0f, dbs.enter_progress + dt / enter_duration);

    if (dbs.enter_progress >= 1.0f) {
      // Log modifiers at Entering -> InCombat boundary
      if (e.has<DeferredFlavorMods>()) {
        const auto &def = e.get<DeferredFlavorMods>();
        log_info(
            "PHASE_TRANSITION: Dish {} Entering->InCombat - DeferredFlavorMods "
            "PRESENT just before phase switch: satiety={}, sweetness={}, "
            "spice={}, acidity={}, umami={}, richness={}, freshness={}",
            e.id, def.satiety, def.sweetness, def.spice, def.acidity, def.umami,
            def.richness, def.freshness);
      } else {
        log_info("PHASE_TRANSITION: Dish {} Entering->InCombat - "
                 "DeferredFlavorMods: none just before phase switch",
                 e.id);
      }
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
