#pragma once

#include <afterhours/ah.h>
#include <string>
#include <vector>

struct SideEffectTracker : afterhours::BaseComponent {
  bool enabled = false;
  std::vector<std::string> violations;

  SideEffectTracker() = default;

  void reset() { violations.clear(); }

  void add_violation(const std::string &msg) { violations.push_back(msg); }
};
