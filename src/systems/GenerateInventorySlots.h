#pragma once

#include "../components/has_tooltip.h"
#include "../components/is_dish.h"
#include "../components/is_drop_slot.h"
#include "../components/is_inventory_item.h"
#include "../components/render_order.h"
#include "../components/transform.h"
#include "../dish_types.h"
#include "../shop.h"
#include "../tooltip.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/color.h>
#include <magic_enum/magic_enum.hpp>

using namespace afterhours;

struct ShopItemColor : HasColor {
  ShopItemColor(raylib::Color color) : HasColor(color) {}
};

struct GenerateInventorySlots : System<> {
  bool initialized = false;

  void once(float) override {
    if (initialized)
      return;
    initialized = true;

    for (int i = 0; i < INVENTORY_SLOTS; ++i) {
      auto position = calculate_inventory_position(i);
      make_drop_slot(INVENTORY_SLOT_OFFSET + i, position,
                     vec2{SLOT_SIZE, SLOT_SIZE}, true, true);
    }

    add_starter_inventory_item();
    EntityHelper::merge_entity_arrays();
  }

private:
  void add_starter_inventory_item() {
    auto &entity = EntityHelper::createEntity();
    auto position = calculate_inventory_position(0);
    auto dish_type = get_random_dish();
    auto dish_info = get_dish_info(dish_type);

    entity.addComponent<Transform>(position, vec2{SLOT_SIZE, SLOT_SIZE});
    entity.addComponent<IsDish>(dish_type);
    entity.addComponent<IsInventoryItem>();
    entity.addComponent<HasRenderOrder>(RenderOrder::InventoryItems);
    entity.addComponent<ShopItemColor>(dish_info.color);
    entity.addComponent<HasTooltip>(generate_dish_tooltip(dish_type));
    entity.get<IsInventoryItem>().slot = INVENTORY_SLOT_OFFSET;

    log_info("Added starter dish: {} to inventory",
             magic_enum::enum_name(dish_type));
  }
};
