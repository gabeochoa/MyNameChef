#pragma once

#include "../test_macros.h"
#include "../test_input.h"
#include "../../components/is_held.h"
#include "../../components/transform.h"
#include "../../components/is_draggable.h"
#include "../../query.h"
#include <afterhours/ah.h>

// Test that the test_input framework works correctly for drag-and-drop simulation
TEST(validate_test_input_framework) {
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_frames(10);
  
  // Find a shop item to test with
  afterhours::Entity *shop_item = nullptr;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsShopItem>()
           .gen()) {
    if (entity.has<IsDish>() && entity.has<Transform>() && entity.has<IsDraggable>()) {
      shop_item = &entity;
      break;
    }
  }
  
  app.expect_true(shop_item != nullptr, "Found a shop item to test with");
  
  if (!shop_item) {
    return;
  }
  
  afterhours::EntityID shop_item_id = shop_item->id;
  vec2 shop_item_pos = shop_item->get<Transform>().center();
  
  // Wait for entity to be merged (MarkIsHeldWhenHeld queries without force_merge)
  for (int i = 0; i < 20; ++i) {
    app.wait_for_frames(1);
    afterhours::OptEntity merged_opt =
        afterhours::EntityQuery()
            .whereID(shop_item_id)
            .gen_first();
    if (merged_opt.has_value()) {
      const Transform &t = merged_opt.asE().get<Transform>();
      shop_item_pos = t.center();
      // Ensure mouse position is within entity bounds (not just at center)
      Rectangle entity_rect = t.rect();
      shop_item_pos.x = std::max(entity_rect.x + 1.0f, 
                                 std::min(entity_rect.x + entity_rect.width - 1.0f, 
                                          shop_item_pos.x));
      shop_item_pos.y = std::max(entity_rect.y + 1.0f, 
                                 std::min(entity_rect.y + entity_rect.height - 1.0f, 
                                          shop_item_pos.y));
      break;
    }
  }
  
  // Test 1: Simulate mouse press - item should be marked as held
  // Set mouse position first, then wait a frame, then simulate press
  test_input::set_mouse_position(shop_item_pos);
  app.wait_for_frames(1); // Let mouse position be set
  test_input::simulate_mouse_button_press(raylib::MOUSE_BUTTON_LEFT);
  app.wait_for_frames(3); // Let MarkIsHeldWhenHeld process
  
  // Verify item is held
  afterhours::OptEntity held_item_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(shop_item_id)
          .gen_first();
  app.expect_true(held_item_opt.has_value(), "Shop item still exists");
  app.expect_true(held_item_opt.asE().has<IsHeld>(), "Shop item is marked as held");
  
  // Test 2: Simulate mouse release - item should no longer be held
  test_input::simulate_mouse_button_release(raylib::MOUSE_BUTTON_LEFT);
  app.wait_for_frames(3); // Let DropWhenNoLongerHeld process
  
  // Verify item is no longer held
  afterhours::OptEntity released_item_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(shop_item_id)
          .gen_first();
  app.expect_true(released_item_opt.has_value(), "Shop item still exists after release");
  app.expect_false(released_item_opt.asE().has<IsHeld>(), "Shop item is no longer held");
  
  // Clean up
  test_input::clear_simulated_input();
  
  log_info("TEST_INPUT_FRAMEWORK: All tests passed");
}

