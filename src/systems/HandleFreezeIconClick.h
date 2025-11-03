#pragma once

#include "../components/is_shop_item.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../rl.h"
#include "../shop.h"
#include <afterhours/ah.h>

struct HandleFreezeIconClick
    : afterhours::System<IsShopItem, Transform, Freezeable> {

  virtual bool should_run(float) override {
    GameStateManager &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  virtual void for_each_with(afterhours::Entity &, IsShopItem &,
                             Transform &transform, Freezeable &freezeable,
                             float) override {
    if (!afterhours::input::is_mouse_button_pressed(
            raylib::MOUSE_BUTTON_LEFT)) {
      return;
    }

    vec2 mouse_pos = afterhours::input::get_mouse_position();
    const float icon_size = 24.0f;
    const float padding = 5.0f;

    const float icon_x = transform.position.x + padding;
    const float icon_y =
        transform.position.y + transform.size.y - icon_size - padding;
    const Rectangle icon_rect = {icon_x, icon_y, icon_size, icon_size};

    bool mouse_in_icon = CheckCollisionPointRec(
        raylib::Vector2{mouse_pos.x, mouse_pos.y}, icon_rect);

    if (!mouse_in_icon) {
      return;
    }

    freezeable.isFrozen = !freezeable.isFrozen;
  }
};
