#pragma once

#include "../components/dish_level.h"
#include "../components/is_dish.h"
#include "../components/transform.h"
#include "../font_info.h"
#include "../game_state_manager.h"
#include "../render_backend.h"
#include "../rl.h"
#include <afterhours/ah.h>
#include <string>

struct RenderDishLevelBadgeSystem
    : afterhours::System<IsDish, DishLevel, Transform> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  void for_each_with(const Entity &, const IsDish &,
                     const DishLevel &level, const Transform &transform,
                     float) const override {
    if (level.level <= 1) {
      return;
    }

    float badge_x = transform.position.x + transform.size.x - 12;
    float badge_y = transform.position.y + 12;
    float radius = 11.0f;

    render_backend::DrawCircle(static_cast<int>(badge_x),
                               static_cast<int>(badge_y), radius,
                               raylib::Color{255, 215, 0, 255});

    std::string text = std::to_string(level.level);
    float fontSize = font_sizes::Small;
    float textWidth =
        render_backend::MeasureTextWithActiveFont(text.c_str(), fontSize);
    render_backend::DrawTextWithActiveFont(
        text.c_str(), static_cast<int>(badge_x - textWidth / 2.0f),
        static_cast<int>(badge_y - fontSize / 2.0f), fontSize,
        raylib::Color{0, 0, 0, 255});
  }
};
