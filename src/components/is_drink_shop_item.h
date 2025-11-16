#pragma once

#include "../drink_types.h"
#include <afterhours/ah.h>

struct IsDrinkShopItem : afterhours::BaseComponent {
  int slot = -1;
  DrinkType drink_type = DrinkType::Water;

  IsDrinkShopItem() = default;
  explicit IsDrinkShopItem(int s, DrinkType dt) : slot(s), drink_type(dt) {}
};

