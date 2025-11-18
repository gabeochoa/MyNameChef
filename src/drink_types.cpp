#include "drink_types.h"
#include "log.h"
#include "seeded_rng.h"
#include <array>
#include <magic_enum/magic_enum.hpp>
#include <span>
#include <vector>

DrinkInfo get_drink_info(DrinkType type) {
  switch (type) {
  case DrinkType::Water:
    return DrinkInfo{"Water", 1, SpriteLocation{7, 5}};
  case DrinkType::OrangeJuice:
    return DrinkInfo{"Orange Juice", 1, SpriteLocation{2, 5}};
  case DrinkType::Coffee:
    return DrinkInfo{"Coffee", 2, SpriteLocation{0, 5}};
  case DrinkType::RedSoda:
    return DrinkInfo{"Red Soda", 2, SpriteLocation{5, 5}};
  case DrinkType::HotCocoa:
    return DrinkInfo{"Hot Cocoa", 2, SpriteLocation{1, 5}};
  case DrinkType::RedWine:
    return DrinkInfo{"Red Wine", 3, SpriteLocation{10, 5}};
  case DrinkType::GreenSoda:
    return DrinkInfo{"Green Soda", 2, SpriteLocation{4, 5}};
  case DrinkType::BlueSoda:
    return DrinkInfo{"Blue Soda", 2, SpriteLocation{3, 5}};
  case DrinkType::WhiteWine:
    return DrinkInfo{"White Wine", 3, SpriteLocation{11, 5}};
  case DrinkType::WatermelonJuice:
    return DrinkInfo{"Watermelon Juice", 3, SpriteLocation{8, 5}};
  case DrinkType::YellowSoda:
    return DrinkInfo{"Yellow Soda", 3, SpriteLocation{6, 5}};
  default:
    log_error("get_drink_info: Unhandled drink type");
    return DrinkInfo{"Water", 1, SpriteLocation{7, 5}};
  }
}

DrinkType get_random_drink() {
  auto all_drinks = magic_enum::enum_values<DrinkType>();
  auto &rng = SeededRng::get();
  return all_drinks[rng.gen_index(all_drinks.size())];
}

DrinkType get_random_drink_for_tier(int tier) {
  static const std::array<DrinkType, 4> tier1_drinks = {
      DrinkType::Water, DrinkType::OrangeJuice, DrinkType::Coffee,
      DrinkType::RedSoda};
  static const std::array<DrinkType, 8> tier1_2_drinks = {
      DrinkType::Water,    DrinkType::OrangeJuice, DrinkType::Coffee,
      DrinkType::RedSoda,  DrinkType::HotCocoa,    DrinkType::GreenSoda,
      DrinkType::BlueSoda, DrinkType::RedWine};
  static const std::array<DrinkType, 11> all_drinks = {
      DrinkType::Water,           DrinkType::OrangeJuice, DrinkType::Coffee,
      DrinkType::RedSoda,         DrinkType::HotCocoa,    DrinkType::GreenSoda,
      DrinkType::BlueSoda,        DrinkType::RedWine,     DrinkType::WhiteWine,
      DrinkType::WatermelonJuice, DrinkType::YellowSoda};

  std::span<const DrinkType> available_drinks;

  if (tier >= 3) {
    available_drinks = all_drinks;
  } else if (tier >= 2) {
    available_drinks = tier1_2_drinks;
  } else if (tier >= 1) {
    available_drinks = tier1_drinks;
  } else {
    log_warn("No drinks available for tier {}, falling back to Water", tier);
    return DrinkType::Water;
  }

  auto &rng = SeededRng::get();
  return available_drinks[rng.gen_index(available_drinks.size())];
}
