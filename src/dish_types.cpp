#include "dish_types.h"

DishInfo get_dish_info(DishType type) {
  switch (type) {
  case DishType::Potato:
    return DishInfo{.name = "Potato",
                    .color = raylib::Color{200, 150, 100, 255},
                    .price = 1,
                    .flavor = FlavorStats{.satiety = 1}};
  case DishType::GarlicBread:
    return DishInfo{.name = "Garlic Bread",
                    .color = raylib::Color{180, 120, 80, 255},
                    .price = 3,
                    .flavor = FlavorStats{.satiety = 2, .richness = 2}};
  case DishType::TomatoSoup:
    return DishInfo{.name = "Tomato Soup",
                    .color = raylib::Color{160, 40, 40, 255},
                    .price = 3,
                    .flavor = FlavorStats{.acidity = 2, .freshness = 2}};
  case DishType::GrilledCheese:
    return DishInfo{.name = "Grilled Cheese",
                    .color = raylib::Color{200, 160, 60, 255},
                    .price = 3,
                    .flavor = FlavorStats{.richness = 2, .satiety = 2}};
  case DishType::ChickenSkewer:
    return DishInfo{.name = "Chicken Skewer",
                    .color = raylib::Color{150, 80, 60, 255},
                    .price = 3,
                    .flavor = FlavorStats{.spice = 2, .umami = 2}};
  case DishType::CucumberSalad:
    return DishInfo{.name = "Cucumber Salad",
                    .color = raylib::Color{60, 160, 100, 255},
                    .price = 3,
                    .flavor = FlavorStats{.freshness = 3}};
  case DishType::VanillaSoftServe:
    return DishInfo{.name = "Vanilla Soft Serve",
                    .color = raylib::Color{220, 200, 180, 255},
                    .price = 3,
                    .flavor = FlavorStats{.sweetness = 3, .richness = 1}};
  case DishType::CapreseSalad:
    return DishInfo{.name = "Caprese Salad",
                    .color = raylib::Color{120, 200, 140, 255},
                    .price = 3,
                    .flavor =
                        FlavorStats{.freshness = 2, .acidity = 1, .umami = 1}};
  case DishType::Minestrone:
    return DishInfo{.name = "Minestrone",
                    .color = raylib::Color{160, 90, 60, 255},
                    .price = 3,
                    .flavor = FlavorStats{.satiety = 2, .freshness = 1}};
  case DishType::SearedSalmon:
    return DishInfo{.name = "Seared Salmon",
                    .color = raylib::Color{230, 140, 90, 255},
                    .price = 3,
                    .flavor = FlavorStats{.umami = 3, .richness = 2}};
  case DishType::SteakFlorentine:
    return DishInfo{.name = "Steak Florentine",
                    .color = raylib::Color{160, 80, 80, 255},
                    .price = 3,
                    .flavor =
                        FlavorStats{.satiety = 3, .umami = 2, .richness = 2}};
  }
  return DishInfo{};
}