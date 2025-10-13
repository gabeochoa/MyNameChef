#pragma once

#include "../components/is_drop_slot.h"
#include "../game_state_manager.h"
#include "../shop.h"
#include <afterhours/ah.h>

using namespace afterhours;

struct GenerateInventorySlots : System<> {
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.is_game_active();
  }

  void once(float) override {
    auto &gsm = GameStateManager::get();

    // Generate inventory slots when entering shop screen
    if (gsm.active_screen == GameStateManager::Screen::Shop &&
        last_screen != GameStateManager::Screen::Shop) {

      // Check if inventory slots already exist
      bool inventory_slots_exist = false;
      for (auto &ref : afterhours::EntityQuery()
                           .template whereHasComponent<IsDropSlot>()
                           .gen()) {
        const auto &drop_slot = ref.get().template get<IsDropSlot>();
        if (drop_slot.accepts_inventory_items && drop_slot.accepts_shop_items) {
          inventory_slots_exist = true;
          break;
        }
      }

      // Only create inventory slots if they don't exist
      if (!inventory_slots_exist) {
        for (int i = 0; i < INVENTORY_SLOTS; ++i) {
          auto position = calculate_inventory_position(i);
          make_drop_slot(INVENTORY_SLOT_OFFSET + i, position,
                         vec2{SLOT_SIZE, SLOT_SIZE}, true, true);
        }

        // Create a sell drop slot to the right of inventory
        {
          vec2 pos{static_cast<float>(SELL_SLOT_X),
                   static_cast<float>(SELL_SLOT_Y)};
          make_drop_slot(SELL_SLOT_ID, pos, vec2{SLOT_SIZE, SLOT_SIZE}, true,
                         false);
        }

        EntityHelper::merge_entity_arrays();
        log_info("Generated inventory slots for shop screen");
      }
    }

    last_screen = gsm.active_screen;
  }
};
