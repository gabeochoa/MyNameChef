#pragma once

#include "../components/is_drop_slot.h"
#include "../components/is_shop_item.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

struct CleanupShopEntities : afterhours::System<> {
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.is_game_active();
  }

  void once(float) override {
    auto &gsm = GameStateManager::get();

    // Only clean up when leaving the shop screen
    if (last_screen == GameStateManager::Screen::Shop &&
        gsm.active_screen != GameStateManager::Screen::Shop) {

      // Mark all shop item entities for cleanup
      for (auto &ref : afterhours::EntityQuery()
                           .template whereHasComponent<IsShopItem>()
                           .gen()) {
        ref.get().cleanup = true;
        log_info("Marked shop item entity {} for cleanup", ref.get().id);
      }

      // Mark all shop drop slot entities for cleanup (shop-only slots)
      for (auto &ref : afterhours::EntityQuery()
                           .template whereHasComponent<IsDropSlot>()
                           .gen()) {
        const auto &drop_slot = ref.get().get<IsDropSlot>();
        // Shop-only slots: accepts_inventory=false, accepts_shop=true
        if (!drop_slot.accepts_inventory_items &&
            drop_slot.accepts_shop_items) {
          ref.get().cleanup = true;
          log_info("Marked shop drop slot entity {} for cleanup", ref.get().id);
        }
      }

      log_info("Cleaned up shop entities when leaving shop screen");
    }

    last_screen = gsm.active_screen;
  }
};
