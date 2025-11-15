#pragma once

#include "cuisine_tag.h"
#include <afterhours/ah.h>
#include <set>
#include <utility>

struct AppliedSetBonuses : afterhours::BaseComponent {
  std::set<std::pair<CuisineTagType, int>> applied_cuisine;
};
