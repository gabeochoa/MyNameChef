#pragma once

#include "../components/drink_pairing.h"
#include "../components/is_dish.h"
#include "../components/is_drink_shop_item.h"
#include "../components/is_held.h"
#include "../components/is_inventory_item.h"
#include "../components/transform.h"
#include "../drink_types.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../rl.h"
#include "../shop.h"
#include <afterhours/ah.h>

using namespace afterhours;

struct HandleDrinkDrop : System<IsHeld, Transform, IsDrinkShopItem> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  void for_each_with(Entity &drink_entity, IsHeld &held, Transform &transform,
                     IsDrinkShopItem &drink_shop_item, float) override {
    if (!afterhours::input::is_mouse_button_released(
            raylib::MOUSE_BUTTON_LEFT)) {
      return;
    }

    vec2 mouse_pos = afterhours::input::get_mouse_position();
    raylib::Vector2 mouse_vec{mouse_pos.x, mouse_pos.y};

    for (Entity &dish :
         EQ().whereHasComponent<IsDish>()
             .whereHasComponent<IsInventoryItem>()
             .whereHasComponent<Transform>()
             // TODO make whereMousOn
             .whereLambda([mouse_vec](const Entity &dish) {
               const Transform &dish_transform = dish.get<Transform>();
               Rectangle dish_rect = dish_transform.rect();
               return CheckCollisionPointRec(
                   mouse_vec, Rectangle{dish_rect.x, dish_rect.y,
                                        dish_rect.width, dish_rect.height});
             })
             .gen()) {

      // TODO: Add visual highlight feedback when dragging drink over valid dish

      if (!wallet_charge(1)) {
        make_toast("Not enough gold!");
        snap_back(drink_entity, held);
        return;
      }

      DrinkPairing &drink_pairing = dish.addComponentIfMissing<DrinkPairing>();
      drink_pairing.drink = drink_shop_item.drink_type;

      DrinkInfo drink_info = get_drink_info(drink_shop_item.drink_type);
      make_toast("Applied " + drink_info.name + "!");
      empty_originating_slot(drink_shop_item.slot);

      drink_entity.removeComponent<IsHeld>();
      drink_entity.cleanup = true;
      return;
    }

    snap_back(drink_entity, held);
  }

private:
  void snap_back(Entity &entity, const IsHeld &held) {
    entity.get<Transform>().position = held.original_position;
    entity.removeComponent<IsHeld>();
  }

  void empty_originating_slot(int slot_id) {
    if (slot_id >= 0) {
      if (auto original_slot_entity = EQ().whereHasComponent<IsDropSlot>()
                                          .whereSlotID(slot_id)
                                          .gen_first()) {
        original_slot_entity->get<IsDropSlot>().occupied = false;
      }
    }
  }
};
