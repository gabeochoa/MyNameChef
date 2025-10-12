#pragma once

#include "../components/is_draggable.h"
#include "../components/is_drop_slot.h"
#include "../components/is_held.h"
#include "../components/is_inventory_item.h"
#include "../components/is_shop_item.h"
#include "../components/transform.h"
#include "../rl.h"
#include <afterhours/ah.h>

using namespace afterhours;

struct MarkIsHeldWhenHeld : System<IsDraggable, Transform> {
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
        int item_slot = -1;
        if (entity.has<IsInventoryItem>()) {
          item_slot = entity.get<IsInventoryItem>().slot;
        } else if (entity.has<IsShopItem>()) {
          item_slot = entity.get<IsShopItem>().slot;
        }

        if (item_slot >= 0) {
          // Find the slot entity and mark it as unoccupied
          for (auto &ref :
               EntityQuery().whereHasComponent<IsDropSlot>().gen()) {
            auto &slot_entity = ref.get();
            if (slot_entity.get<IsDropSlot>().slot_id == item_slot) {
              slot_entity.get<IsDropSlot>().occupied = false;
              break;
            }
          }
        }
      }
    }
  }
};
