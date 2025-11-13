#pragma once

#include <afterhours/ah.h>
#include <afterhours/src/bitwise.h>

enum struct BrandTagType : int {
  LocalFarm = 1 << 0,
  StreetFood = 1 << 1,
  Gourmet = 1 << 2,
  FastFood = 1 << 3,
  Homemade = 1 << 4,
  Restaurant = 1 << 5,
  Bakery = 1 << 6,
};

struct BrandTag : afterhours::BaseComponent {
  int flags = 0;

  BrandTag() = default;
  explicit BrandTag(BrandTagType v) : flags(static_cast<int>(v)) {}
  explicit BrandTag(int f) : flags(f) {}

  bool has(BrandTagType tag) const {
    return (flags & static_cast<int>(tag)) != 0;
  }

  void add(BrandTagType tag) { flags |= static_cast<int>(tag); }
};
