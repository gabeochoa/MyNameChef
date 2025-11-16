#pragma once

#include <cstddef>
#include <string>

enum struct DrinkType {
  Water = 0,
};

struct DrinkInfo {
  std::string name;
  int price = 1;
  struct {
    int i; // sprite column (x / 32)
    int j; // sprite row (y / 32)
  } sprite;
};

DrinkInfo get_drink_info(DrinkType type);
DrinkType get_random_drink();
