#pragma once

#include <afterhours/ah.h>

enum struct CourseTagType : int {
  Appetizer = 1 << 0,
  Soup = 1 << 1,
  Entree = 1 << 2,
  Drink = 1 << 3,
  Dessert = 1 << 4,
  Topping = 1 << 5,
};

struct CourseTag : afterhours::BaseComponent {
  int flags = 0;

  CourseTag() = default;
  explicit CourseTag(CourseTagType v) : flags(static_cast<int>(v)) {}
  explicit CourseTag(int f) : flags(f) {}

  bool has(CourseTagType tag) const {
    return (flags & static_cast<int>(tag)) != 0;
  }

  void add(CourseTagType tag) { flags |= static_cast<int>(tag); }
};
