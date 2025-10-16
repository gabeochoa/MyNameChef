#pragma once

#include "trigger_event.h"
#include <afterhours/ah.h>
#include <vector>

struct TriggerQueue : afterhours::BaseComponent {
  std::vector<TriggerEvent> events;
  
  TriggerQueue() = default;
  
  void add_event(const TriggerEvent& event) {
    events.push_back(event);
  }
  
  void clear() {
    events.clear();
  }
  
  bool empty() const {
    return events.empty();
  }
  
  size_t size() const {
    return events.size();
  }
};
