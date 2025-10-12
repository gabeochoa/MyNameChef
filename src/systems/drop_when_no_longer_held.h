#pragma once

#include "../components/is_drop_slot.h"
#include "../components/is_held.h"
#include "../components/is_inventory_item.h"
#include "../components/is_shop_item.h"
#include "../components/transform.h"
#include "../rl.h"
#include <afterhours/ah.h>
#include <limits>

struct DropWhenNoLongerHeld : System<IsHeld, Transform> {
  void for_each_with(Entity &entity, IsHeld &held, Transform &transform,
                     float) override {

    // If mouse button is released, drop the item
    if (afterhours::input::is_mouse_button_released(
            raylib::MOUSE_BUTTON_LEFT)) {
      vec2 mouse_pos = afterhours::input::get_mouse_position();

      // Find the best drop slot
      Entity *best_drop_slot = nullptr;
      float best_distance = std::numeric_limits<float>::max();

      for (auto &ref : EntityQuery().whereHasComponent<IsDropSlot>().gen()) {
        auto &slot_entity = ref.get();
        if (!slot_entity.has<Transform>())
          continue;

        const auto &slot_transform = slot_entity.get<Transform>();
        const auto &drop_slot = slot_entity.get<IsDropSlot>();

        // Check if this slot accepts this type of item
        bool can_drop = false;
        if (entity.has<IsInventoryItem>() &&
            drop_slot.accepts_inventory_items) {
          can_drop = true;
        } else if (entity.has<IsShopItem>() && drop_slot.accepts_shop_items) {
          can_drop = true;
        }

        if (!can_drop)
          continue;

        // Check if mouse is over this slot
        Rectangle slot_rect = slot_transform.rect();
        bool mouse_over_slot = CheckCollisionPointRec(
            raylib::Vector2{mouse_pos.x, mouse_pos.y},
            Rectangle{slot_rect.x, slot_rect.y, slot_rect.width,
                      slot_rect.height});

        if (mouse_over_slot) {
          // Calculate distance from mouse to slot center
          vec2 slot_center = slot_transform.center();
          float distance =
              Vector2Distance(raylib::Vector2{mouse_pos.x, mouse_pos.y},
                              raylib::Vector2{slot_center.x, slot_center.y});

          if (distance < best_distance) {
            best_distance = distance;
            best_drop_slot = &slot_entity;
          }
        }
      }

      // Place the item
      if (best_drop_slot) {
        // Snap to the drop slot
        const auto &slot_transform = best_drop_slot->get<Transform>();
        vec2 slot_center = slot_transform.center();
        vec2 item_size = transform.size;

        // Update the item's position to center it on the slot
        entity.get<Transform>().position = slot_center - item_size * 0.5f;

        // Update slot information if needed
        if (entity.has<IsInventoryItem>()) {
          entity.get<IsInventoryItem>().slot =
              best_drop_slot->get<IsDropSlot>().slot_id;
        } else if (entity.has<IsShopItem>()) {
          entity.get<IsShopItem>().slot =
              best_drop_slot->get<IsDropSlot>().slot_id;
        }
      } else {
        // No valid drop slot found, return to original position
        // For now, we'll just keep it where it is
        // TODO: Could implement snap-back to original position
      }

      // Remove the IsHeld component
      entity.removeComponent<IsHeld>();
    } else {
      // Continue dragging - update position to follow mouse
      vec2 mouse_pos = afterhours::input::get_mouse_position();
      vec2 new_position = mouse_pos + held.offset - transform.size * 0.5f;
      entity.get<Transform>().position = new_position;
    }
  }
};