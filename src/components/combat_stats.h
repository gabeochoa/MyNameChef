#pragma once

#include <afterhours/ah.h>

struct CombatStats : afterhours::BaseComponent {
  int baseZing = 0;
  int baseBody = 0;
  int currentZing = 0;
  int currentBody = 0;
};
