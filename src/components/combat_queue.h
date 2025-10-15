#pragma once

#include <afterhours/ah.h>

struct CombatQueue : afterhours::BaseComponent {
  int current_index; // 0..6
  int total_courses;
  bool complete;

  CombatQueue() { reset(); }

  void reset() {
    current_index = 0;
    total_courses = 7;
    complete = false;
  }
};
