#pragma once

#include <afterhours/ah.h>

// Tag: entity can have items dropped onto it
struct CanDropOnto : afterhours::BaseComponent {
  bool enabled = true;

  CanDropOnto() = default;
  explicit CanDropOnto(bool e) : enabled(e) {}
};
