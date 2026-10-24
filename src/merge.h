#pragma once

#include "components/dish_level.h"
#include "components/is_dish.h"
#include "components/is_inventory_item.h"
#include "query.h"
#include <afterhours/ah.h>

namespace merge {

inline afterhours::OptEntity find_triple_merge_dish(
    const afterhours::Entity &target_item,
    const afterhours::Entity &donor) {
  if (!target_item.has<IsInventoryItem>() || !target_item.has<IsDish>() ||
      !target_item.has<DishLevel>()) {
    return afterhours::OptEntity();
  }
  int target_slot = target_item.get<IsInventoryItem>().slot;
  DishType target_type = target_item.get<IsDish>().type;
  int target_level = target_item.get<DishLevel>().level;
  int candidate_slots[2] = {target_slot - 1, target_slot + 1};
  for (int candidate_slot : candidate_slots) {
    if (candidate_slot < 0) {
      continue;
    }
    for (auto &ref : EQ().whereHasComponent<IsInventoryItem>()
                         .whereHasComponent<IsDish>()
                         .whereHasComponent<DishLevel>()
                         .gen()) {
      afterhours::Entity &candidate = ref.get();
      if (candidate.id == target_item.id || candidate.id == donor.id) {
        continue;
      }
      if (candidate.get<IsInventoryItem>().slot != candidate_slot) {
        continue;
      }
      if (candidate.get<IsDish>().type != target_type) {
        continue;
      }
      if (candidate.get<DishLevel>().level > target_level) {
        continue;
      }
      return afterhours::OptEntity(candidate);
    }
  }
  return afterhours::OptEntity();
}

inline bool dish_has_merge_partner(const afterhours::Entity &dish) {
  if (!dish.has<IsInventoryItem>() || !dish.has<IsDish>() ||
      !dish.has<DishLevel>()) {
    return false;
  }
  DishType type = dish.get<IsDish>().type;
  for (auto &ref : EQ().whereHasComponent<IsInventoryItem>()
                       .whereHasComponent<IsDish>()
                       .whereHasComponent<DishLevel>()
                       .gen()) {
    afterhours::Entity &candidate = ref.get();
    if (candidate.id == dish.id) {
      continue;
    }
    if (candidate.get<IsDish>().type != type) {
      continue;
    }
    return true;
  }
  return false;
}

} // namespace merge
