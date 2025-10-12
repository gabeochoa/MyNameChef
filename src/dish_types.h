#pragma once

#include "rl.h"
#include <string>
#include <optional>

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

// Accessors
DishInfo get_dish_info(DishType type);

// Data-driven loading API
// Loads dish definitions from resources/dishes.json. Safe to call multiple times.
bool load_dish_types_from_json();

// Optional helpers for conversions when working with data files
std::optional<DishType> dish_type_from_string(const std::string &name);
std::string dish_type_to_string(DishType type);
