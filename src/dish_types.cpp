#include "dish_types.h"

DishInfo get_dish_info(DishType type) {
  switch (type) {
  case DishType::Burger:
    return DishInfo{.name = "Burger",
                    .color = raylib::Color{200, 150, 100, 255},
                    .price = 3,
                    .flavor = FlavorStats{.satiety = 2, .richness = 1},
                    .sprite = SpriteLocation{8, 0}}; // 15_burger.png x=256,y=0
  case DishType::Burrito:
    return DishInfo{.name = "Burrito",
                    .color = raylib::Color{190, 140, 90, 255},
                    .price = 3,
                    .flavor = FlavorStats{.satiety = 2, .umami = 1},
                    .sprite =
                        SpriteLocation{11, 0}}; // 18_burrito.png x=352,y=0
  case DishType::Cheesecake:
    return DishInfo{.name = "Cheesecake",
                    .color = raylib::Color{230, 210, 180, 255},
                    .price = 3,
                    .flavor = FlavorStats{.sweetness = 3, .richness = 2},
                    .sprite =
                        SpriteLocation{2, 1}}; // 22_cheesecake.png x=64,y=32
  case DishType::Donut:
    return DishInfo{.name = "Donut",
                    .color = raylib::Color{210, 170, 120, 255},
                    .price = 2,
                    .flavor = FlavorStats{.sweetness = 2, .richness = 1},
                    .sprite = SpriteLocation{10, 1}}; // 34_donut.png x=320,y=32
  case DishType::Dumplings:
    return DishInfo{.name = "Dumplings",
                    .color = raylib::Color{200, 180, 150, 255},
                    .price = 3,
                    .flavor = FlavorStats{.satiety = 2, .umami = 1},
                    .sprite =
                        SpriteLocation{17, 0}}; // 36_dumplings.png x=544,y=0
  case DishType::FriedEgg:
    return DishInfo{.name = "Fried Egg",
                    .color = raylib::Color{245, 230, 140, 255},
                    .price = 2,
                    .flavor = FlavorStats{.richness = 1},
                    .sprite =
                        SpriteLocation{2, 7}}; // 38_friedegg.png x=64,y=224
  case DishType::FrenchFries:
    return DishInfo{.name = "French Fries",
                    .color = raylib::Color{240, 210, 90, 255},
                    .price = 2,
                    .flavor = FlavorStats{.satiety = 1, .richness = 1},
                    .sprite =
                        SpriteLocation{1, 9}}; // 44_frenchfries.png x=32,y=288
  case DishType::IceCream:
    return DishInfo{.name = "Ice Cream",
                    .color = raylib::Color{240, 220, 210, 255},
                    .price = 2,
                    .flavor = FlavorStats{.sweetness = 3},
                    .sprite =
                        SpriteLocation{18, 1}}; // 57_icecream.png x=576,y=32
  case DishType::LemonPie:
    return DishInfo{.name = "Lemon Pie",
                    .color = raylib::Color{245, 225, 140, 255},
                    .price = 3,
                    .flavor = FlavorStats{.sweetness = 2, .acidity = 1},
                    .sprite =
                        SpriteLocation{22, 1}}; // 63_lemonpie.png x=704,y=32
  case DishType::MacNCheese:
    return DishInfo{.name = "Mac 'n' Cheese",
                    .color = raylib::Color{255, 220, 120, 255},
                    .price = 3,
                    .flavor = FlavorStats{.satiety = 2, .richness = 2},
                    .sprite =
                        SpriteLocation{25, 0}}; // 67_macncheese.png x=800,y=0
  case DishType::Meatball:
    return DishInfo{.name = "Meatball",
                    .color = raylib::Color{180, 100, 80, 255},
                    .price = 2,
                    .flavor = FlavorStats{.umami = 2, .satiety = 1},
                    .sprite =
                        SpriteLocation{27, 0}}; // 69_meatball.png x=864,y=0
  case DishType::Nacho:
    return DishInfo{.name = "Nacho",
                    .color = raylib::Color{240, 200, 80, 255},
                    .price = 2,
                    .flavor = FlavorStats{.satiety = 1, .richness = 1},
                    .sprite = SpriteLocation{29, 0}}; // 71_nacho.png x=928,y=0
  case DishType::Omlet:
    return DishInfo{.name = "Omlet",
                    .color = raylib::Color{250, 230, 150, 255},
                    .price = 3,
                    .flavor = FlavorStats{.satiety = 1, .richness = 1},
                    .sprite = SpriteLocation{31, 0}}; // 73_omlet.png x=992,y=0
  case DishType::Pancakes:
    return DishInfo{.name = "Pancakes",
                    .color = raylib::Color{240, 210, 150, 255},
                    .price = 3,
                    .flavor = FlavorStats{.sweetness = 1, .satiety = 1},
                    .sprite =
                        SpriteLocation{33, 0}}; // 79_pancakes.png x=1056,y=0
  case DishType::Pizza:
    return DishInfo{.name = "Pizza",
                    .color = raylib::Color{220, 160, 100, 255},
                    .price = 4,
                    .flavor = FlavorStats{.satiety = 2, .umami = 1},
                    .sprite = SpriteLocation{35, 0}}; // 81_pizza.png x=1120,y=0
  case DishType::Ramen:
    return DishInfo{.name = "Ramen",
                    .color = raylib::Color{210, 170, 120, 255},
                    .price = 4,
                    .flavor = FlavorStats{.satiety = 2, .umami = 2},
                    .sprite = SpriteLocation{39, 0}}; // 87_ramen.png x=1248,y=0
  case DishType::RoastedChicken:
    return DishInfo{
        .name = "Roasted Chicken",
        .color = raylib::Color{220, 150, 100, 255},
        .price = 4,
        .flavor = FlavorStats{.satiety = 2, .umami = 2},
        .sprite = SpriteLocation{37, 0}}; // 85_roastedchicken.png x=1184,y=0
  case DishType::Sandwich:
    return DishInfo{.name = "Sandwich",
                    .color = raylib::Color{210, 180, 120, 255},
                    .price = 3,
                    .flavor = FlavorStats{.satiety = 2},
                    .sprite =
                        SpriteLocation{40, 0}}; // 92_sandwich.png x=1280,y=0
  case DishType::Spaghetti:
    return DishInfo{.name = "Spaghetti",
                    .color = raylib::Color{210, 140, 100, 255},
                    .price = 3,
                    .flavor = FlavorStats{.satiety = 2, .umami = 1},
                    .sprite =
                        SpriteLocation{42, 0}}; // 94_spaghetti.png x=1344,y=0
  case DishType::Steak:
    return DishInfo{.name = "Steak",
                    .color = raylib::Color{170, 90, 80, 255},
                    .price = 4,
                    .flavor = FlavorStats{.satiety = 3, .umami = 2},
                    .sprite = SpriteLocation{43, 0}}; // 95_steak.png x=1376,y=0
  case DishType::Sushi:
    return DishInfo{.name = "Sushi",
                    .color = raylib::Color{200, 200, 200, 255},
                    .price = 4,
                    .flavor = FlavorStats{.freshness = 2, .umami = 2},
                    .sprite = SpriteLocation{45, 0}}; // 97_sushi.png x=1440,y=0
  case DishType::Taco:
    return DishInfo{.name = "Taco",
                    .color = raylib::Color{220, 170, 90, 255},
                    .price = 3,
                    .flavor = FlavorStats{.satiety = 2, .spice = 1},
                    .sprite = SpriteLocation{47, 0}}; // 99_taco.png x=1504,y=0
  case DishType::Salmon:
    return DishInfo{.name = "Salmon",
                    .color = raylib::Color{230, 140, 90, 255},
                    .price = 4,
                    .flavor = FlavorStats{.umami = 3, .freshness = 2},
                    .sprite =
                        SpriteLocation{6, 7}}; // 88_salmon.png x=192,y=224
  case DishType::Bagel:
    return DishInfo{.name = "Bagel",
                    .color = raylib::Color{210, 170, 120, 255},
                    .price = 2,
                    .flavor = FlavorStats{.satiety = 1},
                    .sprite = SpriteLocation{13, 0}}; // 20_bagel.png x=416,y=0
  case DishType::Baguette:
    return DishInfo{.name = "Baguette",
                    .color = raylib::Color{220, 180, 120, 255},
                    .price = 2,
                    .flavor = FlavorStats{.satiety = 1},
                    .sprite = SpriteLocation{1, 0}}; // 09_baguette.png x=32,y=0
  case DishType::GarlicBread:
    return DishInfo{
        .name = "Garlic Bread",
        .color = raylib::Color{230, 200, 120, 255},
        .price = 3,
        .flavor = FlavorStats{.satiety = 2, .richness = 2},
        .sprite = SpriteLocation{0, 9}}; // 07_bread.png x=0,y=288 (placeholder)
  case DishType::Potato:
    return DishInfo{.name = "Potato",
                    .color = raylib::Color{200, 150, 100, 255},
                    .price = 1,
                    .flavor = FlavorStats{.satiety = 1},
                    .sprite = SpriteLocation{13, 3}}; // potato.png x=416,y=96
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