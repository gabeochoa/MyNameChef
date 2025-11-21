#pragma once

#include "../font_info.h"
#include "../game_state_manager.h"
#include "../render_backend.h"
#include "../shop.h"
#include <afterhours/ah.h>

using namespace afterhours;

struct RenderShopPriceDisplaySystem : System<> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  virtual void once(float) const override {
    float font_size = font_sizes::Normal;
    float y_pos = SHOP_START_Y - font_size * 1.5f;
    render_backend::DrawTextWithActiveFont("Items: 3 gold each", SHOP_START_X,
                                           static_cast<int>(y_pos), font_size,
                                           raylib::WHITE);
  }
};
