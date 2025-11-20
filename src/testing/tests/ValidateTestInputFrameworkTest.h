#pragma once

#include "../test_macros.h"
#include "../test_input.h"
#include "../../components/is_held.h"
#include "../../components/transform.h"
#include "../../components/is_draggable.h"
#include "../../log.h"
#include "../../query.h"
#include <afterhours/ah.h>
#include <source_location>

// Test that the test_input framework works correctly for drag-and-drop simulation
TEST(validate_test_input_framework) {
  const TestOperationID setup_stage =
      TestApp::generate_operation_id(std::source_location::current(),
                                     "validate_test_input_framework.setup_stage");
  if (!app.completed_operations.count(setup_stage)) {
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
  
  // CRITICAL: Verify the entity is queryable by MarkIsHeldWhenHeld system (without force_merge)
  bool system_can_find_entity = false;
  for (int check = 0; check < 10; ++check) {
    app.wait_for_frames(1);
    for (afterhours::Entity &entity :
         afterhours::EntityQuery() // No force_merge, like MarkIsHeldWhenHeld uses
             .whereHasComponent<IsDraggable>()
             .whereHasComponent<Transform>()
             .gen()) {
      if (entity.id == shop_item_id) {
        system_can_find_entity = true;
        // Update position from the entity the system can see
        const Transform &t = entity.get<Transform>();
        shop_item_pos = t.center();
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
    if (system_can_find_entity) {
      break;
    }
  }
  
    app.expect_true(system_can_find_entity, "Shop item found by system query (without force_merge)");
    if (!system_can_find_entity) {
      test_input::clear_simulated_input();
      return;
    }
    
    // Store shop item ID for later stages
    app.set_test_int("validate_test_input_framework.shop_item_id", shop_item_id);
    app.completed_operations.insert(setup_stage);
    return;
  }
  
  // Retrieve shop item ID from stored data
  auto shop_item_id_opt = app.get_test_int("validate_test_input_framework.shop_item_id");
  if (!shop_item_id_opt.has_value()) {
    app.fail("Missing stored shop item ID");
    return;
  }
  afterhours::EntityID shop_item_id = static_cast<afterhours::EntityID>(shop_item_id_opt.value());
  
  // Get shop item position
  afterhours::OptEntity shop_item_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(shop_item_id)
          .gen_first();
  if (!shop_item_opt.has_value()) {
    app.fail("Shop item not found");
    return;
  }
  vec2 shop_item_pos = shop_item_opt.asE().get<Transform>().center();
  Rectangle entity_rect = shop_item_opt.asE().get<Transform>().rect();
  shop_item_pos.x = std::max(entity_rect.x + 1.0f, 
                             std::min(entity_rect.x + entity_rect.width - 1.0f, 
                                      shop_item_pos.x));
  shop_item_pos.y = std::max(entity_rect.y + 1.0f, 
                             std::min(entity_rect.y + entity_rect.height - 1.0f, 
                                      shop_item_pos.y));
  
  const TestOperationID press_stage =
      TestApp::generate_operation_id(std::source_location::current(),
                                     "validate_test_input_framework.press_stage");
  if (!app.completed_operations.count(press_stage)) {
    // Test 1: Simulate mouse press - item should be marked as held
  // Set mouse position first, then wait a frame, then simulate press
  test_input::set_mouse_position(shop_item_pos);
  app.wait_for_frames(1); // Let mouse position be set
  test_input::simulate_mouse_button_press(raylib::MOUSE_BUTTON_LEFT);
  app.wait_for_frames(5); // Let MarkIsHeldWhenHeld process (flag persists until consumed)
  
  // Verify item is held - poll for a few frames since the system might not have run yet
  // The system marks it as held, but we need to wait for it to be visible in queries
  bool is_held = false;
  for (int check = 0; check < 20; ++check) {
    app.wait_for_frames(1);
    afterhours::OptEntity check_opt =
        afterhours::EntityQuery({.force_merge = true})
            .whereID(shop_item_id)
            .gen_first();
    if (check_opt.has_value() && check_opt.asE().has<IsHeld>()) {
      is_held = true;
      log_error("TEST_INPUT_FRAMEWORK: Item {} is held after {} checks", shop_item_id, check + 1);
      break;
    }
  }
  
  if (!is_held) {
    // Final check - maybe the entity ID changed or something else happened
    int held_count = 0;
    for (afterhours::Entity &e :
         afterhours::EntityQuery({.force_merge = true})
             .whereHasComponent<IsHeld>()
             .gen()) {
      held_count++;
      if (e.id == shop_item_id) {
        is_held = true;
        log_error("TEST_INPUT_FRAMEWORK: Found held item {} in separate query", shop_item_id);
        break;
      }
    }
    log_error("TEST_INPUT_FRAMEWORK: After polling, is_held={}, held_count={}, shop_item_id={}", is_held, held_count, shop_item_id);
  }
  
    app.expect_true(is_held, "Shop item is marked as held");
    if (!is_held) {
      test_input::clear_simulated_input();
      return;
    }
    
    app.completed_operations.insert(press_stage);
    return;
  }
  
  const TestOperationID release_stage =
      TestApp::generate_operation_id(std::source_location::current(),
                                     "validate_test_input_framework.release_stage");
  if (!app.completed_operations.count(release_stage)) {
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
    
    app.completed_operations.insert(release_stage);
    return;
  }
  
  // Clean up
  test_input::clear_simulated_input();
  
  log_info("TEST_INPUT_FRAMEWORK: All tests passed");
}

