#pragma once

#include "dish_effect.h"
#include <afterhours/ah.h>
#include <vector>

struct DrinkEffects : afterhours::BaseComponent {
  std::vector<DishEffect> effects;
};
