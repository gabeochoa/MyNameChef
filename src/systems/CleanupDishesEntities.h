#pragma once

#include "../components/is_gallery_item.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

struct CleanupDishesEntities : afterhours::System<> {
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override { return true; }

  void once(float) override {
    auto &gsm = GameStateManager::get();
    if (last_screen == GameStateManager::Screen::Dishes &&
        gsm.active_screen != GameStateManager::Screen::Dishes) {
      for (auto &ref : afterhours::EntityQuery()
                           .template whereHasComponent<IsGalleryItem>()
                           .gen()) {
        ref.get().cleanup = true;
      }
    }
    last_screen = gsm.active_screen;
  }
};
