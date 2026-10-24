#pragma once

#include "../../components/dish_level.h"
#include "../../components/is_drop_slot.h"
#include "../../components/is_inventory_item.h"
#include "../../components/transform.h"
#include "../../dish_types.h"
#include "../../query.h"
#include "../../rl.h"
#include "../test_app.h"
#include "../test_input.h"
#include "../test_macros.h"
#include <string>

namespace {

void press_donor(TestApp &app, int donor_slot, const std::string &key) {
  app.run_once(key, [&app, donor_slot]() {
    auto donor_opt = app.find_inventory_item_by_slot(donor_slot);
    app.expect_true(donor_opt.has_value(), "donor found for drag");
    test_input::set_mouse_position(
        donor_opt.asE().get<Transform>().center());
    test_input::simulate_mouse_button_press(raylib::MOUSE_BUTTON_LEFT);
  });
}

void release_on_target(TestApp &app, int target_slot, const std::string &key) {
  app.run_once(key, [&app, target_slot]() {
    auto target_opt = app.find_inventory_item_by_slot(target_slot);
    app.expect_true(target_opt.has_value(), "target found for drop");
    test_input::set_mouse_position(
        target_opt.asE().get<Transform>().center());
    test_input::simulate_mouse_button_release(raylib::MOUSE_BUTTON_LEFT);
  });
}

void expect_slot_freed(TestApp &app, int slot, const std::string &label) {
  auto slot_opt = app.find_drop_slot(slot);
  app.expect_true(slot_opt.has_value(), label + " drop slot found");
  app.expect_false(slot_opt.asE().get<IsDropSlot>().occupied,
                   label + " drop slot freed");
}

} // namespace

TEST(validate_triple_merge) {
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_ui_exists("Next Round");
  app.wait_for_frames(10);

  app.run_once("triple_create", [&app]() {
    app.set_wallet_gold(1000);
    app.create_inventory_item(DishType::Potato, 4);
    app.create_inventory_item(DishType::Potato, 5);
    app.create_inventory_item(DishType::Potato, 6);
  });
  app.wait_for_frames(5);

  press_donor(app, 4, "triple_press");
  app.wait_for_frames(5);
  release_on_target(app, 5, "triple_release");
  app.wait_for_frames(5);

  app.run_once("triple_verify", [&app]() {
    auto target_opt = app.find_inventory_item_by_slot(5);
    app.expect_true(target_opt.has_value(), "target still in slot 5");
    DishLevel &level = target_opt.asE().get<DishLevel>();
    app.expect_eq(level.level, 2, "triple merge leveled up to 2");
    app.expect_eq(level.merge_progress, 0, "triple merge progress reset");
    app.expect_false(app.find_inventory_item_by_slot(4).has_value(),
                     "donor consumed by triple merge");
    app.expect_false(app.find_inventory_item_by_slot(6).has_value(),
                     "third dish consumed by triple merge");
    expect_slot_freed(app, 4, "donor slot 4");
    expect_slot_freed(app, 6, "third slot 6");
  });

  app.run_once("triple_clear", []() { test_input::clear_simulated_input(); });
}

