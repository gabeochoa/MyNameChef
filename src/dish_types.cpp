#include "dish_types.h"
#include <vector>

// All dishes now have the same price
static constexpr int kUnifiedDishPrice = 3;

static DishInfo make_dish(const char *name, FlavorStats flavor,
                          SpriteLocation sprite, int tier = 2) {
  DishInfo result;
  result.name = name;
  result.price = kUnifiedDishPrice; // Always 3
  result.tier = tier;
  result.flavor = flavor;
  result.sprite = sprite;
  return result;
}

DishInfo get_dish_info(DishType type) {
  switch (type) {
  // Tier 1: Raw/Single Ingredient
  case DishType::Potato:
    return make_dish("Potato", FlavorStats{.satiety = 1}, SpriteLocation{13, 3},
                     1); // potato.png x=416,y=96 - Tier 1
  case DishType::Salmon:
    return make_dish("Salmon", FlavorStats{.umami = 3, .freshness = 2},
                     SpriteLocation{6, 7},
                     1); // 88_salmon.png x=192,y=224 - Tier 1
  case DishType::Bagel:
    return make_dish("Bagel", FlavorStats{.satiety = 1}, SpriteLocation{13, 0},
                     1); // 20_bagel.png x=416,y=0 - Tier 1
  case DishType::Baguette:
    return make_dish("Baguette", FlavorStats{.satiety = 1},
                     SpriteLocation{1, 0},
                     1); // 09_baguette.png x=32,y=0 - Tier 1
  case DishType::GarlicBread:
    return make_dish("Garlic Bread", FlavorStats{.satiety = 1, .richness = 1},
                     SpriteLocation{0, 9},
                     1); // 07_bread.png x=0,y=288 - Tier 1
  case DishType::FriedEgg:
    return make_dish("Fried Egg", FlavorStats{.richness = 1},
                     SpriteLocation{2, 7},
                     1); // 38_friedegg.png x=64,y=224 - Tier 1
  case DishType::FrenchFries:
    return make_dish("French Fries", FlavorStats{.satiety = 1, .richness = 1},
                     SpriteLocation{1, 9},
                     1); // 44_frenchfries.png x=32,y=288 - Tier 1

  // Tier 2: Simple 2-ingredient items
  case DishType::Pancakes:
    return make_dish("Pancakes", FlavorStats{.sweetness = 1, .satiety = 1},
                     SpriteLocation{33, 0},
                     2); // 79_pancakes.png x=1056,y=0 - Tier 2
  case DishType::Sandwich:
    return make_dish("Sandwich", FlavorStats{.satiety = 2},
                     SpriteLocation{40, 0},
                     2); // 92_sandwich.png x=1280,y=0 - Tier 2
  case DishType::Nacho:
    return make_dish("Nacho", FlavorStats{.satiety = 1, .richness = 1},
                     SpriteLocation{29, 0},
                     2); // 71_nacho.png x=928,y=0 - Tier 2

  // Tier 3: Moderate complexity
  case DishType::Burger:
    return make_dish("Burger", FlavorStats{.satiety = 2, .richness = 1},
                     SpriteLocation{8, 0},
                     3); // 15_burger.png x=256,y=0 - Tier 3
  case DishType::Taco:
    return make_dish("Taco", FlavorStats{.satiety = 2, .spice = 1},
                     SpriteLocation{47, 0},
                     3); // 99_taco.png x=1504,y=0 - Tier 3
  case DishType::Omlet:
    return make_dish("Omlet", FlavorStats{.satiety = 1, .richness = 1},
                     SpriteLocation{31, 0},
                     3); // 73_omlet.png x=992,y=0 - Tier 3
  case DishType::IceCream:
    return make_dish("Ice Cream", FlavorStats{.sweetness = 3},
                     SpriteLocation{18, 1},
                     3); // 57_icecream.png x=576,y=32 - Tier 3

  // Tier 4: Complex dishes
  case DishType::Pizza:
    return make_dish("Pizza", FlavorStats{.satiety = 2, .umami = 1},
                     SpriteLocation{35, 0},
                     4); // 81_pizza.png x=1120,y=0 - Tier 4
  case DishType::Ramen:
    return make_dish("Ramen", FlavorStats{.satiety = 2, .umami = 2},
                     SpriteLocation{39, 0},
                     4); // 87_ramen.png x=1248,y=0 - Tier 4
  case DishType::MacNCheese:
    return make_dish("Mac 'n' Cheese", FlavorStats{.satiety = 2, .richness = 2},
                     SpriteLocation{25, 0},
                     4); // 67_macncheese.png x=800,y=0 - Tier 4
  case DishType::Dumplings:
    return make_dish("Dumplings", FlavorStats{.satiety = 2, .umami = 1},
                     SpriteLocation{17, 0},
                     4); // 36_dumplings.png x=544,y=0 - Tier 4
  case DishType::Burrito:
    return make_dish("Burrito", FlavorStats{.satiety = 2, .umami = 1},
                     SpriteLocation{11, 0},
                     4); // 18_burrito.png x=352,y=0 - Tier 4
  case DishType::Spaghetti:
    return make_dish("Spaghetti", FlavorStats{.satiety = 2, .umami = 1},
                     SpriteLocation{42, 0},
                     4); // 94_spaghetti.png x=1344,y=0 - Tier 4
  case DishType::RoastedChicken:
    return make_dish("Roasted Chicken", FlavorStats{.satiety = 2, .umami = 2},
                     SpriteLocation{37, 0},
                     4); // 85_roastedchicken.png x=1184,y=0 - Tier 4

  // Tier 5: Most complex/Gourmet
  case DishType::Steak:
    return make_dish("Steak", FlavorStats{.satiety = 3, .umami = 2},
                     SpriteLocation{43, 0},
                     5); // 95_steak.png x=1376,y=0 - Tier 5
  case DishType::Sushi:
    return make_dish("Sushi", FlavorStats{.freshness = 2, .umami = 2},
                     SpriteLocation{45, 0},
                     5); // 97_sushi.png x=1440,y=0 - Tier 5
  case DishType::Cheesecake:
    return make_dish("Cheesecake", FlavorStats{.sweetness = 3, .richness = 2},
                     SpriteLocation{2, 1},
                     5); // 22_cheesecake.png x=64,y=32 - Tier 5
  case DishType::LemonPie:
    return make_dish("Lemon Pie", FlavorStats{.sweetness = 2, .acidity = 1},
                     SpriteLocation{22, 1},
                     5); // 63_lemonpie.png x=704,y=32 - Tier 5
  case DishType::Donut:
    return make_dish("Donut", FlavorStats{.sweetness = 2, .richness = 1},
                     SpriteLocation{10, 1},
                     5); // 34_donut.png x=320,y=32 - Tier 5
  case DishType::Meatball:
    return make_dish("Meatball", FlavorStats{.umami = 2, .satiety = 1},
                     SpriteLocation{27, 0},
                     5); // 69_meatball.png x=864,y=0 - Tier 5
  }
  return DishInfo{};
}

const std::vector<DishType> &get_default_dish_pool() {
  static const std::vector<DishType> pool = {
      DishType::Burger,      DishType::Burrito,        DishType::Cheesecake,
      DishType::Donut,       DishType::Dumplings,      DishType::FriedEgg,
      DishType::FrenchFries, DishType::IceCream,       DishType::LemonPie,
      DishType::MacNCheese,  DishType::Meatball,       DishType::Nacho,
      DishType::Omlet,       DishType::Pancakes,       DishType::Pizza,
      DishType::Ramen,       DishType::RoastedChicken, DishType::Sandwich,
      DishType::Spaghetti,   DishType::Steak,          DishType::Sushi,
      DishType::Taco,        DishType::Salmon,         DishType::Bagel,
      DishType::Baguette,    DishType::GarlicBread,    DishType::Potato,
  };
  return pool;
}

const std::vector<DishType> &get_dishes_for_tier(int max_tier) {
  static std::vector<DishType> tier_pool;
  tier_pool.clear();

  const auto &all_dishes = get_default_dish_pool();
  for (const auto &dish : all_dishes) {
    auto info = get_dish_info(dish);
    if (info.tier <= max_tier) {
      tier_pool.push_back(dish);
    }
  }

  return tier_pool;
}