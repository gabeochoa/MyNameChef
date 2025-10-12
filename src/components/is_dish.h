#pragma once

#include "../rl.h"
#include <afterhours/ah.h>
#include <string>

struct IsDish : afterhours::BaseComponent {
  std::string name;
  raylib::Color color{200, 200, 200, 255};

  IsDish() = default;
  IsDish(std::string n, raylib::Color c) : name(std::move(n)), color(c) {}
};
