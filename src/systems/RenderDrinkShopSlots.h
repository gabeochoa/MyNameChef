#pragma once

#include "../game_state_manager.h"
#include "../shop.h"
#include <afterhours/ah.h>
#include <raylib/raylib.h>

struct RenderDrinkShopSlots : afterhours::System<> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  void once(float) const override {
    float screen_width = static_cast<float>(raylib::GetScreenWidth());
    float drink_shop_start_x =
        screen_width - (2 * (SLOT_SIZE + SLOT_GAP)) - 50.0f;

    // Draw background for each drink slot
    for (int i = 0; i < DRINK_SHOP_SLOTS; ++i) {
      auto position = calculate_slot_position(
          i, static_cast<int>(drink_shop_start_x), DRINK_SHOP_START_Y, 2);

      // Draw a subtle background rectangle for the slot
      raylib::Color bg_color =
          raylib::Color{60, 60, 80, 200}; // Dark blue-gray with transparency
      raylib::DrawRectangle(
          static_cast<int>(position.x), static_cast<int>(position.y),
          static_cast<int>(SLOT_SIZE), static_cast<int>(SLOT_SIZE), bg_color);

      // Draw a border to make the slot more visible
      raylib::Color border_color =
          raylib::Color{100, 100, 120, 255}; // Lighter border
      raylib::DrawRectangleLines(static_cast<int>(position.x),
                                 static_cast<int>(position.y),
                                 static_cast<int>(SLOT_SIZE),
                                 static_cast<int>(SLOT_SIZE), border_color);
    }
  }
};
