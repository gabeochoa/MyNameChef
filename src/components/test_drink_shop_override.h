#pragma once

#include "../drink_types.h"
#include <afterhours/ah.h>
#include <vector>

/**
 * Test-only component to override drink shop generation.
 * When present, GenerateDrinkShop will use the drinks in this list
 * instead of randomly generating them.
 */
struct TestDrinkShopOverride : afterhours::BaseComponent {
  std::vector<DrinkType> drinks;
  bool used = false; // Track if this override has been consumed

  TestDrinkShopOverride() = default;
  TestDrinkShopOverride(const std::vector<DrinkType> &drink_list)
      : drinks(drink_list), used(false) {}
};

