#pragma once

#include "dish_types.h"
#include <cstddef>
#include <string>

enum struct DrinkType {
  Water = 0,
};

struct DrinkInfo {
  std::string name;
  int price = 1;
  SpriteLocation sprite;
};

DrinkInfo get_drink_info(DrinkType type);
DrinkType get_random_drink();
