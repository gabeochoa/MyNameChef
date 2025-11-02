#pragma once

#include <afterhours/ah.h>

struct BattleSessionTag : afterhours::BaseComponent {
  uint32_t sessionId = 0;

  BattleSessionTag() = default;
  BattleSessionTag(uint32_t id) : sessionId(id) {}
};
