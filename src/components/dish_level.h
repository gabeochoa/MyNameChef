#pragma once

#include <afterhours/ah.h>

struct DishLevel : afterhours::BaseComponent {
  int level = 1;
  int merge_progress = 0; // 0-2, tracks merges needed for next level
  int merges_needed =
      2; // merges needed to level up (2 level Ns make 1 level N+1)

  DishLevel() = default;
  explicit DishLevel(int lvl) : level(lvl) {}

  bool can_level_up() const { return merge_progress >= merges_needed; }

  void add_merge() { add_merge_value(1); }

  int contribution_value() const {
    int base_value = 1;
    for (int i = 1; i < level; ++i) {
      base_value *= merges_needed;
    }
    int progress_value = (level == 1)
                             ? merge_progress
                             : merge_progress * (base_value / merges_needed);
    return base_value + progress_value;
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
