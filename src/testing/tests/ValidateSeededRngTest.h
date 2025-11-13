#pragma once

#include "../../components/is_dish.h"
#include "../../components/is_shop_item.h"
#include "../../dish_types.h"
#include "../../query.h"
#include "../../seeded_rng.h"
#include "../../shop.h"
#include "../test_macros.h"
#include <vector>

TEST(validate_seeded_rng_determinism) {
  // Test that SeededRng produces deterministic results

  // Helper function to get shop items
  // Use force_merge since this helper might be called immediately after
  // creation
  auto get_shop_items = []() {
    std::vector<DishType> items;
    for (auto &ref : afterhours::EntityQuery({.force_merge = true})
                         .template whereHasComponent<IsShopItem>()
                         .gen()) {
      auto &dish = ref.get().get<IsDish>();
      items.push_back(dish.type);
    }
    return items;
  };

  // Initialize game
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_frames(20);

  // Clear any existing shop items
  // Use force_merge here since we're querying immediately after cleanup
  for (auto &ref : afterhours::EntityQuery({.force_merge = true})
                       .template whereHasComponent<IsShopItem>()
                       .gen()) {
    ref.get().cleanup = true;
  }
  afterhours::EntityHelper::cleanup();
  // Cleanup will be processed by system loop
  app.wait_for_frames(5);

  // Test 1: Verify same seed produces same shop items
  const uint64_t test_seed = 1234567890;

  // Set seed and generate shop items (round 1)
  auto &rng = SeededRng::get();
  rng.set_seed(test_seed);

  // Generate shop items
  auto free_slots = get_free_slots(SHOP_SLOTS);
  for (int slot : free_slots) {
    make_shop_item(slot, get_random_dish_for_tier(1));
  }
  // Entities will be merged by system loop
  app.wait_for_frames(5);

  std::vector<DishType> first_run = get_shop_items();
  app.expect_count_eq(static_cast<int>(first_run.size()), SHOP_SLOTS,
                      "shop items from first run");

  // Clear shop items
  for (auto &ref : afterhours::EntityQuery()
                       .template whereHasComponent<IsShopItem>()
                       .gen()) {
    ref.get().cleanup = true;
  }
  afterhours::EntityHelper::cleanup();
  // Cleanup will be processed by system loop
  app.wait_for_frames(5);

  // Reset seed to same value and generate again (round 2)
  rng.set_seed(test_seed);

  free_slots = get_free_slots(SHOP_SLOTS);
  for (int slot : free_slots) {
    make_shop_item(slot, get_random_dish_for_tier(1));
  }
  // Entities will be merged by system loop
  app.wait_for_frames(5);

  std::vector<DishType> second_run = get_shop_items();
  app.expect_count_eq(static_cast<int>(second_run.size()), SHOP_SLOTS,
                      "shop items from second run");

  // Verify both runs produced identical results
  app.expect_count_eq(static_cast<int>(first_run.size()),
                      static_cast<int>(second_run.size()),
                      "shop item count matches between runs");

  for (size_t i = 0; i < first_run.size(); ++i) {
    app.expect_eq(static_cast<int>(first_run[i]),
                  static_cast<int>(second_run[i]),
                  "shop item " + std::to_string(i) + " matches");
  }

  // Test 2: Verify different seed produces different shop items
  rng.set_seed(test_seed + 1);

  // Clear shop items
  for (auto &ref : afterhours::EntityQuery()
                       .template whereHasComponent<IsShopItem>()
                       .gen()) {
    ref.get().cleanup = true;
  }
  afterhours::EntityHelper::cleanup();
  // Cleanup will be processed by system loop
  app.wait_for_frames(5);

  // Generate with different seed
  free_slots = get_free_slots(SHOP_SLOTS);
  for (int slot : free_slots) {
    make_shop_item(slot, get_random_dish_for_tier(1));
  }
  // Entities will be merged by system loop
  app.wait_for_frames(5);

  std::vector<DishType> different_seed_run = get_shop_items();

  // Verify different seed produced different results (at least one item
  // differs)
  bool found_difference = false;
  for (size_t i = 0; i < std::min(first_run.size(), different_seed_run.size());
       ++i) {
    if (first_run[i] != different_seed_run[i]) {
      found_difference = true;
      break;
    }
  }
  app.expect_true(found_difference,
                  "different seed produces different shop items");
}

TEST(validate_seeded_rng_helper_methods) {
  // Test helper methods work correctly

  app.launch_game();

  auto &rng = SeededRng::get();
  rng.set_seed(9999);

  // Test gen_index
  std::vector<int> test_vec = {10, 20, 30, 40, 50};
  size_t idx1 = rng.gen_index(test_vec.size());
  size_t idx2 = rng.gen_index(test_vec.size());
  app.expect_true(idx1 < test_vec.size(), "gen_index returns valid index");
  app.expect_true(idx2 < test_vec.size(), "gen_index returns valid index");
  // With same seed, should get same sequence
  rng.set_seed(9999);
  size_t idx1_again = rng.gen_index(test_vec.size());
  app.expect_eq(static_cast<int>(idx1), static_cast<int>(idx1_again),
                "gen_index is deterministic");

  // Test gen_in_range
  rng.set_seed(8888);
  int val1 = rng.gen_in_range(1, 10);
  rng.set_seed(8888);
  int val2 = rng.gen_in_range(1, 10);
  app.expect_eq(val1, val2, "gen_in_range is deterministic");
  app.expect_true(val1 >= 1 && val1 <= 10, "gen_in_range within bounds");

  // Test gen_float
  rng.set_seed(7777);
  float f1 = rng.gen_float(0.0f, 1.0f);
  rng.set_seed(7777);
  float f2 = rng.gen_float(0.0f, 1.0f);
  app.expect_true(std::abs(f1 - f2) < 0.0001f, "gen_float is deterministic");
  app.expect_true(f1 >= 0.0f && f1 < 1.0f, "gen_float within bounds");

  // Test gen_mod
  rng.set_seed(6666);
  int mod1 = rng.gen_mod(5);
  rng.set_seed(6666);
  int mod2 = rng.gen_mod(5);
  app.expect_eq(mod1, mod2, "gen_mod is deterministic");
  app.expect_true(mod1 >= 0 && mod1 < 5, "gen_mod within bounds");
}
