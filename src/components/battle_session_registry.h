#pragma once

#include <afterhours/ah.h>
#include <vector>

struct BattleSessionRegistry : afterhours::BaseComponent {
  uint32_t sessionId =
      0; // Current battle session ID (increments for each new battle/replay)
  std::vector<int> ownedEntityIds;

  BattleSessionRegistry() = default;

  void reset() {
    sessionId = 0;
    ownedEntityIds.clear();
  }
};
