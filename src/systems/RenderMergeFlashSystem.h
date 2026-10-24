#pragma once

#include "../components/merge_flash.h"
#include "../components/transform.h"
#include "../font_info.h"
#include "../game_state_manager.h"
#include "../render_backend.h"
#include "../rl.h"
#include <afterhours/ah.h>
#include <algorithm>
#include <string>

struct RenderMergeFlashSystem
    : afterhours::System<MergeFlash, Transform> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  void for_each_with(const afterhours::Entity &,
                     const MergeFlash &flash,
                     const Transform &transform, float) const override {
    float progress =
        std::clamp(flash.elapsed / flash.duration, 0.0f, 1.0f);
    if (progress >= 1.0f) {
      return;
    }

    vec2 center = transform.center();
    unsigned char alpha =
        static_cast<unsigned char>(200.0f * (1.0f - progress));
    raylib::Color gold = raylib::Color{255, 215, 0, alpha};

    float radius = 30.0f + progress * 40.0f;
    render_backend::DrawCircleLines(static_cast<int>(center.x),
                                    static_cast<int>(center.y),
                                    radius, gold);

    if (!flash.leveledUp) {
      return;
    }

    std::string text = "LEVEL UP!";
    float fontSize = font_sizes::Normal;
    float textWidth =
        render_backend::MeasureTextWithActiveFont(text.c_str(), fontSize);
    float textY = center.y - transform.size.y * 0.5f - 10.0f -
                  progress * 20.0f;
    render_backend::DrawTextWithActiveFont(
        text.c_str(), static_cast<int>(center.x - textWidth / 2.0f),
        static_cast<int>(textY), fontSize, gold);
  }
};
