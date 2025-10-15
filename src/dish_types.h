#pragma once

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
  inline int zing() const {
    int total = 0;
    total += spice;
    total += acidity;
    total += umami;
    return total;
  }
  inline int body() const {
    int total = 0;
    total += satiety;
    total += richness;
    total += sweetness;
    total += freshness;
    return total;
  }
};

struct SpriteLocation {
  int i = 0;
  int j = 0;
};

struct DishInfo {
  std::string name;
  int price = 1;
  int tier = 1;
  FlavorStats flavor;
  SpriteLocation sprite;
};

DishInfo get_dish_info(DishType type);

// Shared pool of dishes used in shop and battle systems
const std::vector<DishType> &get_default_dish_pool();

// Get dishes available at a specific tier or lower
const std::vector<DishType> &get_dishes_for_tier(int max_tier);
