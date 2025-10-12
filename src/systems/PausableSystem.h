#pragma once

// #include "game_state_manager.h"  // TODO: This file is missing
#include <afterhours/ah.h>

template <typename... Components>
struct PausableSystem : afterhours::System<Components...> {
  virtual bool should_run(float) override {
    return true; // TODO: GameStateManager::get().is_paused() - GameStateManager is missing
  }
};
