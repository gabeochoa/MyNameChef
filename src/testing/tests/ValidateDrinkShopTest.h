#pragma once

#include "../../components/is_drink_shop_item.h"
#include "../../drink_types.h"
#include "../../query.h"
#include "../test_macros.h"

TEST(validate_drink_shop) {
  app.launch_game();
  app.set_drink_shop_override({DrinkType::Water, DrinkType::Water,
                               DrinkType::Water, DrinkType::Water});
  app.navigate_to_shop();

  app.wait_for_ui_exists("Next Round");
  app.wait_for_ui_exists("Reroll (1 gold)");

  app.wait_for_frames(10);

  int drink_count =
      static_cast<int>(afterhours::EntityQuery({.force_merge = true})
                           .whereHasComponent<IsDrinkShopItem>()
                           .gen_count());
  app.expect_count_gt(drink_count, 0, "drink shop items");
  app.expect_count_eq(drink_count, 4, "drink shop items should be 4");

  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsDrinkShopItem>()
           .gen()) {
    const IsDrinkShopItem &drink = entity.get<IsDrinkShopItem>();
    app.expect_true(drink.slot >= 0 && drink.slot < 4,
                    "drink slot should be 0-3");
    app.expect_true(drink.drink_type == DrinkType::Water,
                    "drink type should be Water");
  }
  app.clear_drink_shop_override();
}
