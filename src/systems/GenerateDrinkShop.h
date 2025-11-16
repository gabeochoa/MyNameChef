#pragma once

#include "../components/is_draggable.h"
#include "../components/is_drink_shop_item.h"
#include "../components/render_order.h"
#include "../components/transform.h"
#include "../drink_types.h"
#include "../game_state_manager.h"
#include "../log.h"
#include "../query.h"
#include "../render_constants.h"
#include "../rl.h"
#include "../shop.h"
#include "../texture_library.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/texture_manager.h>

using namespace afterhours;

struct GenerateDrinkShop : System<> {
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.is_game_active();
  }

  void once(float) override {
    auto &gsm = GameStateManager::get();

    // Generate drink shop when entering shop screen
    if (gsm.active_screen == GameStateManager::Screen::Shop &&
        last_screen != GameStateManager::Screen::Shop) {

      bool drink_shop_exists =
          EntityQuery().whereHasComponent<IsDrinkShopItem>().has_values();

      if (!drink_shop_exists) {
        generate_drink_shop();
      }
    }

    last_screen = gsm.active_screen;
  }

private:
  void generate_drink_shop() {
    float screen_width = static_cast<float>(raylib::GetScreenWidth());
    float drink_shop_start_x =
        screen_width - (2 * (SLOT_SIZE + SLOT_GAP)) - 50.0f;

    for (int i = 0; i < DRINK_SHOP_SLOTS; ++i) {
      auto position = calculate_slot_position(
          i, static_cast<int>(drink_shop_start_x), DRINK_SHOP_START_Y, 2);
      DrinkType drink_type = get_random_drink();

      auto &e = EntityHelper::createEntity();
      e.addComponent<Transform>(position, vec2{SLOT_SIZE, SLOT_SIZE});
      e.addComponent<IsDrinkShopItem>(i, drink_type);
      e.addComponent<IsDraggable>(true);
      e.addComponent<HasRenderOrder>(RenderOrder::ShopItems,
                                     RenderScreen::Shop);

      // Water sprite is at column 7, row 5 in the spritesheet (beverages row)
      const auto frame = afterhours::texture_manager::idx_to_sprite_frame(7, 5);
      e.addComponent<afterhours::texture_manager::HasSprite>(
          position, vec2{SLOT_SIZE, SLOT_SIZE}, 0.f, frame,
          render_constants::kDishSpriteScale, raylib::WHITE);
    }

    log_info("Generated drink shop with {} drinks", DRINK_SHOP_SLOTS);
  }
};
