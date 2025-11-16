#pragma once

#include "../components/dish_level.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../components/transform.h"
#include "../font_info.h"
#include "../game_state_manager.h"
#include "../render_backend.h"
#include "../render_constants.h"
#include "../rl.h"
#include "../ui/text_formatting.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/color.h>

using namespace afterhours;

struct RenderDishProgressBars : System<IsDish, DishLevel, Transform> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  void for_each_with(const Entity &, const IsDish &, const DishLevel &level,
                     const Transform &transform, float) const override {

    // Render progress bars for both shop and inventory items
    // (Remove the restriction to only inventory items)

    vec2 center = transform.center();
    vec2 bar_position = {center.x - 20, center.y + transform.size.y * 0.5f + 8};

    // Draw background bar with border (smaller size)
    DrawRectangle(static_cast<int>(bar_position.x),
                  static_cast<int>(bar_position.y), 40, 10,
                  Color{30, 30, 30, 255}); // Darker background
    DrawRectangleLines(static_cast<int>(bar_position.x),
                       static_cast<int>(bar_position.y), 40, 10,
                       Color{255, 255, 255, 255}); // White border

    raylib::Color gold_color = text_formatting::TextFormatting::get_color(
        text_formatting::SemanticColor::Gold,
        text_formatting::FormattingContext::HUD);
    raylib::Color muted_color = text_formatting::TextFormatting::get_color(
        text_formatting::SemanticColor::TextMuted,
        text_formatting::FormattingContext::HUD);
    raylib::Color accent_color = text_formatting::TextFormatting::get_color(
        text_formatting::SemanticColor::Accent,
        text_formatting::FormattingContext::HUD);

    // Draw progress indicators (smaller)
    for (int i = 0; i < level.merges_needed; ++i) {
      Color color = (i < level.merge_progress) ? gold_color : muted_color;

      int x = static_cast<int>(bar_position.x) + 2 + i * 12;
      int y = static_cast<int>(bar_position.y) + 2;

      DrawRectangle(x, y, 8, 6, color);
    }

    // Draw level text if above level 1 (make it more visible)
    if (level.level > 1) {
      std::string level_text = "L" + std::to_string(level.level);
      render_backend::DrawTextWithActiveFont(level_text.c_str(), static_cast<int>(center.x - 10),
                                            static_cast<int>(bar_position.y - 25), font_sizes::Normal,
                                            accent_color);
    }
  }
};
