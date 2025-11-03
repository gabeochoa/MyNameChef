#pragma once

#include <afterhours/ah.h>
//
#include "../game_state_manager.h"
#include "../shop.h"

struct InitialShopFill : afterhours::System<> {
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

    // Get current shop tier
    auto shop_tier_entity = afterhours::EntityHelper::get_singleton<ShopTier>();
    int current_tier = 1; // Default to tier 1
    if (shop_tier_entity.get().has<ShopTier>()) {
      current_tier = shop_tier_entity.get().get<ShopTier>().current_tier;
    }

    for (int slot : free_slots) {
      make_shop_item(slot, get_random_dish_for_tier(current_tier));
    }
    afterhours::EntityHelper::merge_entity_arrays();
    log_info("Store refilled with {} new items at tier {}", free_slots.size(),
             current_tier);
  }
};
