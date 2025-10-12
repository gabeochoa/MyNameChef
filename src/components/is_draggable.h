#pragma once

#include <afterhours/ah.h>

// Tag: entity can be dragged by mouse
struct IsDraggable : afterhours::BaseComponent {
  bool enabled = true;

  IsDraggable() = default;
  explicit IsDraggable(bool e) : enabled(e) {}
};
