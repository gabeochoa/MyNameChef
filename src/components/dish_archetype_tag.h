#pragma once

#include <afterhours/ah.h>
#include <afterhours/src/bitwise.h>

enum struct DishArchetypeTagType : int {
  Bread = 1 << 0,
  Dairy = 1 << 1,
  Beverage = 1 << 2,
  Wine = 1 << 3,
  Coffee = 1 << 4,
  Tea = 1 << 5,
  Side = 1 << 6,
  Sauce = 1 << 7,
  Protein = 1 << 8,
  Vegetable = 1 << 9,
  Grain = 1 << 10,
};

struct DishArchetypeTag : afterhours::BaseComponent {
  int flags = 0;

  DishArchetypeTag() = default;
  explicit DishArchetypeTag(DishArchetypeTagType v) : flags(static_cast<int>(v)) {}
  explicit DishArchetypeTag(int f) : flags(f) {}

  bool has(DishArchetypeTagType tag) const {
    return (flags & static_cast<int>(tag)) != 0;
  }

  void add(DishArchetypeTagType tag) {
    flags |= static_cast<int>(tag);
  }
};

