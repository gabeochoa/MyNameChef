#pragma once

#include "../rl.h"
#include <afterhours/ah.h>

// Tag: entity is currently being held/dragged by the mouse
struct IsHeld : afterhours::BaseComponent {
  vec2 offset{0, 0}; // Offset from mouse position when first picked up
  vec2 original_position{0, 0}; // Original position before being picked up

  IsHeld() = default;
  explicit IsHeld(vec2 off, vec2 orig_pos)
      : offset(off), original_position(orig_pos) {}
};
