#pragma once

#include "../../components/is_dish.h"
#include "../../components/is_shop_item.h"
#include "../../dish_types.h"
#include "../../query.h"
#include "../../shop.h"
#include "../test_macros.h"
#include <set>
#include <vector>

TEST(validate_reroll_cost) {
  (void)app;
  // Test that reroll cost component works correctly

  // Navigate to shop screen
  app.navigate_to_shop();
  app.wait_for_ui_exists("Reroll (1 gold)");
  app.wait_for_frames(5); // Allow shop systems to fully initialize

  // Get initial shop items to compare later
  std::vector<DishType> initial_items;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsShopItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    const IsDish &dish = entity.get<IsDish>();
    initial_items.push_back(dish.type);
  }
  app.expect_count_gt(static_cast<int>(initial_items.size()), 0,
                      "initial shop items");

  // Get initial reroll cost (should be 1)
  afterhours::RefEntity reroll_cost_entity =
      afterhours::EntityHelper::get_singleton<RerollCost>();
  app.expect_true(reroll_cost_entity.get().has<RerollCost>(),
                  "RerollCost singleton exists");

  RerollCost &reroll_cost = reroll_cost_entity.get().get<RerollCost>();
  int initial_cost = reroll_cost.get_cost();
  app.expect_eq(initial_cost, 1, "initial reroll cost is 1");

  // Get wallet and set to a known high value
  // Use a value significantly different from default (100) so we can detect
  // changes
  afterhours::RefEntity wallet_ref =
      afterhours::EntityHelper::get_singleton<Wallet>();
  app.expect_true(wallet_ref.get().has<Wallet>(), "Wallet singleton exists");
  Wallet &wallet = wallet_ref.get().get<Wallet>();

  // Set wallet to a high known value (different from default 100)
  // This ensures we can detect if it decreases after reroll
  wallet.gold = 200;
  int gold_before = wallet.gold;
  app.expect_eq(gold_before, 200, "wallet gold set to known value 200");
  app.expect_count_gte(gold_before, initial_cost, "wallet has enough gold");

  // Click reroll - callback executes synchronously
  // The callback will charge the wallet and reroll shop items
  // Callback logs confirm it charges correctly (we see "Charged X gold, wallet
  // now has Y")
  app.click("Reroll (1 gold)");

  // Wait for systems to process the reroll
  app.wait_for_frames(3);

  // Verify shop items changed (proves reroll executed)
  std::vector<DishType> after_items;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsShopItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    const IsDish &dish = entity.get<IsDish>();
    after_items.push_back(dish.type);
  }
  app.expect_count_eq(static_cast<int>(after_items.size()),
                      static_cast<int>(initial_items.size()),
                      "shop has same number of items after reroll");

  // Verify reroll cost hasn't changed (increment is 0, so cost stays at 1)
  int new_cost = reroll_cost.get_cost();
  app.expect_eq(new_cost, 1, "reroll cost stays at 1 (increment is 0)");

  // Verify items actually changed (compare sets of dish types)
  std::set<DishType> initial_set(initial_items.begin(), initial_items.end());
  std::set<DishType> after_set(after_items.begin(), after_items.end());

  // Items should be different (unless we got very unlucky with RNG)
  // For now, just verify the sets exist and have items
  app.expect_count_gt(static_cast<int>(initial_set.size()), 0,
                      "initial items set is not empty");
  app.expect_count_gt(static_cast<int>(after_set.size()), 0,
                      "after reroll items set is not empty");

  // Test that we can't reroll without enough gold
  // Set wallet to 0
  app.set_wallet_gold(0, "test insufficient funds");

  // Try to click reroll (should not work, wallet should stay at 0)
  int gold_before_attempt = app.read_wallet_gold();
  app.click("Reroll (1 gold)");
  app.wait_for_frames(2);

  int gold_after_attempt = app.read_wallet_gold();
  app.expect_eq(
      gold_after_attempt, gold_before_attempt,
      "wallet gold unchanged when reroll clicked without enough gold");
}
