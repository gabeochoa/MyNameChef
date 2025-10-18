#pragma once

#include "../components/dish_level.h"
#include "../components/is_dish.h"
#include "../components/is_draggable.h"
#include "../components/is_drop_slot.h"
#include "../components/is_inventory_item.h"
#include "../components/render_order.h"
#include "../components/transform.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../render_constants.h"
#include "../shop.h"
#include <afterhours/ah.h>

using namespace afterhours;

struct GenerateInventorySlots : System<> {
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.is_game_active();
  }

  void once(float) override {
    auto &gsm = GameStateManager::get();

    // Generate inventory slots when entering shop screen
    if (gsm.active_screen == GameStateManager::Screen::Shop &&
        last_screen != GameStateManager::Screen::Shop) {

      // Check if inventory slots already exist
      bool inventory_slots_exist = false;
      for (auto &ref : afterhours::EntityQuery()
                           .template whereHasComponent<IsDropSlot>()
                           .gen()) {
        const auto &drop_slot = ref.get().template get<IsDropSlot>();
        if (drop_slot.accepts_inventory_items && drop_slot.accepts_shop_items) {
          inventory_slots_exist = true;
          break;
        }
      }

      // Only create inventory slots if they don't exist
      if (!inventory_slots_exist) {
        for (int i = 0; i < INVENTORY_SLOTS; ++i) {
          auto position = calculate_inventory_position(i);
          make_drop_slot(INVENTORY_SLOT_OFFSET + i, position,
                         vec2{SLOT_SIZE, SLOT_SIZE}, true, true);
          // TODO: Add UI labels to inventory slots for better testability
          // Expected: Each inventory slot should have a UI label like "Inventory Slot 1", "Inventory Slot 2", etc.
          // This would allow UITestHelpers::visible_ui_exists("Inventory Slot 1") to work
        }

        // Create a sell drop slot to the right of inventory
        {
          vec2 pos{static_cast<float>(SELL_SLOT_X),
                   static_cast<float>(SELL_SLOT_Y)};
          make_drop_slot(SELL_SLOT_ID, pos, vec2{SLOT_SIZE, SLOT_SIZE}, true,
                         false);
        }

        EntityHelper::merge_entity_arrays();
        log_info("Generated inventory slots for shop screen");
      }
    }

    // Add test dishes directly to the inventory using real DishType values
    if (gsm.active_screen == GameStateManager::Screen::Shop &&
        last_screen != GameStateManager::Screen::Shop) {

      // Check if test dishes already exist
      bool test_dishes_exist = false;
      for (auto &ref : afterhours::EntityQuery()
                           .template whereHasComponent<IsInventoryItem>()
                           .gen()) {
        (void)ref; // Suppress unused variable warning
        test_dishes_exist = true;
        break;
      }

      // Only create test dishes if they don't exist
      if (!test_dishes_exist) {
        struct TestDishSpawn {
          DishType dish_type;
          int target_inventory_slot;
        };

        std::vector<TestDishSpawn> spawnList = {
            {DishType::Bagel, INVENTORY_SLOT_OFFSET + 0},
            {DishType::Salmon, INVENTORY_SLOT_OFFSET + 1},
            {DishType::Salmon, INVENTORY_SLOT_OFFSET + 2},
            {DishType::Bagel, INVENTORY_SLOT_OFFSET + 3},
        };

        for (auto &spawn : spawnList) {
          auto position = calculate_inventory_position(
              spawn.target_inventory_slot - INVENTORY_SLOT_OFFSET);

          afterhours::Entity &dish = afterhours::EntityHelper::createEntity();

          // Add core dish components
          dish.addComponent<Transform>(position, vec2{SLOT_SIZE, SLOT_SIZE});
          dish.addComponent<IsDish>(spawn.dish_type);
          dish.addComponent<DishLevel>(1);
          dish.addComponent<IsInventoryItem>();
          dish.get<IsInventoryItem>().slot = spawn.target_inventory_slot;
          dish.addComponent<IsDraggable>(true);
          dish.addComponent<HasRenderOrder>(RenderOrder::ShopItems,
                                            RenderScreen::Shop);

          // Add sprite using dish atlas grid indices
          auto dish_info = get_dish_info(spawn.dish_type);
          const auto frame = afterhours::texture_manager::idx_to_sprite_frame(
              dish_info.sprite.i, dish_info.sprite.j);
          dish.addComponent<afterhours::texture_manager::HasSprite>(
              position, vec2{SLOT_SIZE, SLOT_SIZE}, 0.f, frame,
              render_constants::kDishSpriteScale,
              raylib::Color{255, 255, 255, 255});

          // Mark the corresponding slot as occupied
          for (auto &ref : afterhours::EntityQuery()
                               .template whereHasComponent<IsDropSlot>()
                               .gen()) {
            auto &slot_entity = ref.get();
            if (slot_entity.get<IsDropSlot>().slot_id ==
                spawn.target_inventory_slot) {
              slot_entity.get<IsDropSlot>().occupied = true;
              break;
            }
          }
        }

        EntityHelper::merge_entity_arrays();
        log_info("Created test dishes in inventory slots");
      }
    }

    last_screen = gsm.active_screen;
  }
};
