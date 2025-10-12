#pragma once

#include "../rl.h"
#include <afterhours/ah.h>

// Tag: entity is currently being held/dragged by the mouse
struct IsHeld : afterhours::BaseComponent {
  vec2 offset{0, 0}; // Offset from mouse position when first picked up

  IsHeld() = default;
  explicit IsHeld(vec2 off) : offset(off) {}
};
