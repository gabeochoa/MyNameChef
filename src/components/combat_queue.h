#pragma once

#include <afterhours/ah.h>

// Singleton driving course-by-course advancement
struct CombatQueue : afterhours::BaseComponent {
  int current_index = 0; // 0..6
  int total_courses = 7;
  bool complete = false;
};
