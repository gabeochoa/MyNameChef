#pragma once

#include "cuisine_tag.h"
#include <afterhours/ah.h>
#include <map>

struct BattleSynergyCounts : afterhours::BaseComponent {
  std::map<CuisineTagType, int> player_cuisine_counts;
  std::map<CuisineTagType, int> opponent_cuisine_counts;
};
