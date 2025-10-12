#pragma once

#include "../components/can_drop_onto.h"
#include "../components/is_dish.h"
#include "../components/is_drop_slot.h"
#include "../components/is_held.h"
#include "../components/is_inventory_item.h"
#include "../components/is_shop_item.h"
#include "../components/transform.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../rl.h"
#include "../shop.h"
#include <afterhours/ah.h>
#include <limits>
#include <magic_enum/magic_enum.hpp>

using namespace afterhours;

struct DropWhenNoLongerHeld : System<IsHeld, Transform> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

private:
  bool can_drop_item(const Entity &entity, const IsDropSlot &drop_slot) {
    return (entity.has<IsInventoryItem>() &&
            drop_slot.accepts_inventory_items) ||
           (entity.has<IsShopItem>() && drop_slot.accepts_shop_items);
  }

  int get_slot_id(const Entity &entity) {
    if (entity.has<IsInventoryItem>()) {
      return entity.get<IsInventoryItem>().slot;
    } else if (entity.has<IsShopItem>()) {
      return entity.get<IsShopItem>().slot;
    }
    return -1;
  }

  void set_slot_id(Entity &entity, int slot_id) {
    if (entity.has<IsInventoryItem>()) {
      entity.get<IsInventoryItem>().slot = slot_id;
    } else if (entity.has<IsShopItem>()) {
      entity.get<IsShopItem>().slot = slot_id;
    }
  }

  Entity *find_item_in_slot(int slot_id, const Entity &exclude_entity) {
    for (auto &ref : EntityQuery().whereHasComponent<IsInventoryItem>().gen()) {
      auto &other_entity = ref.get();
      if (other_entity.get<IsInventoryItem>().slot == slot_id &&
          &other_entity != &exclude_entity) {
        return &other_entity;
      }
    }

    for (auto &ref : EntityQuery().whereHasComponent<IsShopItem>().gen()) {
      auto &other_entity = ref.get();
      if (other_entity.get<IsShopItem>().slot == slot_id &&
          &other_entity != &exclude_entity) {
        return &other_entity;
      }
    }
    return nullptr;
  }

  void snap_back_to_original(Entity &entity, const IsHeld &held) {
    entity.get<Transform>().position = held.original_position;
    entity.removeComponent<IsHeld>();
  }

  bool try_purchase_shop_item(Entity &entity, Entity *drop_slot) {
    if (!entity.has<IsShopItem>() ||
        !drop_slot->get<IsDropSlot>().accepts_inventory_items) {
      return true;
    }

    auto &dish = entity.get<IsDish>();
    auto wallet_entity = EntityHelper::get_singleton<Wallet>();

    if (!wallet_entity.get().has<Wallet>()) {
      return false;
    }

    auto &wallet = wallet_entity.get().get<Wallet>();
    int dish_price = get_dish_info(dish.type).price;

    if (wallet.gold < dish_price) {
      return false;
    }

    wallet.gold -= dish_price;
    entity.removeComponent<IsShopItem>();
    entity.addComponent<IsInventoryItem>();
    entity.get<IsInventoryItem>().slot = drop_slot->get<IsDropSlot>().slot_id;

    return true;
  }

  void drop_into_empty_slot(Entity &entity, Entity *drop_slot,
                            const Transform &transform, const IsHeld &held) {
    vec2 slot_center = drop_slot->get<Transform>().center();
    entity.get<Transform>().position = slot_center - transform.size * 0.5f;

    if (!try_purchase_shop_item(entity, drop_slot)) {
      snap_back_to_original(entity, held);
      return;
    }

    set_slot_id(entity, drop_slot->get<IsDropSlot>().slot_id);
    drop_slot->get<IsDropSlot>().occupied = true;
  }

  void swap_items(Entity &entity, Entity *occupied_slot,
                  const Transform &transform, const IsHeld &held) {
    Entity *item_in_slot =
        find_item_in_slot(occupied_slot->get<IsDropSlot>().slot_id, entity);
    if (!item_in_slot)
      return;

    if (entity.has<IsShopItem>() || item_in_slot->has<IsShopItem>()) {
      snap_back_to_original(entity, held);
      return;
    }

    int original_slot_id = get_slot_id(entity);
    int target_slot_id = occupied_slot->get<IsDropSlot>().slot_id;

    vec2 slot_center = occupied_slot->get<Transform>().center();
    entity.get<Transform>().position = slot_center - transform.size * 0.5f;
    item_in_slot->get<Transform>().position = held.original_position;

    set_slot_id(entity, target_slot_id);
    set_slot_id(*item_in_slot, original_slot_id);

    occupied_slot->get<IsDropSlot>().occupied = true;
    if (original_slot_id >= 0) {
      auto original_slot = EQ().whereHasComponent<IsDropSlot>()
                               .whereSlotID(original_slot_id)
                               .gen_first();
      if (original_slot) {
        original_slot->get<IsDropSlot>().occupied = true;
      }
    }
  }

public:
  void for_each_with(Entity &entity, IsHeld &held, Transform &transform,
                     float) override {
    if (!afterhours::input::is_mouse_button_released(
            raylib::MOUSE_BUTTON_LEFT)) {
      vec2 mouse_pos = afterhours::input::get_mouse_position();
      entity.get<Transform>().position =
          mouse_pos + held.offset - transform.size * 0.5f;
      return;
    }

    vec2 mouse_pos = afterhours::input::get_mouse_position();
    Rectangle mouse_rect = {mouse_pos.x, mouse_pos.y, 1, 1};

    Entity *best_drop_slot = nullptr;
    Entity *occupied_slot = nullptr;
    float best_distance = std::numeric_limits<float>::max();

    for (auto &ref : EQ().whereHasComponent<Transform>()
                         .whereHasComponent<CanDropOnto>()
                         .whereLambda([](const Entity &e) {
                           return e.get<CanDropOnto>().enabled;
                         })
                         .whereHasComponent<IsDropSlot>()
                         .whereLambda([&](const Entity &e) {
                           return can_drop_item(entity, e.get<IsDropSlot>());
                         })
                         .whereOverlaps(mouse_rect)
                         .gen()) {
      auto &slot_entity = ref.get();

      vec2 slot_center = slot_entity.get<Transform>().center();
      float distance =
          Vector2Distance(raylib::Vector2{mouse_pos.x, mouse_pos.y},
                          raylib::Vector2{slot_center.x, slot_center.y});

      if (slot_entity.get<IsDropSlot>().occupied) {
        occupied_slot = &slot_entity;
      } else if (distance < best_distance) {
        best_distance = distance;
        best_drop_slot = &slot_entity;
      }
    }

    if (best_drop_slot) {
      drop_into_empty_slot(entity, best_drop_slot, transform, held);
    } else if (occupied_slot) {
      swap_items(entity, occupied_slot, transform, held);
    } else {
      snap_back_to_original(entity, held);
    }

    entity.removeComponent<IsHeld>();
  }
};