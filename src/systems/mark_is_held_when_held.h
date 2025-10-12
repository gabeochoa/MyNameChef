#pragma once

#include "../components/is_draggable.h"
#include "../components/is_held.h"
#include "../components/transform.h"
#include "../rl.h"
#include <afterhours/ah.h>

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

        entity.addComponent<IsHeld>(offset);
      }
    }
  }
};
