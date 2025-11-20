#pragma once

#include "../components/is_draggable.h"
#include "../components/is_drink_shop_item.h"
#include "../components/render_order.h"
#include "../components/test_drink_shop_override.h"
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
using namespace afterhours::components;

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

    // Check for test override first
    bool use_override = false;
    std::vector<DrinkType> override_drinks;
    
    // Check for test override using singleton map directly
    try {
      const auto override_id = get_type_id<TestDrinkShopOverride>();
      auto &singleton_map = EntityHelper::get().singletonMap;
      auto it = singleton_map.find(override_id);
      if (it != singleton_map.end()) {
        auto &override_entity = *it->second;
        if (override_entity.has<TestDrinkShopOverride>()) {
          auto &override = override_entity.get<TestDrinkShopOverride>();
          if (!override.used && !override.drinks.empty()) {
            use_override = true;
            override_drinks = override.drinks;
            override.used = true; // Mark as used so it's only applied once
          }
        }
      }
    } catch (...) {
      // Override not available, use normal generation
    }

    // Get current shop tier (only needed if not using override)
    int current_tier = 1;
    if (!use_override) {
      auto shop_tier_entity = EntityHelper::get_singleton<ShopTier>();
      if (shop_tier_entity.get().has<ShopTier>()) {
        current_tier = shop_tier_entity.get().get<ShopTier>().current_tier;
      }
    }

    for (int i = 0; i < DRINK_SHOP_SLOTS; ++i) {
      auto position = calculate_slot_position(
          i, static_cast<int>(drink_shop_start_x), DRINK_SHOP_START_Y, 2);
      
      DrinkType drink_type;
      if (use_override && i < static_cast<int>(override_drinks.size())) {
        // Use override drink for this slot
        drink_type = override_drinks[i];
      } else if (use_override) {
        // Override list is shorter than slots, fill remaining with random
        drink_type = get_random_drink_for_tier(current_tier);
      } else {
        // Normal random generation
        drink_type = get_random_drink_for_tier(current_tier);
      }

      auto &e = EntityHelper::createEntity();
      e.addComponent<Transform>(position, vec2{SLOT_SIZE, SLOT_SIZE});
      e.addComponent<IsDrinkShopItem>(i, drink_type);
      e.addComponent<IsDraggable>(true);
      e.addComponent<HasRenderOrder>(RenderOrder::ShopItems,
                                     RenderScreen::Shop);

      DrinkInfo drink_info = get_drink_info(drink_type);
      const auto frame = afterhours::texture_manager::idx_to_sprite_frame(
          drink_info.sprite.i, drink_info.sprite.j);
      e.addComponent<afterhours::texture_manager::HasSprite>(
          position, vec2{SLOT_SIZE, SLOT_SIZE}, 0.f, frame,
          render_constants::kDishSpriteScale, raylib::WHITE);
    }

    if (use_override) {
      log_info("Generated drink shop with {} drinks (test override)", DRINK_SHOP_SLOTS);
    } else {
      log_info("Generated drink shop with {} drinks", DRINK_SHOP_SLOTS);
    }
  }
};
