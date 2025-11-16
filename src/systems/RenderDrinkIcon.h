#pragma once

#include "../components/drink_pairing.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../components/is_shop_item.h"
#include "../components/transform.h"
#include "../drink_types.h"
#include "../game_state_manager.h"
#include "../log.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/texture_manager.h>

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

    // TODO run this in once instead so
    // Get spritesheet
    auto *spritesheet_component = afterhours::EntityHelper::get_singleton_cmp<
        afterhours::texture_manager::HasSpritesheet>();
    if (!spritesheet_component) {
      log_error("RenderDrinkIcon: spritesheet component not found");
      return;
    }

    raylib::Texture2D sheet = spritesheet_component->texture;

    // Get drink sprite frame
    DrinkType drink_type = drink_pairing.drink.value();
    DrinkInfo drink_info = get_drink_info(drink_type);
    const auto frame = afterhours::texture_manager::idx_to_sprite_frame(
        drink_info.sprite.i, drink_info.sprite.j);

    const float icon_size = 24.0f;
    const float padding = 5.0f;

    const float dest_x = transform.position.x + padding;
    const float dest_y =
        transform.position.y + transform.size.y - icon_size - padding;

    // Draw the drink sprite
    raylib::DrawTexturePro(
        sheet, frame, raylib::Rectangle{dest_x, dest_y, icon_size, icon_size},
        raylib::Vector2{0, 0}, 0.f, raylib::WHITE);
  }
};