TEST(validate_triple_merge_far_third) {
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_ui_exists("Next Round");
  app.wait_for_frames(10);

  app.run_once("far_create", [&app]() {
    app.set_wallet_gold(1000);
    app.create_inventory_item(DishType::Potato, 0);
    app.create_inventory_item(DishType::Potato, 3);
    app.create_inventory_item(DishType::Potato, 6);
  });
  app.wait_for_frames(5);

  press_donor(app, 0, "far_press");
  app.wait_for_frames(5);
  release_on_target(app, 3, "far_release");
  app.wait_for_frames(5);

  app.run_once("far_verify", [&app]() {
    auto target_opt = app.find_inventory_item_by_slot(3);
    app.expect_true(target_opt.has_value(), "target still in slot 3");
    DishLevel &level = target_opt.asE().get<DishLevel>();
    app.expect_eq(level.level, 1, "far third stays level 1");
    app.expect_eq(level.merge_progress, 1,
                  "far third gives double-merge progress only");
    app.expect_false(app.find_inventory_item_by_slot(0).has_value(),
                     "donor consumed by double merge");
    auto third_opt = app.find_inventory_item_by_slot(6);
    app.expect_true(third_opt.has_value(), "far third dish survives");
    DishLevel &third_level = third_opt.asE().get<DishLevel>();
    app.expect_eq(third_level.level, 1, "far third still level 1");
    app.expect_eq(third_level.merge_progress, 0,
                  "far third progress untouched");
    expect_slot_freed(app, 0, "donor slot 0");
    auto slot6_opt = app.find_drop_slot(6);
    app.expect_true(slot6_opt.has_value(), "slot 6 found");
    app.expect_true(slot6_opt.asE().get<IsDropSlot>().occupied,
                    "slot 6 still occupied");
  });

  app.run_once("far_clear", []() { test_input::clear_simulated_input(); });
}

TEST(validate_triple_merge_mixed_levels) {
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_ui_exists("Next Round");
  app.wait_for_frames(10);

  app.run_once("mixed_create", [&app]() {
    app.set_wallet_gold(1000);
    app.create_inventory_item(DishType::Potato, 0);
    app.create_inventory_item(DishType::Potato, 3);
    app.create_inventory_item(DishType::Potato, 6);
  });
  app.wait_for_frames(5);

  press_donor(app, 0, "mixed_first_press");
  app.wait_for_frames(5);
  release_on_target(app, 3, "mixed_first_release");
  app.wait_for_frames(5);

  app.run_once("mixed_verify_first_double", [&app]() {
    auto target_opt = app.find_inventory_item_by_slot(3);
    app.expect_true(target_opt.has_value(), "target still in slot 3");
    DishLevel &level = target_opt.asE().get<DishLevel>();
    app.expect_eq(level.level, 1, "first double merge keeps level 1");
    app.expect_eq(level.merge_progress, 1, "first double merge adds progress");
  });

  press_donor(app, 6, "mixed_second_press");
  app.wait_for_frames(5);
  release_on_target(app, 3, "mixed_second_release");
  app.wait_for_frames(5);

  app.run_once("mixed_verify_level_two", [&app]() {
    auto target_opt = app.find_inventory_item_by_slot(3);
    app.expect_true(target_opt.has_value(), "target still in slot 3");
    DishLevel &level = target_opt.asE().get<DishLevel>();
    app.expect_eq(level.level, 2, "second double merge reached level 2");
    app.expect_eq(level.merge_progress, 0, "progress reset after level up");
  });

  app.run_once("mixed_create_third", [&app]() {
    app.create_inventory_item(DishType::Potato, 2);
    app.create_inventory_item(DishType::Potato, 4);
  });
  app.wait_for_frames(5);

  press_donor(app, 2, "mixed_third_press");
  app.wait_for_frames(5);
  release_on_target(app, 3, "mixed_third_release");
  app.wait_for_frames(5);

  app.run_once("mixed_verify_triple", [&app]() {
    auto target_opt = app.find_inventory_item_by_slot(3);
    app.expect_true(target_opt.has_value(), "target still in slot 3");
    DishLevel &level = target_opt.asE().get<DishLevel>();
    app.expect_eq(level.level, 3, "triple merge reached level 3");
    app.expect_eq(level.merge_progress, 0, "progress reset after level up");
    app.expect_false(app.find_inventory_item_by_slot(2).has_value(),
                     "donor consumed by triple merge");
    app.expect_false(app.find_inventory_item_by_slot(4).has_value(),
                     "third dish consumed by triple merge");
    expect_slot_freed(app, 2, "donor slot 2");
    expect_slot_freed(app, 4, "third slot 4");
  });

  app.run_once("mixed_clear", []() { test_input::clear_simulated_input(); });
}
