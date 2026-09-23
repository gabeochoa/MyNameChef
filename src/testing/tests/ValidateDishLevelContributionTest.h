#pragma once

#include "../../components/dish_level.h"
#include "../../components/is_dish.h"
#include "../../components/is_drop_slot.h"
#include "../../components/is_inventory_item.h"
#include "../../dish_types.h"
#include "../../query.h"
#include "../../shop.h"
#include "../test_app.h"
#include "../test_macros.h"

namespace ValidateDishLevelContributionTestHelpers {

static afterhours::Entity &dish_by_id(TestApp &app, afterhours::EntityID id) {
  afterhours::OptEntity opt = EQ({.force_merge = true}).whereID(id).gen_first();
  app.expect_true(opt.has_value(), "dish " + std::to_string(id) + " exists");
  return opt.asE();
}

static afterhours::Entity &dish_in_slot(TestApp &app, int slot) {
  afterhours::OptEntity opt = app.find_inventory_item_by_slot(slot);
  app.expect_true(opt.has_value(),
                  "dish found in slot " + std::to_string(slot));
  return opt.asE();
}

static void perform_merge(TestApp &app, afterhours::Entity &donor,
                          afterhours::Entity &target) {
  app.expect_true(donor.has<DishLevel>(), "donor has DishLevel");
  app.expect_true(target.has<DishLevel>(), "target has DishLevel");
  app.expect_eq(static_cast<int>(donor.get<IsDish>().type),
                static_cast<int>(target.get<IsDish>().type),
                "donor and target have same dish type");

  DishLevel &donor_level = donor.get<DishLevel>();
  DishLevel &target_level = target.get<DishLevel>();
  app.expect_true(donor_level.level <= target_level.level,
                  "donor level <= target level");

  const int level_before = target_level.level;
  const int progress_before = target_level.merge_progress;
  target_level.add_merge_value(donor_level.contribution_value());
  app.expect_true(target_level.level > level_before ||
                      target_level.merge_progress > progress_before,
                  "merge actually applied - level or progress increased");

  app.expect_true(donor.has<IsInventoryItem>(), "donor has IsInventoryItem");
  afterhours::OptEntity slot_opt =
      app.find_drop_slot(donor.get<IsInventoryItem>().slot);
  app.expect_true(slot_opt.has_value(), "donor slot found");
  slot_opt.asE().get<IsDropSlot>().occupied = false;
  donor.cleanup = true;
}

static void expect_level(TestApp &app, afterhours::Entity &dish, int level,
                         int progress, const std::string &what) {
  DishLevel &dish_level = dish.get<DishLevel>();
  app.expect_eq(dish_level.level, level, what + " level");
  app.expect_eq(dish_level.merge_progress, progress, what + " progress");
}

} // namespace ValidateDishLevelContributionTestHelpers

TEST(validate_dish_level_contribution) {
  using namespace ValidateDishLevelContributionTestHelpers;

  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_ui_exists("Next Round");
  app.wait_for_frames(10);

  app.create_inventory_item(DishType::Potato, 0);
  app.create_inventory_item(DishType::Potato, 1);
  app.wait_for_frames(5);

  app.once([&] {
    afterhours::Entity &dish1 = dish_in_slot(app, 0);
    afterhours::Entity &dish2 = dish_in_slot(app, 1);
    app.set_test_int("target_id", dish1.id);
    expect_level(app, dish1, 1, 0, "dish1");
    app.expect_eq(dish1.get<DishLevel>().contribution_value(), 1,
                  "level 1 contributes 1");
    perform_merge(app, dish2, dish1);
    expect_level(app, dish1, 1, 1, "after first merge");
  });
  app.wait_for_frames(5);

  app.create_inventory_item(DishType::Potato, 1);
  app.wait_for_frames(5);

  app.once([&] {
    afterhours::Entity &target =
        dish_by_id(app, app.get_test_int("target_id").value());
    perform_merge(app, dish_in_slot(app, 1), target);
    expect_level(app, target, 2, 0, "after 2 level 1s");
    app.expect_eq(target.get<DishLevel>().contribution_value(), 2,
                  "level 2 contributes 2");
  });
  app.wait_for_frames(5);

  app.create_inventory_item(DishType::Potato, 2);
  app.create_inventory_item(DishType::Potato, 3);
  app.wait_for_frames(5);

  app.once([&] {
    afterhours::Entity &dish5 = dish_in_slot(app, 3);
    app.set_test_int("dish5_id", dish5.id);
    perform_merge(app, dish_in_slot(app, 2), dish5);
    expect_level(app, dish5, 1, 1, "dish5 after first merge");
  });
  app.wait_for_frames(5);

  app.create_inventory_item(DishType::Potato, 2);
  app.wait_for_frames(5);

  app.once([&] {
    afterhours::Entity &dish5 =
        dish_by_id(app, app.get_test_int("dish5_id").value());
    perform_merge(app, dish_in_slot(app, 2), dish5);
    expect_level(app, dish5, 2, 0, "dish5 after second merge");
    app.expect_eq(dish5.get<DishLevel>().contribution_value(), 2,
                  "level 2 contributes 2");

    afterhours::Entity &target =
        dish_by_id(app, app.get_test_int("target_id").value());
    perform_merge(app, dish5, target);
    expect_level(app, target, 3, 0, "after 2 level 2s");
    app.expect_eq(target.get<DishLevel>().contribution_value(), 4,
                  "level 3 contributes 4");
  });
  app.wait_for_frames(5);

  app.create_inventory_item(DishType::Salmon, 4);
  app.create_inventory_item(DishType::Salmon, 5);
  app.create_inventory_item(DishType::Salmon, 6);
  app.wait_for_frames(5);

  app.once([&] {
    afterhours::Entity &salmon1 = dish_in_slot(app, 4);
    app.set_test_int("salmon_id", salmon1.id);
    perform_merge(app, dish_in_slot(app, 5), salmon1);
    expect_level(app, salmon1, 1, 1, "salmon after first merge");
    perform_merge(app, dish_in_slot(app, 6), salmon1);
    expect_level(app, salmon1, 2, 0, "salmon after 2 merges");
  });
  app.wait_for_frames(5);

  app.create_inventory_item(DishType::Salmon, 5);
  app.wait_for_frames(5);

  app.once([&] {
    afterhours::Entity &salmon =
        dish_by_id(app, app.get_test_int("salmon_id").value());
    perform_merge(app, dish_in_slot(app, 5), salmon);
    expect_level(app, salmon, 2, 1, "salmon after 3 merges");
  });
  app.wait_for_frames(5);

  app.create_inventory_item(DishType::Salmon, 6);
  app.wait_for_frames(5);

  app.once([&] {
    afterhours::Entity &salmon =
        dish_by_id(app, app.get_test_int("salmon_id").value());
    perform_merge(app, dish_in_slot(app, 6), salmon);
    expect_level(app, salmon, 3, 0, "salmon after 4 level 1s");
    app.expect_eq(salmon.get<DishLevel>().contribution_value(), 4,
                  "level 3 contributes 4");
  });
  app.wait_for_frames(5);
}
