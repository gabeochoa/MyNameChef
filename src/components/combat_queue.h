#pragma once

#include <afterhours/ah.h>
#include <optional>

struct CombatQueue : afterhours::BaseComponent {
  int current_index; // 0..6
  int total_courses;
  bool complete;
  std::optional<int> current_player_dish_id;   // Track which dish is fighting for player side
  std::optional<int> current_opponent_dish_id; // Track which dish is fighting for opponent side

  CombatQueue() { reset(); }

  void reset() {
    current_index = 0;
    total_courses = 7;
    complete = false;
    current_player_dish_id = std::nullopt;
    current_opponent_dish_id = std::nullopt;
  }
};
