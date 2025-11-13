#pragma once

#include "../../components/can_drop_onto.h"
#include "../../components/dish_level.h"
#include "../../components/is_dish.h"
#include "../../components/is_drop_slot.h"
#include "../../components/is_held.h"
#include "../../components/is_inventory_item.h"
#include "../../components/transform.h"
#include "../../dish_types.h"
#include "../../query.h"
#include "../../shop.h"
#include "../test_app.h"
#include "../test_macros.h"

namespace {

void perform_merge(TestApp &app, afterhours::Entity &donor,
                   afterhours::Entity &target) {
  afterhours::EntityID donor_id = donor.id;
  afterhours::EntityID target_id = target.id;

  app.expect_true(donor.has<IsDish>(), "donor has IsDish");
  app.expect_true(target.has<IsDish>(), "target has IsDish");
  app.expect_true(donor.has<DishLevel>(), "donor has DishLevel");
  app.expect_true(target.has<DishLevel>(), "target has DishLevel");

  app.expect_eq(static_cast<int>(donor.get<IsDish>().type),
                static_cast<int>(target.get<IsDish>().type),
                "donor and target have same dish type");

  app.expect_true(donor.get<DishLevel>().level <= target.get<DishLevel>().level,
                  "donor level <= target level");

  app.expect_true(target.has<IsInventoryItem>(), "target has IsInventoryItem");
  int target_slot_id = target.get<IsInventoryItem>().slot;

  auto target_slot_opt = EQ().whereHasComponent<IsDropSlot>()
                             .whereSlotID(target_slot_id)
                             .gen_first();
  app.expect_true(target_slot_opt.has_value(), "target slot found");
  afterhours::Entity &target_slot = target_slot_opt.asE();

  app.expect_true(target_slot.has<Transform>(), "target slot has Transform");

  vec2 slot_center = target_slot.get<Transform>().center();
  vec2 original_position = vec2{0, 0};
  app.expect_true(donor.has<Transform>(), "donor has Transform");
  original_position = donor.get<Transform>().position;

  donor.addComponent<IsHeld>(vec2{0, 0}, original_position);
  donor.get<Transform>().position =
      slot_center - donor.get<Transform>().size * 0.5f;

  target_slot.addComponent<CanDropOnto>(true);

  // Wait a frame for entities to be merged by system loop
  app.wait_for_frames(1);

  // Use donor reference directly instead of querying by ID
  afterhours::Entity &donor_entity = donor;
  app.expect_true(donor_entity.has<IsHeld>(), "donor has IsHeld");
  app.expect_true(donor_entity.has<Transform>(), "donor has Transform");

  target_slot.get<IsDropSlot>().occupied = true;

  // Wait a frame for occupied flag to be processed
  app.wait_for_frames(1);

  // Use target reference directly instead of querying by ID
  afterhours::Entity &item_in_slot = target;

  app.expect_true(donor_entity.has<IsDish>(), "donor has IsDish after merge");
  app.expect_true(item_in_slot.has<IsDish>(), "target has IsDish after merge");
  app.expect_true(donor_entity.has<DishLevel>(),
                  "donor has DishLevel after merge");
  app.expect_true(item_in_slot.has<DishLevel>(),
                  "target has DishLevel after merge");

  auto &donor_level = donor_entity.get<DishLevel>();
  auto &target_level = item_in_slot.get<DishLevel>();

  app.expect_true(donor_level.level <= target_level.level,
                  "donor level <= target level for merge");

  int donor_contribution = donor_level.contribution_value();
  int target_level_before = target_level.level;
  int target_progress_before = target_level.merge_progress;

  target_level.add_merge_value(donor_contribution);

  app.expect_true(target_level.level > target_level_before ||
                      target_level.merge_progress > target_progress_before,
                  "merge actually applied - level or progress increased");

  app.expect_true(donor_entity.has<IsInventoryItem>(),
                  "donor has IsInventoryItem");
  int original_slot_id = donor_entity.get<IsInventoryItem>().slot;

  donor_entity.cleanup = true;
  donor_entity.removeComponent<IsHeld>();

  auto original_slot_opt = EQ().whereHasComponent<IsDropSlot>()
                               .whereSlotID(original_slot_id)
                               .gen_first();
  app.expect_true(original_slot_opt.has_value(), "original slot found");
  original_slot_opt.asE().get<IsDropSlot>().occupied = false;
}

} // namespace

