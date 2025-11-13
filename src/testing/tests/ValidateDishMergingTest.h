#pragma once

#include "../../components/can_drop_onto.h"
#include "../../components/dish_level.h"
#include "../../components/is_dish.h"
#include "../../components/is_drop_slot.h"
#include "../../components/is_held.h"
#include "../../components/is_inventory_item.h"
#include "../../components/is_shop_item.h"
#include "../../components/transform.h"
#include "../../dish_types.h"
#include "../../query.h"
#include "../../shop.h"
#include "../test_app.h"
#include "../test_macros.h"
#include <functional>
#include <optional>

namespace {

afterhours::OptEntity find_entity_by_id(afterhours::EntityID id) {
  return EQ().whereID(id).gen_first();
}

// Simulate drag-and-drop merge between two dish entities
bool simulate_drag_and_drop(afterhours::Entity &donor,
                            afterhours::Entity &target) {
  afterhours::EntityID donor_id = donor.id;
  afterhours::EntityID target_id = target.id;

  // Validate both entities are dishes with matching types
  if (!donor.has<IsDish>() || !target.has<IsDish>() ||
      !donor.has<DishLevel>() || !target.has<DishLevel>()) {
    return false;
  }

  if (donor.get<IsDish>().type != target.get<IsDish>().type) {
    return false;
  }

  if (donor.get<DishLevel>().level > target.get<DishLevel>().level) {
    return false;
  }

  // Find target slot
  int target_slot_id = -1;
  if (target.has<IsInventoryItem>()) {
    target_slot_id = target.get<IsInventoryItem>().slot;
  } else if (target.has<IsShopItem>()) {
    target_slot_id = target.get<IsShopItem>().slot;
  } else {
    return false;
  }

  auto target_slot_opt = EQ({.force_merge = true})
                             .whereHasComponent<IsDropSlot>()
                             .whereSlotID(target_slot_id)
                             .gen_first();
  if (!target_slot_opt) {
    return false;
  }
  afterhours::Entity &target_slot = target_slot_opt.asE();

  // Set up transform and drag state
  if (!donor.has<Transform>()) {
    donor.addComponent<Transform>(vec2{0, 0}, vec2{80.0f, 80.0f});
  }
  if (!target_slot.has<Transform>()) {
    return false;
  }

  vec2 slot_center = target_slot.get<Transform>().center();
  Transform &donor_transform = donor.get<Transform>();
  vec2 original_position = donor_transform.position;

  donor.addComponent<IsHeld>(vec2{0, 0}, original_position);
  donor.get<Transform>().position =
      slot_center - donor.get<Transform>().size * 0.5f;

  if (!target_slot.has<CanDropOnto>()) {
    target_slot.addComponent<CanDropOnto>(true);
  }

  // Use donor reference directly instead of re-querying
  afterhours::Entity &donor_entity = donor;
  if (!donor_entity.has<IsHeld>() || !donor_entity.has<Transform>()) {
    return false;
  }

  IsHeld &held = donor_entity.get<IsHeld>();

  // Mark target slot as occupied
  if (!target_slot.get<IsDropSlot>().occupied) {
    target_slot.get<IsDropSlot>().occupied = true;
  }

  // Use target reference directly instead of re-querying
  afterhours::Entity &item_in_slot = target;

  // Re-validate merge conditions
  if (!donor_entity.has<IsDish>() || !item_in_slot.has<IsDish>() ||
      !donor_entity.has<DishLevel>() || !item_in_slot.has<DishLevel>()) {
    donor_entity.removeComponent<IsHeld>();
    return false;
  }

  auto &donor_dish = donor_entity.get<IsDish>();
  auto &target_dish = item_in_slot.get<IsDish>();
  auto &donor_level = donor_entity.get<DishLevel>();
  auto &target_level = item_in_slot.get<DishLevel>();

  if (donor_dish.type != target_dish.type ||
      donor_level.level > target_level.level) {
    donor_entity.removeComponent<IsHeld>();
    return false;
  }

  // Charge for shop purchase if needed
  if (donor_entity.has<IsShopItem>() && item_in_slot.has<IsInventoryItem>()) {
    if (!charge_for_shop_purchase(donor_dish.type)) {
      donor_entity.removeComponent<IsHeld>();
      donor_entity.get<Transform>().position = held.original_position;
      return false;
    }
  }

  // Perform the merge
  target_level.add_merge_value(donor_level.contribution_value());

  // Free the donor's original slot
  int original_slot_id = -1;
  if (donor_entity.has<IsInventoryItem>()) {
    original_slot_id = donor_entity.get<IsInventoryItem>().slot;
  } else if (donor_entity.has<IsShopItem>()) {
    original_slot_id = donor_entity.get<IsShopItem>().slot;
  }

  donor_entity.cleanup = true;
  donor_entity.removeComponent<IsHeld>();

  if (original_slot_id >= 0) {
    auto original_slot_opt = EQ({.force_merge = true})
                                 .whereHasComponent<IsDropSlot>()
                                 .whereSlotID(original_slot_id)
                                 .gen_first();
    if (original_slot_opt) {
      original_slot_opt.asE().get<IsDropSlot>().occupied = false;
    }
  }

  return true;
}

} // namespace

