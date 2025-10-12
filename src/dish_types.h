#pragma once

#include "rl.h"
#include <string>

enum struct DishType {
  Potato = 0,
  GarlicBread,
  TomatoSoup,
  GrilledCheese,
  ChickenSkewer,
  CucumberSalad,
  VanillaSoftServe,
  CapreseSalad,
  Minestrone,
  SearedSalmon,
  SteakFlorentine,
};

struct FlavorStats {
  int satiety = 0;
  int sweetness = 0;
  int spice = 0;
  int acidity = 0;
  int umami = 0;
  int richness = 0;
  int freshness = 0;
};

struct DishInfo {
  std::string name;
  raylib::Color color{200, 200, 200, 255};
  int price = 1;
  FlavorStats flavor;
};

DishInfo get_dish_info(DishType type);