TEST(validate_dish_level_contribution) {
  (void)app;

  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_ui_exists("Next Round");
  app.wait_for_frames(10);

  app.set_wallet_gold(1000);

  // Test 1: Verify level 1 contribution value
  app.create_inventory_item(DishType::Potato, 0);
  app.wait_for_frames(5);

  auto dish1_opt = app.find_inventory_item_by_slot(0);
  app.expect_true(dish1_opt.has_value(), "dish1 found in slot 0");
  afterhours::Entity &dish1 = dish1_opt.asE();
  DishLevel &level1 = dish1.get<DishLevel>();
  app.expect_eq(level1.level, 1, "dish1 is level 1");
  app.expect_eq(level1.merge_progress, 0, "dish1 has 0 progress");
  app.expect_eq(level1.contribution_value(), 1, "level 1 contributes 1");

  // Test 2: Merge 2 level 1s → should create level 2
  app.create_inventory_item(DishType::Potato, 1);
  app.wait_for_frames(5);

  auto dish2_opt = app.find_inventory_item_by_slot(1);
  app.expect_true(dish2_opt.has_value(), "dish2 found in slot 1");
  afterhours::Entity &dish2 = dish2_opt.asE();
  DishLevel &level1_donor = dish2.get<DishLevel>();
  app.expect_eq(level1_donor.contribution_value(), 1,
                "level 1 donor contributes 1");

  afterhours::EntityID target_id = dish1.id;
  perform_merge(app, dish2, dish1);

  app.wait_for_frames(5);

  // Use force_merge to query immediately after merge
  auto merged1_opt = EQ({.force_merge = true}).whereID(target_id).gen_first();
  app.expect_true(merged1_opt.has_value(),
                  "merged dish exists after first merge");
  afterhours::Entity &merged1 = merged1_opt.asE();
  DishLevel &merged1_level = merged1.get<DishLevel>();
  app.expect_eq(merged1_level.level, 1,
                "merged dish is still level 1 after first merge");
  app.expect_eq(merged1_level.merge_progress, 1,
                "merged dish has progress 1 after first merge");

  app.create_inventory_item(DishType::Potato, 1);
  app.wait_for_frames(5);

  auto dish3_opt = app.find_inventory_item_by_slot(1);
  app.expect_true(dish3_opt.has_value(), "dish3 found in slot 1");
  afterhours::Entity &dish3 = dish3_opt.asE();
  perform_merge(app, dish3, merged1);

  app.wait_for_frames(5);

  // Use force_merge to query immediately after merge
  auto merged2_opt = EQ({.force_merge = true}).whereID(target_id).gen_first();
  app.expect_true(merged2_opt.has_value(),
                  "merged dish exists after second merge");
  afterhours::Entity &merged2 = merged2_opt.asE();
  DishLevel &merged2_level = merged2.get<DishLevel>();
  app.expect_eq(merged2_level.level, 2,
                "merged dish is level 2 after 2 level 1s");
  app.expect_eq(merged2_level.merge_progress, 0,
                "merged dish has 0 progress after level up");
  app.expect_eq(merged2_level.contribution_value(), 2, "level 2 contributes 2");

  // Test 3: Merge 2 level 2s → should create level 3
  app.create_inventory_item(DishType::Potato, 2);
  app.create_inventory_item(DishType::Potato, 3);
  app.wait_for_frames(5);

  auto dish4_opt = app.find_inventory_item_by_slot(2);
  auto dish5_opt = app.find_inventory_item_by_slot(3);
  app.expect_true(dish4_opt.has_value(), "dish4 found");
  app.expect_true(dish5_opt.has_value(), "dish5 found");

  afterhours::Entity &dish4 = dish4_opt.asE();
  afterhours::Entity &dish5 = dish5_opt.asE();

  afterhours::EntityID dish5_id = dish5.id;
  perform_merge(app, dish4, dish5);

  app.wait_for_frames(5);

  // Use force_merge to query immediately after merge
  auto dish5_merged1_opt = EQ({.force_merge = true}).whereID(dish5_id).gen_first();
  app.expect_true(dish5_merged1_opt.has_value(),
                  "dish5 exists after first merge");
  afterhours::Entity &dish5_merged1 = dish5_merged1_opt.asE();
  DishLevel &dish5_level1 = dish5_merged1.get<DishLevel>();
  app.expect_eq(dish5_level1.level, 1, "dish5 is level 1 after first merge");
  app.expect_eq(dish5_level1.merge_progress, 1, "dish5 has progress 1");

  app.create_inventory_item(DishType::Potato, 2);
  app.wait_for_frames(5);

  auto dish6_opt = app.find_inventory_item_by_slot(2);
  app.expect_true(dish6_opt.has_value(), "dish6 found");
  afterhours::Entity &dish6 = dish6_opt.asE();
  perform_merge(app, dish6, dish5_merged1);

  app.wait_for_frames(5);

  // Use force_merge to query immediately after merge
  auto merged3_opt = EQ({.force_merge = true}).whereID(dish5_id).gen_first();
  app.expect_true(merged3_opt.has_value(), "merged level 2 dish exists");
  afterhours::Entity &merged3 = merged3_opt.asE();
  DishLevel &merged3_level = merged3.get<DishLevel>();
  app.expect_eq(merged3_level.level, 2, "merged dish is level 2");
  app.expect_eq(merged3_level.contribution_value(), 2, "level 2 contributes 2");

  afterhours::EntityID target2_id = merged2.id;
  perform_merge(app, merged3, merged2);

  app.wait_for_frames(5);

  // Use force_merge to query immediately after merge
  auto merged4_opt = EQ({.force_merge = true}).whereID(target2_id).gen_first();
  app.expect_true(merged4_opt.has_value(), "merged level 3 dish exists");
  afterhours::Entity &merged4 = merged4_opt.asE();
  DishLevel &merged4_level = merged4.get<DishLevel>();
  app.expect_eq(merged4_level.level, 3,
                "merged dish is level 3 after 2 level 2s");
  app.expect_eq(merged4_level.merge_progress, 0, "merged dish has 0 progress");
  app.expect_eq(merged4_level.contribution_value(), 4, "level 3 contributes 4");

  // Test 4: Merge 4 level 1s → should create level 3 directly
  app.create_inventory_item(DishType::Salmon, 4);
  app.create_inventory_item(DishType::Salmon, 5);
  app.create_inventory_item(DishType::Salmon, 6);
  app.wait_for_frames(5);

  auto salmon1_opt = app.find_inventory_item_by_slot(4);
  auto salmon2_opt = app.find_inventory_item_by_slot(5);
  auto salmon3_opt = app.find_inventory_item_by_slot(6);
  app.expect_true(salmon1_opt.has_value(), "salmon1 found");
  app.expect_true(salmon2_opt.has_value(), "salmon2 found");
  app.expect_true(salmon3_opt.has_value(), "salmon3 found");

  afterhours::Entity &salmon1 = salmon1_opt.asE();
  afterhours::Entity &salmon2 = salmon2_opt.asE();
  afterhours::Entity &salmon3 = salmon3_opt.asE();

  afterhours::EntityID salmon_target_id = salmon1.id;

  perform_merge(app, salmon2, salmon1);
  app.wait_for_frames(5);

  // Use force_merge to query immediately after merge
  auto salmon_merged1_opt = EQ({.force_merge = true}).whereID(salmon_target_id).gen_first();
  app.expect_true(salmon_merged1_opt.has_value(), "salmon merged 1 exists");
  afterhours::Entity &salmon_merged1 = salmon_merged1_opt.asE();
  DishLevel &salmon_level1 = salmon_merged1.get<DishLevel>();
  app.expect_eq(salmon_level1.level, 1, "salmon is level 1 after first merge");
  app.expect_eq(salmon_level1.merge_progress, 1,
                "salmon has progress 1 after first merge");

  perform_merge(app, salmon3, salmon_merged1);
  app.wait_for_frames(5);

  // Use force_merge to query immediately after merge
  auto salmon_merged2_opt = EQ({.force_merge = true}).whereID(salmon_target_id).gen_first();
  app.expect_true(salmon_merged2_opt.has_value(), "salmon merged 2 exists");
  afterhours::Entity &salmon_merged2 = salmon_merged2_opt.asE();
  DishLevel &salmon_level2 = salmon_merged2.get<DishLevel>();
  app.expect_eq(salmon_level2.level, 2, "salmon is level 2 after 2 merges");
  app.expect_eq(salmon_level2.merge_progress, 0,
                "salmon has 0 progress after level up");

  app.create_inventory_item(DishType::Salmon, 0);
  app.wait_for_frames(5);

  auto salmon4_opt = app.find_inventory_item_by_slot(0);
  app.expect_true(salmon4_opt.has_value(), "salmon4 found");
  afterhours::Entity &salmon4 = salmon4_opt.asE();

  perform_merge(app, salmon4, salmon_merged2);
  app.wait_for_frames(5);

  // Use force_merge to query immediately after merge
  auto salmon_merged3_opt = EQ({.force_merge = true}).whereID(salmon_target_id).gen_first();
  app.expect_true(salmon_merged3_opt.has_value(), "salmon merged 3 exists");
  afterhours::Entity &salmon_merged3 = salmon_merged3_opt.asE();
  DishLevel &salmon_level3_check = salmon_merged3.get<DishLevel>();
  app.expect_eq(salmon_level3_check.level, 2,
                "salmon is level 2 after 3 merges");
  app.expect_eq(salmon_level3_check.merge_progress, 1, "salmon has progress 1");

  app.create_inventory_item(DishType::Salmon, 1);
  app.wait_for_frames(5);

  auto salmon5_opt = app.find_inventory_item_by_slot(1);
  app.expect_true(salmon5_opt.has_value(), "salmon5 found");
  afterhours::Entity &salmon5 = salmon5_opt.asE();

  perform_merge(app, salmon5, salmon_merged3);
  app.wait_for_frames(5);

  // Use force_merge to query immediately after merge
  auto salmon_final_opt = EQ({.force_merge = true}).whereID(salmon_target_id).gen_first();
  app.expect_true(salmon_final_opt.has_value(), "salmon final exists");
  afterhours::Entity &salmon_final = salmon_final_opt.asE();
  DishLevel &salmon_final_level = salmon_final.get<DishLevel>();
  app.expect_eq(salmon_final_level.level, 3,
                "salmon is level 3 after 4 level 1s");
  app.expect_eq(salmon_final_level.merge_progress, 0, "salmon has 0 progress");
  app.expect_eq(salmon_final_level.contribution_value(), 4,
                "level 3 contributes 4");
}
