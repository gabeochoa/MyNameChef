#pragma once

#include "../components/battle_team_tags.h"
#include "../components/is_dish.h"
#include "../components/judging_state.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/animation.h>
#include <cmath>

enum struct ScoreBarKey : size_t { Handle };

struct RenderScoringBar : afterhours::System<> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void once(float dt) const override {
    // Read live judging totals if available
    int pTotal = 0;
    int oTotal = 0;
    auto jsEnt = afterhours::EntityHelper::get_singleton<JudgingState>();
    if (jsEnt.get().has<JudgingState>()) {
      const auto &js = jsEnt.get().get<JudgingState>();
      pTotal = js.player_total;
      oTotal = js.opponent_total;
    }

    // Layout (left column)
    const int margin_x = 20;
    const int top = 100;
    const int height = 420;
    const int width = 10;
    const int center_x = margin_x + width / 2;
    const int center_y = top + height / 2;

    // Draw bar background and center line
    raylib::DrawRectangle(margin_x, top, width, height,
                          raylib::Color{40, 40, 40, 255});
    raylib::DrawLine(margin_x - 6, center_y, margin_x + width + 6, center_y,
                     raylib::GRAY);

    // Compute target bias from totals (-1..1), up favors player, down favors
    // opponent
    const int p = std::max(0, pTotal);
    const int o = std::max(0, oTotal);
    const int sum = std::max(1, p + o);
    const float target_bias =
        static_cast<float>(p - o) / static_cast<float>(sum); // [-1,1]

    // Smoothly track target with simple per-frame lerp (independent of anim
    // sys)
    static float bias_value = 0.0f;
    const float speed = 6.0f; // responsiveness per second
    const float t = std::clamp(speed * dt, 0.0f, 1.0f);
    const float prev = bias_value;
    bias_value = std::clamp(prev + (target_bias - prev) * t, -1.0f, 1.0f);
    // no-op logging removed

    // Map bias to Y position on bar (upwards for positive bias)
    const float half = static_cast<float>(height) * 0.5f;
    const float y_offset = -bias_value * (half - 6.0f);
    const int handle_y = static_cast<int>(center_y + y_offset);

    // Draw handle
    const int handle_w = 26;
    const int handle_h = 8;
    const int handle_x = center_x - handle_w / 2;
    raylib::Color handle_col = bias_value >= 0.0f ? raylib::GREEN : raylib::RED;
    raylib::DrawRectangle(handle_x, handle_y - handle_h / 2, handle_w, handle_h,
                          handle_col);
    raylib::DrawRectangleLines(handle_x, handle_y - handle_h / 2, handle_w,
                               handle_h, raylib::WHITE);

    // Labels
    raylib::DrawText("YOU", margin_x + width + 12, top + 4, 14, raylib::GREEN);
    raylib::DrawText("OPP", margin_x + width + 12, top + height - 20, 14,
                     raylib::RED);
  }
};
