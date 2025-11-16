#pragma once

#include "../components/drink_pairing.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../components/is_shop_item.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../texture_library.h"
#include <afterhours/ah.h>

struct RenderDrinkIcon : afterhours::System<IsDish, Transform, DrinkPairing> {
  virtual bool should_run(float) override {
    GameStateManager &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  virtual void for_each_with(const afterhours::Entity &entity, const IsDish &,
                             const Transform &transform,
                             const DrinkPairing &drink_pairing,
                             float) const override {
    if (!drink_pairing.drink.has_value()) {
      return;
    }

    if (!entity.has<IsInventoryItem>() && !entity.has<IsShopItem>()) {
      return;
    }

    const raylib::Texture2D &tex = TextureLibrary::get().get("dollar_sign");

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
};
