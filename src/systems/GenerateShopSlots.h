#pragma once
#include <afterhours/ah.h>
//
#include "../components/is_drop_slot.h"
#include "../game_state_manager.h"
#include "../shop.h"

using namespace afterhours;

struct GenerateShopSlots : System<> {
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.is_game_active();
  }

  void once(float) override {
    auto &gsm = GameStateManager::get();

    // Generate shop slots when entering shop screen
    if (gsm.active_screen == GameStateManager::Screen::Shop &&
        last_screen != GameStateManager::Screen::Shop) {

      // Check if shop slots already exist
      bool shop_slots_exist = false;
      for (auto &ref : afterhours::EntityQuery()
                           .template whereHasComponent<IsDropSlot>()
                           .gen()) {
        const auto &drop_slot = ref.get().template get<IsDropSlot>();
        if (!drop_slot.accepts_inventory_items &&
            drop_slot.accepts_shop_items) {
          shop_slots_exist = true;
          break;
        }
      }

      // Only create shop slots if they don't exist
      if (!shop_slots_exist) {
        for (int i = 0; i < SHOP_SLOTS; ++i) {
          auto position =
              calculate_slot_position(i, SHOP_START_X, SHOP_START_Y);
          make_drop_slot(i, position, vec2{SLOT_SIZE, SLOT_SIZE}, false, true);
          // TODO: Add UI labels to shop slots for better testability
          // Expected: Each shop slot should have a UI label like "Shop Slot 1", "Shop Slot 2", etc.
          // This would allow UITestHelpers::visible_ui_exists("Shop Slot 1") to work
        }

        EntityHelper::merge_entity_arrays();
        log_info("Generated shop slots for shop screen");
      }
    }

    last_screen = gsm.active_screen;
  }
};
