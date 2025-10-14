#pragma once

#include <afterhours/ah.h>

struct DishLevel : afterhours::BaseComponent {
  int level = 1;
  int merge_progress = 0; // 0-2, tracks merges needed for next level
  int merges_needed = 2;  // merges needed to level up (always 2 for now)

  DishLevel() = default;
  explicit DishLevel(int lvl) : level(lvl) {}

  bool can_level_up() const { return merge_progress >= merges_needed; }

  void add_merge() { add_merge_value(1); }

  int contribution_value() const {
    return 1 + (level - 1) * merges_needed + merge_progress;
  }

  void add_merge_value(int value) {
    int total = merge_progress + value;
    while (total >= merges_needed) {
      total -= merges_needed;
      level++;
    }
    merge_progress = total;
  }
};
