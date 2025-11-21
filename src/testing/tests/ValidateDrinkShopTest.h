#pragma once

#include "../../components/is_drink_shop_item.h"
#include "../../query.h"
#include "../test_macros.h"

TEST(validate_drink_shop) {
  // Navigate to shop screen
  app.launch_game();
  app.navigate_to_shop();

  // Verify UI elements exist
  app.wait_for_ui_exists("Next Round");
  app.wait_for_ui_exists("Reroll (1 gold)");

  // Wait for systems to create drink shop items
  app.wait_for_frames(10);

  // Query for drink shop items
  int drink_count = static_cast<int>(afterhours::EntityQuery()
                                         .whereHasComponent<IsDrinkShopItem>()
                                         .gen_count());
  app.expect_count_gt(drink_count, 0, "drink shop items");

  // Verify we have the expected number of drinks (2x2 grid = 4 drinks)
  app.expect_count_eq(drink_count, 4, "drink shop items should be 4");

  // Verify each drink has valid slot and drink type
  for (afterhours::Entity &entity :
       afterhours::EntityQuery().whereHasComponent<IsDrinkShopItem>().gen()) {
    const IsDrinkShopItem &drink = entity.get<IsDrinkShopItem>();
    app.expect_true(drink.slot >= 0 && drink.slot < 4,
                    "drink slot should be 0-3");
    app.expect_true(drink.drink_type == DrinkType::Water,
                    "drink type should be Water");
  }
}
