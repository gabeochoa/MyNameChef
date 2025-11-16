#pragma once

#include "../drink_types.h"
#include <afterhours/ah.h>
#include <optional>

struct DrinkPairing : afterhours::BaseComponent {
  std::optional<DrinkType> drink;

  DrinkPairing() = default;
  explicit DrinkPairing(DrinkType d) : drink(d) {}
};
