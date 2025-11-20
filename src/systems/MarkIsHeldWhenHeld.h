#pragma once

#include "../components/is_draggable.h"
#include "../components/is_drop_slot.h"
#include "../components/is_held.h"
#include "../components/is_inventory_item.h"
#include "../components/is_shop_item.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../log.h"
#include "../query.h"
#include "../rl.h"
#include "../shop.h"
#include "../testing/test_input.h"
#include <afterhours/ah.h>

using namespace afterhours;

struct MarkIsHeldWhenHeld : System<IsDraggable, Transform> {
  virtual bool should_run(float) override {
    GameStateManager &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

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
    afterhours::OptEntity slot_entity =
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
    // Check test input wrapper first, then fall back to real input
    vec2 mouse_pos = test_input::get_mouse_position();
    Rectangle entity_rect = transform.rect();

    bool mouse_over = CheckCollisionPointRec(
        raylib::Vector2{mouse_pos.x, mouse_pos.y},
        Rectangle{entity_rect.x, entity_rect.y, entity_rect.width,
                  entity_rect.height});

    if (!mouse_over) {
      return;
    }

    bool button_pressed = test_input::is_mouse_button_pressed(raylib::MOUSE_BUTTON_LEFT);
    if (!button_pressed) {
      return;
    }
    
    log_error("MARK_IS_HELD: Entity {} - mouse over and button pressed, marking as held", entity.id);

    if (entity.has<IsHeld>()) {
      return;
    }

    // Skip drag if clicking in the freeze icon area (bottom-left corner)
    if (entity.has<IsShopItem>() && entity.has<Freezeable>()) {
      const float icon_size = 24.0f;
      const float padding = 5.0f;
      const float icon_x = transform.position.x + padding;
      const float icon_y =
          transform.position.y + transform.size.y - icon_size - padding;
      const Rectangle icon_rect = {icon_x, icon_y, icon_size, icon_size};

      bool mouse_in_icon = CheckCollisionPointRec(
          raylib::Vector2{mouse_pos.x, mouse_pos.y}, icon_rect);

      if (mouse_in_icon) {
        // Let HandleFreezeIconClick handle this click instead
        return;
      }
    }

    // Calculate offset from mouse to entity center
    vec2 entity_center = transform.center();
    vec2 offset = entity_center - mouse_pos;

    // Store original position for snap-back
    vec2 original_position = transform.position;

    entity.addComponent<IsHeld>(offset, original_position);

    // Mark the slot this item was in as unoccupied
    int item_slot = get_slot_id(entity);
    mark_slot_unoccupied(item_slot);
    
    // Mark that the press flag was actually consumed
    if (test_input::is_simulation_active()) {
      test_input::mark_press_consumed();
    }
  }
};
