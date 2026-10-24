#pragma once

#include <afterhours/ah.h>

struct MergeFlash : afterhours::BaseComponent {
  float duration = 1.0f;
  float elapsed = 0.0f;
  bool leveledUp = false;
};
