#pragma once

#include "rl.h"
#include <string>

enum struct DishType {
  Potato = 0,
  //
  Burger,
  Burrito,
  Cheesecake,
  Donut,
  Dumplings,
  FriedEgg,
  FrenchFries,
  IceCream,
  LemonPie,
  MacNCheese,
  Meatball,
  Nacho,
  Omlet,
  Pancakes,
  Pizza,
  Ramen,
  RoastedChicken,
  Sandwich,
  Spaghetti,
  Steak,
  Sushi,
  Taco,
  Salmon,
  Bagel,
  Baguette,
  GarlicBread,
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

struct SpriteLocation {
  int i = 0;
  int j = 0;
};

struct DishInfo {
  std::string name;
  raylib::Color color{200, 200, 200, 255};
  int price = 1;
  FlavorStats flavor;
  SpriteLocation sprite;
};

DishInfo get_dish_info(DishType type);

// Shared pool of dishes used in shop and battle systems
const std::vector<DishType> &get_default_dish_pool();
