#pragma once

#include "components/deferred_flavor_mods.h"
#include "components/dish_effect.h"
#include <string>
#include <vector>

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
  MisoSoup,
  Tempura,
  Risotto,
  Churros,
  Bouillabaisse,
  FoieGras,
  DebugDish,
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
  void applyMod(const DeferredFlavorMods &mod) {
    satiety += mod.satiety;
    sweetness += mod.sweetness;
    spice += mod.spice;
    acidity += mod.acidity;
    umami += mod.umami;
    richness += mod.richness;
    freshness += mod.freshness;
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
  std::vector<DishEffect> effects;
};

DishInfo get_dish_info(DishType type, int level = 1);

// Shared pool of dishes used in shop and battle systems
const std::vector<DishType> &get_default_dish_pool();

// Get dishes available at a specific tier or lower
const std::vector<DishType> &get_dishes_for_tier(int max_tier);

// Add tag components to a dish entity based on its DishType
void add_dish_tags(afterhours::Entity &entity, DishType type);
