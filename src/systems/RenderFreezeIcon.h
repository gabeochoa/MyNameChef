#pragma once

#include "../components/is_shop_item.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../shop.h"
#include "../texture_library.h"
#include <afterhours/ah.h>

struct RenderFreezeIcon
    : afterhours::System<IsShopItem, Transform, Freezeable> {

  virtual bool should_run(float) override {
    GameStateManager &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  virtual void for_each_with(const afterhours::Entity &, const IsShopItem &,
                             const Transform &transform,
                             const Freezeable &freezeable,
                             float) const override {
    // TODO: Replace dollar_sign texture with appropriate freeze icon texture
    const raylib::Texture2D &tex = TextureLibrary::get().get("dollar_sign");

    if (freezeable.isFrozen) {
      // When frozen, show large icon covering the whole dish (like surrounded
      // by ice)
      const float icon_size = transform.size.x * 1.2f;
      const float dest_x =
          transform.position.x + (transform.size.x - icon_size) * 0.5f;
      const float dest_y =
          transform.position.y + (transform.size.y - icon_size) * 0.5f;

      raylib::DrawTexturePro(
          tex,
          raylib::Rectangle{0, 0, static_cast<float>(tex.width),
                            static_cast<float>(tex.height)},
          raylib::Rectangle{dest_x, dest_y, icon_size, icon_size},
          afterhours::vec2{0, 0}, 0.f, raylib::WHITE);
    } else {
      // When not frozen, show small icon in bottom-left corner
      const float icon_size = 24.0f;
      const float padding = 5.0f;

      const float dest_x = transform.position.x + padding;
      const float dest_y =
          transform.position.y + transform.size.y - icon_size - padding;

      raylib::DrawTexturePro(
          tex,
          raylib::Rectangle{0, 0, static_cast<float>(tex.width),
                            static_cast<float>(tex.height)},
          raylib::Rectangle{dest_x, dest_y, icon_size, icon_size},
          afterhours::vec2{0, 0}, 0.f, raylib::WHITE);
    }
  }
};
