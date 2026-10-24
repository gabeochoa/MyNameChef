#pragma once

#include "../../components/dish_level.h"
#include "../../components/is_drop_slot.h"
#include "../../components/is_inventory_item.h"
#include "../../components/merge_flash.h"
#include "../../components/transform.h"
#include "../../dish_types.h"
#include "../../merge.h"
#include "../../query.h"
#include "../../rl.h"
#include "../test_app.h"
#include "../test_input.h"
#include "../test_macros.h"
#include <string>

namespace {

void merge_visuals_press_donor(TestApp &app, int donor_slot,
                               const std::string &key) {
  app.run_once(key, [&app, donor_slot]() {
    auto donor_opt = app.find_inventory_item_by_slot(donor_slot);
    app.expect_true(donor_opt.has_value(), "donor found for drag");
    test_input::set_mouse_position(
        donor_opt.asE().get<Transform>().center());
    test_input::simulate_mouse_button_press(raylib::MOUSE_BUTTON_LEFT);
  });
}

void merge_visuals_release_on_target(TestApp &app, int target_slot,
                                     const std::string &key) {
  app.run_once(key, [&app, target_slot]() {
    auto target_opt = app.find_inventory_item_by_slot(target_slot);
    app.expect_true(target_opt.has_value(), "target found for drop");
    test_input::set_mouse_position(
        target_opt.asE().get<Transform>().center());
    test_input::simulate_mouse_button_release(raylib::MOUSE_BUTTON_LEFT);
  });
}

} // namespace

TEST(validate_merge_flash_on_merge) {
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_ui_exists("Next Round");
  app.wait_for_frames(10);

  app.run_once("flash_create", [&app]() {
    app.set_wallet_gold(1000);
    app.create_inventory_item(DishType::Potato, 4);
    app.create_inventory_item(DishType::Potato, 5);
  });
  app.wait_for_frames(5);

  app.run_once("flash_absent_before_merge", [&app]() {
    auto target_opt = app.find_inventory_item_by_slot(5);
    app.expect_true(target_opt.has_value(), "target found before merge");
    app.expect_false(target_opt.asE().has<MergeFlash>(),
                     "no merge flash before merge");
  });

  merge_visuals_press_donor(app, 4, "flash_press");
  app.wait_for_frames(5);
  merge_visuals_release_on_target(app, 5, "flash_release");
  app.wait_for_frames(3);

  app.run_once("flash_verify", [&app]() {
    auto target_opt = app.find_inventory_item_by_slot(5);
    app.expect_true(target_opt.has_value(), "target still in slot 5");
    app.expect_true(target_opt.asE().has<MergeFlash>(),
                    "merge flash appears on target after merge");
    if (target_opt.asE().has<MergeFlash>()) {
      app.expect_false(target_opt.asE().get<MergeFlash>().leveledUp,
                       "first merge does not level up");
    }
    DishLevel &level = target_opt.asE().get<DishLevel>();
    app.expect_eq(level.merge_progress, 1,
                  "merge combined progress on target");
    app.expect_false(app.find_inventory_item_by_slot(4).has_value(),
                     "donor consumed by merge");
  });

  app.run_once("flash_clear", []() { test_input::clear_simulated_input(); });
}

TEST(validate_merge_partner_detection) {
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_ui_exists("Next Round");
  app.wait_for_frames(10);

  app.run_once("partner_create", [&app]() {
    app.set_wallet_gold(1000);
    app.create_inventory_item(DishType::Potato, 4);
    app.create_inventory_item(DishType::Potato, 5);
    app.create_inventory_item(DishType::FrenchFries, 6);
  });
  app.wait_for_frames(5);

  app.run_once("partner_verify", [&app]() {
    auto first_opt = app.find_inventory_item_by_slot(4);
    auto second_opt = app.find_inventory_item_by_slot(5);
    auto other_opt = app.find_inventory_item_by_slot(6);
    app.expect_true(first_opt.has_value(), "first Potato found");
    app.expect_true(second_opt.has_value(), "second Potato found");
    app.expect_true(other_opt.has_value(), "FrenchFries found");
    app.expect_true(merge::dish_has_merge_partner(first_opt.asE()),
                    "first Potato has a merge partner");
    app.expect_true(merge::dish_has_merge_partner(second_opt.asE()),
                    "second Potato has a merge partner");
    app.expect_false(merge::dish_has_merge_partner(other_opt.asE()),
                     "lone FrenchFries has no merge partner");
  });
}
