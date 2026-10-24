#pragma once

#include <afterhours/ah.h>
#include <vector>

enum struct StatusEffectType {
  Heat,
  SweetTooth,
  Fatigue,
};

struct StatusEffect {
  StatusEffectType type = StatusEffectType::Heat;
  int zingDelta = 0; // Change to Zing stat
  int bodyDelta = 0; // Change to Body stat
  int duration = 0;  // Number of turns/ticks remaining (0 = permanent)
  int courseApplied = 0; // Course index when the effect was applied
};

struct StatusEffects : afterhours::BaseComponent {
  std::vector<StatusEffect> effects;
};
