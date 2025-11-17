#pragma once

#include <afterhours/ah.h>

struct NextDamageEffect : afterhours::BaseComponent {
  float multiplier = 1.0f;
  int flatModifier = 0;
  int count = 1;
};
