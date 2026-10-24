#pragma once

#include "../components/merge_flash.h"
#include "../game_state_manager.h"
#include "../render_backend.h"
#include <afterhours/ah.h>

struct MergeFlashTimerSystem : afterhours::System<MergeFlash> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  void for_each_with(afterhours::Entity &entity, MergeFlash &flash,
                     float dt) override {
    flash.elapsed += dt * render_backend::timing_speed_scale;
    if (flash.elapsed >= flash.duration) {
      entity.removeComponent<MergeFlash>();
    }
  }
};
