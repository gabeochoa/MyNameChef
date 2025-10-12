#pragma once

#include "../game_state_manager.h"
#include "../shop.h"
#include <afterhours/ah.h>

using namespace afterhours;

struct InitialShopFill : System<> {
  bool filled = false;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();

    // Only run once when first entering shop screen
    return !filled && gsm.active_screen == GameStateManager::Screen::Shop;
  }

  void once(float) override {
    refill_store();
    filled = true;
    log_info("Initial shop filled");
  }

private:
  void refill_store() {
    auto free_slots = get_free_slots(SHOP_SLOTS);
    for (int slot : free_slots) {
      make_shop_item(slot, get_random_dish());
    }
    EntityHelper::merge_entity_arrays();
    log_info("Store refilled with {} new items", free_slots.size());
  }
};
