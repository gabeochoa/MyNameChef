#pragma once

#include "../components/render_order.h"
#include "../components/toast_message.h"
#include "../components/transform.h"
#include "../font_info.h"
#include "../game_state_manager.h"
#include "../render_backend.h"
#include "../ui/text_formatting.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/color.h>

struct RenderToastSystem
    : afterhours::System<ToastMessage, Transform, HasColor, HasRenderOrder> {
  virtual bool should_run(float) override {
    return !render_backend::is_headless_mode;
  }

  virtual void
  for_each_with(const afterhours::Entity &entity, const ToastMessage &toast,
                const Transform &transform, const HasColor &hasColor,
                const HasRenderOrder &renderOrder, float) const override {
    auto &gsm = GameStateManager::get();
    RenderScreen current_screen = get_current_render_screen(gsm);

    if (!renderOrder.should_render_on_screen(current_screen)) {
      return;
    }

    raylib::Rectangle rect{transform.position.x, transform.position.y,
                           transform.size.x, transform.size.y};

    raylib::Color bgColor = hasColor.color();
    render_backend::DrawRectangleRounded(rect, 0.3f, 8, bgColor);

    const float fontSize = font_sizes::Medium;
    float textWidth = render_backend::MeasureTextWithActiveFont(toast.message.c_str(), fontSize);
    float textX = transform.position.x + (transform.size.x - textWidth) / 2.0f;
    float textY = transform.position.y + (transform.size.y - fontSize) / 2.0f;

    raylib::Color textColor = text_formatting::TextFormatting::get_color(
        text_formatting::SemanticColor::Text,
        text_formatting::FormattingContext::UI);
    textColor.a = bgColor.a;

    render_backend::DrawTextWithActiveFont(toast.message.c_str(), static_cast<int>(textX),
                                           static_cast<int>(textY), fontSize, textColor);
  }

private:
  RenderScreen get_current_render_screen(const GameStateManager &gsm) const {
    return static_cast<RenderScreen>(
        GameStateManager::render_screen_for(gsm.active_screen));
  }
};
