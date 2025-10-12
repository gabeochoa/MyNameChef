#pragma once

#include "../dish_types.h"
#include <afterhours/ah.h>
#include <string>

struct IsDish : afterhours::BaseComponent {
  DishType type; // canonical identifier for this dish

  IsDish() = default;
  explicit IsDish(DishType t) : type(t) {}

  // Helper methods that delegate to dish_types registry
  std::string name() const;
  raylib::Color color() const;
  int price() const;
  FlavorStats flavor() const;
};