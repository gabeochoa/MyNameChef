#pragma once

#include "../rl.h"
#include <afterhours/ah.h>
#include <string>

struct IsDish : afterhours::BaseComponent {
  struct DishInfo {
    std::string name;
    raylib::Color color{200, 200, 200, 255};
    int price = 1; // Default price
  };

  DishInfo info;

  IsDish() = default;
  IsDish(DishInfo info) : info(info) {}

  const int price() const { return info.price; }
};
