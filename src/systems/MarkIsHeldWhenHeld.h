#pragma once

#include "../components/is_draggable.h"
#include "../components/is_drop_slot.h"
#include "../components/is_held.h"
#include "../components/is_inventory_item.h"
#include "../components/is_shop_item.h"
#include "../components/transform.h"
#include "../query.h"
#include "../rl.h"
#include <afterhours/ah.h>

using namespace afterhours;

struct MarkIsHeldWhenHeld : System<IsDraggable, Transform> {
private:
  // Helper function to get the slot ID from an entity
  int get_slot_id(const Entity &entity) {
    if (entity.has<IsInventoryItem>()) {
      return entity.get<IsInventoryItem>().slot;
    } else if (entity.has<IsShopItem>()) {
      return entity.get<IsShopItem>().slot;
    }
    return -1;
  }

  void mark_slot_unoccupied(int slot_id) {
    auto slot_entity =
        EQ().whereHasComponent<IsDropSlot>().whereSlotID(slot_id).gen_first();
    if (slot_entity) {
      slot_entity->get<IsDropSlot>().occupied = false;
    }
  }

public:
  void for_each_with(Entity &entity, IsDraggable &draggable,
                     Transform &transform, float) override {
    if (!draggable.enabled) {
      return;
    }

    // Check if mouse is over this entity
    vec2 mouse_pos = afterhours::input::get_mouse_position();
    Rectangle entity_rect = transform.rect();

    bool mouse_over = CheckCollisionPointRec(
        raylib::Vector2{mouse_pos.x, mouse_pos.y},
        Rectangle{entity_rect.x, entity_rect.y, entity_rect.width,
                  entity_rect.height});

    // If mouse is over and left button is pressed, add IsHeld component
    if (mouse_over &&
        afterhours::input::is_mouse_button_pressed(raylib::MOUSE_BUTTON_LEFT)) {
      if (!entity.has<IsHeld>()) {
        // Calculate offset from mouse to entity center
        vec2 entity_center = transform.center();
        vec2 offset = entity_center - mouse_pos;

        // Store original position for snap-back
        vec2 original_position = transform.position;

        entity.addComponent<IsHeld>(offset, original_position);

        // Mark the slot this item was in as unoccupied
        int item_slot = get_slot_id(entity);
        mark_slot_unoccupied(item_slot);
      }
    }
  }
};
