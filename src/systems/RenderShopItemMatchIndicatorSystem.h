#pragma once

#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../components/is_shop_item.h"
#include "../components/transform.h"
#include "../font_info.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../render_backend.h"
#include <afterhours/ah.h>

using namespace afterhours;

struct RenderShopItemMatchIndicatorSystem
    : System<IsShopItem, Transform, IsDish> {
  virtual bool should_run(float) override {
    GameStateManager &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  void for_each_with(const Entity &, const IsShopItem &,
                      const Transform &transform, const IsDish &dish,
                      float) const override {
    DishType shop_type = dish.type;

    bool has_match = false;
    for (auto &inv_ref : EQ().whereHasComponent<IsInventoryItem>().gen()) {
      Entity &inv_item = inv_ref.get();
      if (inv_item.has<IsDish>()) {
        DishType inv_type = inv_item.get<IsDish>().type;
        if (inv_type == shop_type) {
          has_match = true;
          break;
        }
      }
    }

    if (has_match) {
      float font_size = font_sizes::Small;
      float star_width =
          render_backend::MeasureTextWithActiveFont("⭐", font_size);
      float padding = font_size * 0.25f;
      float x = transform.position.x + transform.size.x - star_width - padding;
      float y = transform.position.y + padding;
      render_backend::DrawTextWithActiveFont(
          "⭐", static_cast<int>(x), static_cast<int>(y), font_size,
          raylib::Color{255, 215, 0, 255});
    }
  }
};

