#pragma once

#include "trigger_event.h"
#include <afterhours/ah.h>
#include <vector>

struct TriggerQueue : afterhours::BaseComponent {
  std::vector<TriggerEvent> events;

  TriggerQueue() = default;

  // Prefer in-place construction to avoid copying non-copyable component types
  void add_event(TriggerHook hook, int sourceEntityId, int slotIndex,
                 DishBattleState::TeamSide teamSide) {
    events.emplace_back(hook, sourceEntityId, slotIndex, teamSide);
  }

  void clear() { events.clear(); }

  bool empty() const { return events.empty(); }

  size_t size() const { return events.size(); }
};
