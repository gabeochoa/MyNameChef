#pragma once

#include <afterhours/ah.h>

struct TriggerAnimationState : afterhours::BaseComponent {
  int active = 0;       // number of active trigger animations
  bool running = false; // convenience mirror of (active > 0)
};
