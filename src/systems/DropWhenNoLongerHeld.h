#pragma once

#include "../components/can_drop_onto.h"
#include "../components/is_drop_slot.h"
#include "../components/is_held.h"
#include "../components/is_inventory_item.h"
#include "../components/is_shop_item.h"
#include "../components/transform.h"
#include "../rl.h"
#include <afterhours/ah.h>
#include <limits>

using namespace afterhours;

struct DropWhenNoLongerHeld : System<IsHeld, Transform> {
  void for_each_with(Entity &entity, IsHeld &held, Transform &transform,
                     float) override {

    // If mouse button is released, drop the item
    if (afterhours::input::is_mouse_button_released(
            raylib::MOUSE_BUTTON_LEFT)) {
      vec2 mouse_pos = afterhours::input::get_mouse_position();

      // Find the best drop slot (empty or occupied for swapping)
      Entity *best_drop_slot = nullptr;
      Entity *occupied_slot = nullptr;
      float best_distance = std::numeric_limits<float>::max();

      for (auto &ref : EntityQuery().whereHasComponent<IsDropSlot>().gen()) {
        auto &slot_entity = ref.get();
        if (!slot_entity.has<Transform>())
          continue;

        // Check if this slot has CanDropOnto component
        if (!slot_entity.has<CanDropOnto>())
          continue;

        const auto &can_drop = slot_entity.get<CanDropOnto>();
        if (!can_drop.enabled)
          continue;

        const auto &slot_transform = slot_entity.get<Transform>();
        const auto &drop_slot = slot_entity.get<IsDropSlot>();

        // Check if this slot accepts this type of item
        bool can_drop_item = false;
        if (entity.has<IsInventoryItem>() &&
            drop_slot.accepts_inventory_items) {
          can_drop_item = true;
        } else if (entity.has<IsShopItem>() && drop_slot.accepts_shop_items) {
          can_drop_item = true;
        }

        if (!can_drop_item)
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

          if (drop_slot.occupied) {
            // This is an occupied slot - check if we can swap
            if ((entity.has<IsInventoryItem>() &&
                 drop_slot.accepts_inventory_items) ||
                (entity.has<IsShopItem>() && drop_slot.accepts_shop_items)) {
              occupied_slot = &slot_entity;
            }
          } else {
            // This is an empty slot
            if (distance < best_distance) {
              best_distance = distance;
              best_drop_slot = &slot_entity;
            }
          }
        }
      }

      // Place the item
      if (best_drop_slot) {
        // Drop into empty slot
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

        // Mark the slot as occupied
        best_drop_slot->get<IsDropSlot>().occupied = true;
      } else if (occupied_slot) {
        // Handle swapping with occupied slot
        const auto &slot_transform = occupied_slot->get<Transform>();
        vec2 slot_center = slot_transform.center();
        vec2 item_size = transform.size;

        // Find the item currently in the occupied slot
        Entity *item_in_slot = nullptr;
        int target_slot_id = occupied_slot->get<IsDropSlot>().slot_id;

        // Search for inventory items in the slot
        for (auto &ref :
             EntityQuery().whereHasComponent<IsInventoryItem>().gen()) {
          auto &other_entity = ref.get();
          if (other_entity.has<IsInventoryItem>() &&
              other_entity.get<IsInventoryItem>().slot == target_slot_id &&
              &other_entity != &entity) {
            item_in_slot = &other_entity;
            break;
          }
        }

        // If not found, search for shop items in the slot
        if (!item_in_slot) {
          for (auto &ref :
               EntityQuery().whereHasComponent<IsShopItem>().gen()) {
            auto &other_entity = ref.get();
            if (other_entity.has<IsShopItem>() &&
                other_entity.get<IsShopItem>().slot == target_slot_id &&
                &other_entity != &entity) {
              item_in_slot = &other_entity;
              break;
            }
          }
        }

        if (item_in_slot) {
          // Get the original slot of the item being dropped
          int original_slot_id = -1;
          if (entity.has<IsInventoryItem>()) {
            original_slot_id = entity.get<IsInventoryItem>().slot;
          } else if (entity.has<IsShopItem>()) {
            original_slot_id = entity.get<IsShopItem>().slot;
          }

          // Swap positions
          entity.get<Transform>().position = slot_center - item_size * 0.5f;
          item_in_slot->get<Transform>().position = held.original_position;

          // Swap slot assignments
          if (entity.has<IsInventoryItem>()) {
            entity.get<IsInventoryItem>().slot = target_slot_id;
          } else if (entity.has<IsShopItem>()) {
            entity.get<IsShopItem>().slot = target_slot_id;
          }

          if (item_in_slot->has<IsInventoryItem>()) {
            item_in_slot->get<IsInventoryItem>().slot = original_slot_id;
          } else if (item_in_slot->has<IsShopItem>()) {
            item_in_slot->get<IsShopItem>().slot = original_slot_id;
          }

          // Update slot occupied states
          // The target slot remains occupied (we're putting an item there)
          occupied_slot->get<IsDropSlot>().occupied = true;

          // Find and update the original slot's occupied state
          if (original_slot_id >= 0) {
            for (auto &ref :
                 EntityQuery().whereHasComponent<IsDropSlot>().gen()) {
              auto &slot_entity = ref.get();
              if (slot_entity.get<IsDropSlot>().slot_id == original_slot_id) {
                slot_entity.get<IsDropSlot>().occupied = true;
                break;
              }
            }
          }
        }
      } else {
        // No valid drop slot found, snap back to original position
        entity.get<Transform>().position = held.original_position;
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