#pragma once

#include "../components/has_tooltip.h"
#include "../components/is_dish.h"
#include "../components/transform.h"
#include "../query.h"
#include "../rl.h"
#include <afterhours/ah.h>

struct RenderTooltipSystem : System<> {
public:
  virtual void once(float) const override {
    Entity *hovered_entity = nullptr;

    // Check all entities with tooltips to see if any are being hovered
    for (auto &ref : EQ().whereHasComponent<Transform>()
                         .whereHasComponent<HasTooltip>()
                         .gen()) {
      auto &entity = ref.get();
      const auto &transform = entity.get<Transform>();

      vec2 mouse_pos = afterhours::input::get_mouse_position();
      Rectangle entity_rect = transform.rect();

      bool mouse_over = CheckCollisionPointRec(
          raylib::Vector2{mouse_pos.x, mouse_pos.y},
          Rectangle{entity_rect.x, entity_rect.y, entity_rect.width,
                    entity_rect.height});

      if (mouse_over) {
        hovered_entity = &entity;
        break;
      }
    }

    if (!hovered_entity) {
      return;
    }

    const auto &tooltip = hovered_entity->get<HasTooltip>();

    // Calculate tooltip position (above the entity)
    vec2 mouse_pos = afterhours::input::get_mouse_position();
    float tooltip_x = mouse_pos.x + 10; // Offset from mouse
    float tooltip_y = mouse_pos.y - 30; // Above mouse

    // Measure text to calculate tooltip size
    float text_width = raylib::MeasureText(tooltip.text.c_str(),
                                           static_cast<int>(tooltip.font_size));
    float tooltip_width = text_width + tooltip.padding * 2;
    float tooltip_height =
        tooltip.font_size + tooltip.padding * 4; // Even taller background

    // Keep tooltip on screen
    float screen_width = raylib::GetScreenWidth();
    float screen_height = raylib::GetScreenHeight();

    if (tooltip_x + tooltip_width > screen_width) {
      tooltip_x = screen_width - tooltip_width - 10;
    }
    if (tooltip_y < 0) {
      tooltip_y = mouse_pos.y + 20; // Below mouse if no space above
    }

    // Draw tooltip background
    raylib::DrawRectangle(
        static_cast<int>(tooltip_x), static_cast<int>(tooltip_y),
        static_cast<int>(tooltip_width), static_cast<int>(tooltip_height),
        tooltip.background_color);

    // Draw tooltip border
    raylib::DrawRectangleLines(static_cast<int>(tooltip_x),
                               static_cast<int>(tooltip_y),
                               static_cast<int>(tooltip_width),
                               static_cast<int>(tooltip_height), raylib::WHITE);

    // Draw tooltip text
    raylib::DrawText(tooltip.text.c_str(),
                     static_cast<int>(tooltip_x + tooltip.padding),
                     static_cast<int>(tooltip_y + tooltip.padding),
                     static_cast<int>(tooltip.font_size), tooltip.text_color);
  }
};