TEST(validate_dish_merging) {
  (void)app;

  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_ui_exists("Next Round");
  app.wait_for_frames(10);

  app.set_wallet_gold(1000);

  // Test 1: Merge two same-level dishes
  app.create_inventory_item(DishType::Potato, 4);
  app.create_inventory_item(DishType::Potato, 5);
  app.wait_for_frames(5);

  // Entities merged by system loop, regular query is sufficient
  afterhours::EntityQuery eq_merged;
  auto dish0_opt = app.find_inventory_item_by_slot(4);
  auto dish1_opt = app.find_inventory_item_by_slot(5);

  app.expect_true(dish0_opt.has_value(), "dish0 found in slot 4");
  app.expect_true(dish1_opt.has_value(), "dish1 found in slot 5");

  afterhours::Entity &dish0 = dish0_opt.asE();
  afterhours::Entity &dish1 = dish1_opt.asE();

  DishLevel &dish0_level = dish0.get<DishLevel>();
  DishLevel &dish1_level = dish1.get<DishLevel>();

  app.expect_eq(dish0_level.level, 1, "dish0 initial level");
  app.expect_eq(dish1_level.level, 1, "dish1 initial level");
  app.expect_eq(dish0_level.merge_progress, 0, "dish0 initial merge progress");
  app.expect_eq(dish1_level.merge_progress, 0, "dish1 initial merge progress");

  afterhours::EntityID target_id = dish1.id;
  bool merge_simulated = simulate_drag_and_drop(dish0, dish1);
  app.expect_true(merge_simulated, "merge simulation succeeded");

  app.wait_for_frames(5);

  // Entities merged by system loop, regular query is sufficient
  auto merged_dish_opt = EQ().whereID(target_id).gen_first();
  bool donor_removed = true;
  for (afterhours::Entity &entity :
       eq_merged.whereHasComponent<IsInventoryItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    // TODO add a cleanup filter
    if (entity.id == dish0.id && !entity.cleanup) {
      donor_removed = false;
    }
  }

  app.expect_true(merged_dish_opt.has_value(), "merged dish still exists");
  app.expect_true(donor_removed, "donor dish was removed");

  afterhours::Entity &merged_dish = merged_dish_opt.asE();
  DishLevel &merged_level = merged_dish.get<DishLevel>();
  app.expect_eq(merged_level.level, 1,
                "merged dish still level 1 after first merge");
  app.expect_eq(merged_level.merge_progress, 1,
                "merged dish has progress 1 after first merge");

  // Verify donor slot is freed
  auto donor_slot_opt = app.find_drop_slot(4);
  app.expect_true(donor_slot_opt.has_value(), "donor slot found");
  afterhours::Entity &donor_slot = donor_slot_opt.asE();
  app.expect_false(donor_slot.get<IsDropSlot>().occupied,
                   "donor slot is freed after merge");

  // Test 2: Second merge to level up
  app.create_inventory_item(DishType::Potato, 6);
  app.wait_for_frames(5);

  // Wait a bit more to ensure all entities from first merge are fully processed
  app.wait_for_frames(2);

  auto dish0_opt_2 = app.find_inventory_item_by_slot(6);
  app.expect_true(dish0_opt_2.has_value(),
                  "dish0 found in slot 6 for second merge");

  // Re-query merged dish to get fresh reference after first merge
  // Entities already merged by system loop from wait_for_frames above
  auto merged_dish_opt_refresh = EQ().whereID(target_id).gen_first();
  app.expect_true(merged_dish_opt_refresh.has_value(),
                  "merged dish still exists");

  // Get fresh references for both entities
  afterhours::Entity &merged_dish_ref = merged_dish_opt_refresh.asE();
  afterhours::Entity &dish0_new = dish0_opt_2.asE();

  DishLevel &target_level = merged_dish_ref.get<DishLevel>();
  app.expect_eq(target_level.level, 1,
                "merged dish is level 1 before second merge");
  app.expect_eq(target_level.merge_progress, 1,
                "merged dish has progress 1 before second merge");

  // Wait one more frame before second merge to ensure entities are fully merged
  app.wait_for_frames(1);

  bool second_merge_simulated =
      simulate_drag_and_drop(dish0_new, merged_dish_ref);
  app.expect_true(second_merge_simulated, "second merge simulation succeeded");

  app.wait_for_frames(5);

  // Entities merged by system loop, regular query is sufficient
  auto merged_dish_opt_2 = EQ().whereID(target_id).gen_first();
  app.expect_true(merged_dish_opt_2.has_value(),
                  "merged dish found after second merge");
  afterhours::Entity &merged_dish_final = merged_dish_opt_2.asE();
  DishLevel &final_level = merged_dish_final.get<DishLevel>();
  app.expect_eq(final_level.level, 2,
                "merged dish leveled up to 2 after second merge");
  app.expect_eq(final_level.merge_progress, 0,
                "merged dish merge progress reset after leveling");

  // Test 3: Shop to inventory merge with wallet charge
  app.set_wallet_gold(100);
  int gold_before = app.read_wallet_gold();
  DishType merge_test_type = DishType::Salmon;
  app.wait_for_frames(10);

  // Clear a shop slot to make room for our test item
  int free_slot = -1;
  for (afterhours::Entity &entity :
       EQ({.force_merge = true})
           .template whereHasComponent<IsShopItem>()
           .gen()) {
    int slot = entity.get<IsShopItem>().slot;
    entity.cleanup = true;
    auto slot_entity_opt = EQ({.force_merge = true})
                               .whereHasComponent<IsDropSlot>()
                               .whereSlotID(slot)
                               .gen_first();
    if (slot_entity_opt) {
      slot_entity_opt.asE().get<IsDropSlot>().occupied = false;
    }
    free_slot = slot;
    break;
  }
  app.expect_true(free_slot >= 0, "No free shop slot available");

  // Clear an inventory slot for the shop merge test
  int inv_slot_for_shop_merge = -1;
  for (afterhours::Entity &entity :
       EQ({.force_merge = true})
           .template whereHasComponent<IsInventoryItem>()
           .gen()) {
    int slot = entity.get<IsInventoryItem>().slot;
    if (slot >= 4 && slot <= 6) {
      entity.cleanup = true;
      auto slot_entity_opt = app.find_drop_slot(slot);
      if (slot_entity_opt) {
        slot_entity_opt.asE().get<IsDropSlot>().occupied = false;
      }
      inv_slot_for_shop_merge = slot;
      break;
    }
  }
app.expect_true(inv_slot_for_shop_merge >= 0,
                "No free inventory slot available for shop merge");

afterhours::Entity &shop_entity = make_shop_item(free_slot, merge_test_type);
afterhours::EntityID shop_entity_id = shop_entity.id;

app.create_inventory_item(merge_test_type, inv_slot_for_shop_merge);
app.wait_for_frames(5);

auto shop_item_opt = app.find_shop_item(shop_entity_id, free_slot);
auto inventory_item_opt =
    app.find_inventory_item_by_slot(inv_slot_for_shop_merge);

app.expect_true(shop_item_opt.has_value(), "shop item found");
app.expect_true(inventory_item_opt.has_value(), "inventory item found");

afterhours::Entity &shop_item = shop_item_opt.asE();
afterhours::Entity &inventory_item = inventory_item_opt.asE();

DishType shop_type = shop_item.get<IsDish>().type;
DishType inv_type = inventory_item.get<IsDish>().type;

app.expect_eq(static_cast<int>(shop_type), static_cast<int>(inv_type),
              "shop and inventory items are same type");
app.expect_eq(static_cast<int>(shop_type), static_cast<int>(merge_test_type),
              "shop item is correct type");

int price = get_dish_info(shop_type).price;

bool shop_merge_simulated = simulate_drag_and_drop(shop_item, inventory_item);
app.expect_true(shop_merge_simulated, "shop merge simulation succeeded");

app.wait_for_frames(5);

int gold_after = app.read_wallet_gold();
app.expect_eq(gold_after, gold_before - price,
              "wallet charged for shop dish merge");
}
