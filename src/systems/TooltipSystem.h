#pragma once

#include "../components/has_tooltip.h"
#include "../components/is_dish.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../rl.h"
#include <afterhours/ah.h>
#include <map>
#include <sstream>

struct RenderTooltipSystem : System<> {
public:
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    const bool is_playing =
        gsm.current_state == GameStateManager::GameState::Playing;
    const bool on_allowed_screen =
        gsm.active_screen == GameStateManager::Screen::Shop ||
        gsm.active_screen == GameStateManager::Screen::Battle;
    return is_playing && on_allowed_screen;
  }

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

    // Calculate height based on number of lines
    int line_count = 1; // Start with 1 for the first line
    for (char c : tooltip.text) {
      if (c == '\n') {
        line_count++;
      }
    }
    float tooltip_height =
        (tooltip.font_size * line_count) + tooltip.padding * 2;

    // Keep tooltip on screen
    float screen_width = raylib::GetScreenWidth();

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

    // Draw tooltip text with color support
    float current_y = tooltip_y + tooltip.padding;
    std::istringstream text_stream(tooltip.text);
    std::string line;

    // Color mapping
    std::map<std::string, raylib::Color> color_map = {
        {"[GOLD]", raylib::GOLD},     {"[RED]", raylib::RED},
        {"[GREEN]", raylib::GREEN},   {"[BLUE]", raylib::BLUE},
        {"[ORANGE]", raylib::ORANGE}, {"[PURPLE]", raylib::PURPLE},
        {"[CYAN]", raylib::SKYBLUE},  {"[YELLOW]", raylib::YELLOW}};

    while (std::getline(text_stream, line)) {
      raylib::Color text_color = tooltip.text_color; // Default color

      // Check for color markers
      for (const auto &[marker, color] : color_map) {
        if (line.find(marker) != std::string::npos) {
          text_color = color;
          // Remove the marker from the line
          size_t pos = line.find(marker);
          if (pos != std::string::npos) {
            line.replace(pos, marker.length(), "");
          }
          break;
        }
      }

      // Draw the line with the determined color
      raylib::DrawText(line.c_str(),
                       static_cast<int>(tooltip_x + tooltip.padding),
                       static_cast<int>(current_y),
                       static_cast<int>(tooltip.font_size), text_color);
      current_y += tooltip.font_size;
    }
  }
};
