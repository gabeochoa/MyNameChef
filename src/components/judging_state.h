#pragma once

#include <afterhours/ah.h>

struct JudgingState : afterhours::BaseComponent {
  int current_index = -1;
  int total_courses = 0;
  int player_total = 0;
  int opponent_total = 0;
  bool complete = false;
  float timer = 0.0f;
  float per_course_delay = 1.0f;      // seconds between courses
  float post_complete_elapsed = 0.0f; // time waiting after completion
  bool transitioned = false;          // moved to results
};
