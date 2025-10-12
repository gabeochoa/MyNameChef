#pragma once

#include "../rl.h"
#include <afterhours/ah.h>
#include <string>

struct FlavorStats {
  int satiety = 0;
  int sweetness = 0;
  int spice = 0;
  int acidity = 0;
  int umami = 0;
  int richness = 0;
  int freshness = 0;
};

struct IsDish : afterhours::BaseComponent {
  struct DishInfo {
    std::string name;
    raylib::Color color{200, 200, 200, 255};
    int price = 1; // Default price
    FlavorStats flavor;
  };

  DishInfo info;

  IsDish() = default;
  IsDish(DishInfo info) : info(info) {}

  const int price() const { return info.price; }
};
