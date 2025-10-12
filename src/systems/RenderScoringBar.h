#pragma once

#include "../components/battle_team_tags.h"
#include "../components/is_dish.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/animation.h>

enum struct ScoreBarKey : size_t { Handle };

struct RenderScoringBar : afterhours::System<> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void once(float) const override {
    // Compute quick proxy totals from current teams on Battle screen
    int pTotal = 0;
    for (auto &ref : afterhours::EntityQuery()
                         .whereHasComponent<IsPlayerTeamItem>()
                         .whereHasComponent<IsDish>()
                         .gen()) {
      const auto &dish = ref.get().get<IsDish>();
      DishInfo info = get_dish_info(dish.type);
      const auto &f = info.flavor;
      pTotal += f.satiety + f.sweetness + f.spice + f.acidity + f.umami +
                f.richness + f.freshness;
    }
    int oTotal = 0;
    for (auto &ref : afterhours::EntityQuery()
                         .whereHasComponent<IsOpponentTeamItem>()
                         .whereHasComponent<IsDish>()
                         .gen()) {
      const auto &dish = ref.get().get<IsDish>();
      DishInfo info = get_dish_info(dish.type);
      const auto &f = info.flavor;
      oTotal += f.satiety + f.sweetness + f.spice + f.acidity + f.umami +
                f.richness + f.freshness;
    }

    // Layout (left column)
    const int margin_x = 20;
    const int top = 100;
    const int height = 420;
    const int width = 10;
    const int center_x = margin_x + width / 2;
    const int center_y = top + height / 2;

    // Draw bar background and center line
    raylib::DrawRectangle(margin_x, top, width, height, raylib::Color{40, 40, 40, 255});
    raylib::DrawLine(margin_x - 6, center_y, margin_x + width + 6, center_y, raylib::GRAY);

    // Compute target bias from totals (-1..1), up favors player, down favors opponent
    const int p = std::max(0, pTotal);
    const int o = std::max(0, oTotal);
    const int sum = std::max(1, p + o);
    const float target_bias = static_cast<float>(p - o) / static_cast<float>(sum); // [-1,1]

    // Start animation to target if not active yet
    auto handle = afterhours::animation::anim(ScoreBarKey::Handle);
    if (!handle.is_active() && std::abs(handle.value()) < 1e-6f) {
      afterhours::animation::one_shot(ScoreBarKey::Handle, [target_bias](auto h) {
        h.from(0.0f)
            .to(target_bias * 1.05f, 0.35f, afterhours::animation::EasingType::EaseOutQuad)
            .to(target_bias, 0.15f, afterhours::animation::EasingType::EaseOutQuad);
      });
    }

    // Current animated bias value
    const float bias = std::clamp(afterhours::animation::clamp_value(ScoreBarKey::Handle, -1.0f, 1.0f), -1.0f, 1.0f);

    // Map bias to Y position on bar (upwards for positive bias)
    const float half = static_cast<float>(height) * 0.5f;
    const float y_offset = -bias * (half - 6.0f);
    const int handle_y = static_cast<int>(center_y + y_offset);

    // Draw handle
    const int handle_w = 26;
    const int handle_h = 8;
    const int handle_x = center_x - handle_w / 2;
    raylib::Color handle_col = bias >= 0.0f ? raylib::GREEN : raylib::RED;
    raylib::DrawRectangle(handle_x, handle_y - handle_h / 2, handle_w, handle_h, handle_col);
    raylib::DrawRectangleLines(handle_x, handle_y - handle_h / 2, handle_w, handle_h, raylib::WHITE);

    // Labels
    raylib::DrawText("YOU", margin_x + width + 12, top + 4, 14, raylib::GREEN);
    raylib::DrawText("OPP", margin_x + width + 12, top + height - 20, 14, raylib::RED);
  }
};


