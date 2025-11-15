#pragma once

#include "cuisine_tag.h"
#include "dish_effect.h"
#include <cstddef>
#include <map>

struct SetBonusDefinition {
  TargetScope targetScope;
  int zingDelta;
  int bodyDelta;
  const char *description;
  const DishEffect *bonusEffects;
  size_t bonusEffectsCount;

  constexpr SetBonusDefinition(TargetScope scope, int zing, int body,
                               const char *desc,
                               const DishEffect *effects = nullptr,
                               size_t effectsCount = 0)
      : targetScope(scope), zingDelta(zing), bodyDelta(body), description(desc),
        bonusEffects(effects), bonusEffectsCount(effectsCount) {}
};

namespace synergy_bonuses {
const std::map<CuisineTagType, std::map<int, SetBonusDefinition>> &
get_cuisine_bonus_definitions();
} // namespace synergy_bonuses
