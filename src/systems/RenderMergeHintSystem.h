#pragma once

#include "../components/is_dish.h"
#include "../components/is_held.h"
#include "../components/is_inventory_item.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../merge.h"
#include "../query.h"
#include "../render_backend.h"
#include "../rl.h"
#include <afterhours/ah.h>

struct RenderMergeHintSystem : afterhours::System<> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  void once(float) const override {
    auto held_opt = EQ().whereHasComponent<IsHeld>().gen_first();
    if (held_opt) {
      return;
    }
    for (auto &ref : EQ().whereHasComponent<IsDish>()
                          .whereHasComponent<IsInventoryItem>()
                          .whereHasComponent<Transform>()
                          .gen()) {
      afterhours::Entity &dish = ref.get();
      if (!merge::dish_has_merge_partner(dish)) {
        continue;
      }
      const auto &transform = dish.get<Transform>();
      raylib::Rectangle outline = {transform.position.x - 3,
                                   transform.position.y - 3,
                                   transform.size.x + 6,
                                   transform.size.y + 6};
      render_backend::DrawRectangleLinesEx(
          outline, 3.0f, raylib::Color{255, 215, 0, 255});
    }
  }
};
