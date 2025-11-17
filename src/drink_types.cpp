#include "drink_types.h"
#include "log.h"

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
  default:
    log_error("get_drink_info: Unhandled drink type");
    return DrinkInfo{"Water", 1, SpriteLocation{7, 5}};
  }
}

DrinkType get_random_drink() {
  // TODO: Add more drink types (soda, juice, etc.)
  return DrinkType::Water;
}
