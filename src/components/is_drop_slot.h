#pragma once

#include <afterhours/ah.h>

// Tag: entity represents a slot where draggable items can be dropped
struct IsDropSlot : afterhours::BaseComponent {
  int slot_id = -1; // Unique identifier for this slot
  bool accepts_inventory_items = true;
  bool accepts_shop_items = true;
  bool occupied = false; // Whether this slot currently has an item

  IsDropSlot() = default;
  explicit IsDropSlot(int id) : slot_id(id) {}
  IsDropSlot(int id, bool inventory, bool shop)
      : slot_id(id), accepts_inventory_items(inventory),
        accepts_shop_items(shop) {}
};
