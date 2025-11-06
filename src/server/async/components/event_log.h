#pragma once

#include "../battle_event.h"
#include "../types.h"
#include <afterhours/ah.h>
#include <vector>

namespace server::async {
struct EventLog : afterhours::BaseComponent {
  BattleId battleId;
  std::vector<DebugBattleEvent> events;
  float timestamp;

  EventLog() : timestamp(0.0f) {}
};
} // namespace server::async

