#pragma once

#include "../components/has_tooltip.h"
#include "../components/is_dish.h"
#include "../components/is_gallery_item.h"
#include "../components/render_order.h"
#include "../components/transform.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../render_constants.h"
#include "../shop.h" // for SLOT_SIZE, SLOT_GAP
#include "../tooltip.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/texture_manager.h>

struct GenerateDishesGallery : afterhours::System<> {
  virtual bool should_run(float) override { return true; }

  virtual void once(float) override {
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Dishes)
      return;

    // If already generated, skip
    for (auto &ref : afterhours::EntityQuery({.force_merge = true}).gen()) {
      if (ref.get().has<IsGalleryItem>()) {
        return;
      }
    }

    const int start_x = 100;
    const int start_y = 120;
    const int cols = 10;
    const int icon_size = SLOT_SIZE;
    const int gap = SLOT_GAP;

    int tier_counts[6] = {0, 0, 0, 0, 0, 0};
    const auto &pool = get_default_dish_pool();
    for (auto dish : pool) {
      auto info = get_dish_info(dish);
      int t = std::clamp(info.tier, 1, 5);
      int index_in_tier = tier_counts[t]++;
      int row = t - 1;
      int col = index_in_tier % cols;
      int extra_row = index_in_tier / cols;

      float x = static_cast<float>(start_x + col * (icon_size + gap));
      float y = static_cast<float>(start_y + (row * (icon_size + 3 * gap)) +
                                   extra_row * (icon_size + gap));

      auto &e = afterhours::EntityHelper::createEntity();
      e.addComponent<Transform>(
          afterhours::vec2{x, y},
          afterhours::vec2{(float)icon_size, (float)icon_size});
      e.addComponent<IsDish>(dish);
      e.addComponent<IsGalleryItem>();
      e.addComponent<HasRenderOrder>(RenderOrder::UI, RenderScreen::All);

      auto frame = afterhours::texture_manager::idx_to_sprite_frame(
          info.sprite.i, info.sprite.j);
      e.addComponent<afterhours::texture_manager::HasSprite>(
          afterhours::vec2{x, y},
          afterhours::vec2{(float)icon_size, (float)icon_size}, 0.f, frame,
          render_constants::kDishSpriteScale, raylib::WHITE);
      e.addComponent<HasTooltip>(generate_dish_tooltip(dish));
    }

    afterhours::EntityHelper::merge_entity_arrays();
  }
};
