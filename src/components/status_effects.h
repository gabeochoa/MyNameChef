#pragma once

#include <afterhours/ah.h>
#include <vector>

struct StatusEffect {
  int zingDelta = 0; // Change to Zing stat
  int bodyDelta = 0; // Change to Body stat
  int duration = 0;  // Number of turns/ticks remaining (0 = permanent)
};

struct StatusEffects : afterhours::BaseComponent {
  std::vector<StatusEffect> effects;
};
