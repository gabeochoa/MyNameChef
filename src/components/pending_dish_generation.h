#pragma once

#include <afterhours/ah.h>

struct PendingDishGeneration : afterhours::BaseComponent {
  afterhours::EntityID generatingEntityId;
  float delayRemaining;
  int targetSlot;
  bool isPlayerTeam;

  PendingDishGeneration() { reset(); }

  void reset() {
    generatingEntityId = 0;
    delayRemaining = 0.0f;
    targetSlot = -1; // -1 means auto-determine based on position
    isPlayerTeam = true;
  }
};
