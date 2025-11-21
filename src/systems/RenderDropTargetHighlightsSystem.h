#pragma once

#include "../components/dish_level.h"
#include "../components/is_dish.h"
#include "../components/is_drop_slot.h"
#include "../components/is_held.h"
#include "../components/is_inventory_item.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../shop.h"
#include <afterhours/ah.h>

using namespace afterhours;

struct RenderDropTargetHighlightsSystem : System<> {
  virtual bool should_run(float) override {
    GameStateManager &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  void once(float) const override {
    auto dragged_item_opt = EQ().whereHasComponent<IsHeld>().gen_first();
    if (!dragged_item_opt) {
      return;
    }

    Entity &dragged_item = dragged_item_opt.asE();

    if (!dragged_item.has<IsDish>() || !dragged_item.has<DishLevel>()) {
      return;
    }

    DishType dragged_type = dragged_item.get<IsDish>().type;
    int dragged_level = dragged_item.get<DishLevel>().level;

    for (Entity &slot : EQ().whereHasComponent<IsDropSlot>()
                            .whereHasComponent<Transform>()
                            .gen()) {
      IsDropSlot &drop_slot = slot.get<IsDropSlot>();
      Transform &slot_transform = slot.get<Transform>();
      raylib::Color highlight_color;

      bool should_highlight = false;

      if (drop_slot.slot_id == SELL_SLOT_ID) {
        should_highlight = true;
        highlight_color = raylib::Color{200, 100, 100, 100};
      } else if (drop_slot.slot_id < SELL_SLOT_ID && !drop_slot.occupied) {
        should_highlight = true;
        highlight_color = raylib::Color{100, 200, 100, 100};
      } else if (drop_slot.occupied && drop_slot.slot_id < SELL_SLOT_ID) {
        for (Entity &inv_item :
             EQ().whereHasComponent<IsInventoryItem>().gen()) {
          IsInventoryItem &inv_slot = inv_item.get<IsInventoryItem>();

          if (inv_slot.slot == drop_slot.slot_id && inv_item.has<IsDish>() &&
              inv_item.has<DishLevel>()) {
            DishType inv_type = inv_item.get<IsDish>().type;
            int inv_level = inv_item.get<DishLevel>().level;

            if (inv_type == dragged_type && inv_level >= dragged_level) {
              should_highlight = true;
              highlight_color = raylib::Color{200, 200, 100, 100};
              break;
            }
          }
        }
      }

      if (should_highlight) {
        raylib::DrawRectangle(static_cast<int>(slot_transform.position.x),
                              static_cast<int>(slot_transform.position.y),
                              static_cast<int>(slot_transform.size.x),
                              static_cast<int>(slot_transform.size.y),
                              highlight_color);
      }
    }
  }
};
