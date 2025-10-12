#pragma once

#include "../components/has_tooltip.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../components/render_order.h"
#include "../components/transform.h"
#include "../dish_types.h"
#include "../shop.h"
#include "../tooltip.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/color.h>
#include <afterhours/src/plugins/texture_manager.h>
#include <magic_enum/magic_enum.hpp>

using namespace afterhours;

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
    // Attach sprite using dish atlas grid indices
    {
      const auto frame = afterhours::texture_manager::idx_to_sprite_frame(
          dish_info.sprite.i, dish_info.sprite.j);
      entity.addComponent<afterhours::texture_manager::HasSprite>(
          position, vec2{SLOT_SIZE, SLOT_SIZE}, 0.f, frame, 2.0F,
          raylib::Color{255, 255, 255, 255});
    }
    entity.addComponent<HasTooltip>(generate_dish_tooltip(dish_type));
    entity.get<IsInventoryItem>().slot = INVENTORY_SLOT_OFFSET;

    log_info("Added starter dish: {} to inventory",
             magic_enum::enum_name(dish_type));
  }
};
