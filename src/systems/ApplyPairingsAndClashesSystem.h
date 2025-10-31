#pragma once

#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../components/pre_battle_modifiers.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../query.h"
#include <afterhours/ah.h>
#include <map>
#include <unordered_set>
#include <vector>

struct ApplyPairingsAndClashesSystem
    : afterhours::System<PreBattleModifiers> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &e, PreBattleModifiers &mod, float) override {
    // Only apply once when entering battle (when dish is InQueue)
    if (!e.has<DishBattleState>()) {
      return;
    }

    auto &dbs = e.get<DishBattleState>();
    if (dbs.phase != DishBattleState::Phase::InQueue) {
      return;
    }

    // Skip if already computed (tracked via a flag, or just compute once per battle)
    // For now, we'll recompute each frame but only apply when InQueue
    // In production, add a "computed" flag to avoid redundant work

    // Get all dishes for this team
    auto team_dishes = get_team_dishes(dbs.team_side);
    
    // Compute pairings and clashes for this team
    int pairing_bonus = compute_pairing_bonus(team_dishes);
    int clash_penalty = compute_clash_penalty(team_dishes);

    // Apply modifiers: pairings add Body, clashes reduce Zing
    mod.bodyDelta = pairing_bonus;
    mod.zingDelta = -clash_penalty;

    if (mod.bodyDelta != 0 || mod.zingDelta != 0) {
      log_info("PAIRINGS_CLASHES: Dish {} - team={} slot={} bodyDelta={} zingDelta={}",
               e.id,
               dbs.team_side == DishBattleState::TeamSide::Player ? "Player"
                                                                  : "Opponent",
               dbs.queue_index, mod.bodyDelta, mod.zingDelta);
    }
  }

private:
  // Get all dishes for a team
  std::vector<afterhours::RefEntity> get_team_dishes(DishBattleState::TeamSide side) {
    std::vector<afterhours::RefEntity> dishes;
    for (afterhours::Entity &e :
         EQ().whereHasComponent<IsDish>()
             .whereHasComponent<DishBattleState>()
             .gen()) {
      auto &dbs = e.get<DishBattleState>();
      if (dbs.team_side == side && dbs.phase == DishBattleState::Phase::InQueue) {
        dishes.push_back(e);
      }
    }
    return dishes;
  }

  // Simple pairing rule: complementary flavors give bonus
  // Pairing = dishes with different dominant flavor categories
  int compute_pairing_bonus(const std::vector<afterhours::RefEntity> &dishes) {
    if (dishes.size() < 2) {
      return 0; // Need at least 2 dishes to pair
    }

    // Count unique flavor categories
    std::unordered_set<int> flavor_categories;
    for (const auto &ref : dishes) {
      auto &e = ref.get();
      if (!e.has<IsDish>()) {
        continue;
      }

      auto &dish = e.get<IsDish>();
      const auto &info = get_dish_info(dish.type);
      const FlavorStats &flavor = info.flavor;

      // Classify dish by dominant flavor category
      int dominant = get_dominant_flavor_category(flavor);
      flavor_categories.insert(dominant);
    }

    // Bonus: +1 Body per unique flavor category (max +3 for diversity)
    int unique_categories = static_cast<int>(flavor_categories.size());
    return std::min(unique_categories - 1, 3); // -1 because need 2+ to get bonus
  }

  // Simple clash rule: too many similar dishes cause penalty
  // Clash = multiple dishes of the same type or same dominant flavor
  int compute_clash_penalty(const std::vector<afterhours::RefEntity> &dishes) {
    if (dishes.size() < 2) {
      return 0; // Need at least 2 dishes to clash
    }

    // Count dish types
    std::map<DishType, int> type_counts;
    for (const auto &ref : dishes) {
      auto &e = ref.get();
      if (!e.has<IsDish>()) {
        continue;
      }

      auto &dish = e.get<IsDish>();
      type_counts[dish.type]++;
    }

    // Penalty: -1 Zing for each duplicate dish type
    int clashes = 0;
    for (const auto &[type, count] : type_counts) {
      if (count > 1) {
        clashes += (count - 1); // Each duplicate beyond first is a clash
      }
    }

    return std::min(clashes, 3); // Cap at -3 Zing max penalty
  }

  // Classify dish by dominant flavor category
  // Returns category ID: 0=sweet, 1=spicy, 2=umami, 3=rich, 4=fresh, 5=acidic
  int get_dominant_flavor_category(const FlavorStats &flavor) {
    int max_val = flavor.sweetness;
    int category = 0; // sweet

    if (flavor.spice > max_val) {
      max_val = flavor.spice;
      category = 1; // spicy
    }
    if (flavor.umami > max_val) {
      max_val = flavor.umami;
      category = 2; // umami
    }
    if (flavor.richness > max_val) {
      max_val = flavor.richness;
      category = 3; // rich
    }
    if (flavor.freshness > max_val) {
      max_val = flavor.freshness;
      category = 4; // fresh
    }
    if (flavor.acidity > max_val) {
      max_val = flavor.acidity;
      category = 5; // acidic
    }

    return category;
  }
};

