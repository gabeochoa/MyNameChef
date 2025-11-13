#pragma once

#include <afterhours/ah.h>
#include <afterhours/src/bitwise.h>

enum struct CuisineTagType : int {
  Thai = 1 << 0,
  Italian = 1 << 1,
  Japanese = 1 << 2,
  Mexican = 1 << 3,
  French = 1 << 4,
  Chinese = 1 << 5,
  Indian = 1 << 6,
  American = 1 << 7,
  Korean = 1 << 8,
  Vietnamese = 1 << 9,
};

struct CuisineTag : afterhours::BaseComponent {
  int flags = 0;

  CuisineTag() = default;
  explicit CuisineTag(CuisineTagType v) : flags(static_cast<int>(v)) {}
  explicit CuisineTag(int f) : flags(f) {}

  bool has(CuisineTagType tag) const {
    return (flags & static_cast<int>(tag)) != 0;
  }

  void add(CuisineTagType tag) { flags |= static_cast<int>(tag); }
};
