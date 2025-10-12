#pragma once

#include "../components/is_drop_slot.h"
#include "../components/render_order.h"
#include "../components/transform.h"
#include "../shop.h"
#include <afterhours/ah.h>

using namespace afterhours;

struct GenerateShopSlots : System<> {
  bool initialized = false;

  void once(float) override {
    if (initialized)
      return;
    initialized = true;

    for (int i = 0; i < SHOP_SLOTS; ++i) {
      auto position = calculate_slot_position(i, SHOP_START_X, SHOP_START_Y);
      make_drop_slot(i, position, vec2{SLOT_SIZE, SLOT_SIZE}, false, true);
    }

    EntityHelper::merge_entity_arrays();
  }
};
