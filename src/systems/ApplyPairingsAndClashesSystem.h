#pragma once

#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../components/pairing_clash_modifiers.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../query.h"
#include <afterhours/ah.h>
#include <map>
#include <unordered_set>
#include <vector>

struct ApplyPairingsAndClashesSystem
    : afterhours::System<IsDish, DishBattleState> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &e, IsDish &, DishBattleState &dbs, float) override {
    // Only apply once when entering battle (when dish is InQueue)
    if (dbs.phase != DishBattleState::Phase::InQueue) {
      return;
    }
    
    // Get or create PairingClashModifiers component
    auto &mod = e.addComponentIfMissing<PairingClashModifiers>();
    
    // Track old values to detect overwrites
    int oldBodyDelta = mod.bodyDelta;
    int oldZingDelta = mod.zingDelta;

    // Get all dishes for this team
    auto team_dishes = get_team_dishes(dbs.team_side);
    
    // Compute pairings and clashes for this team
    int pairing_bonus = compute_pairing_bonus(team_dishes);
    int clash_penalty = compute_clash_penalty(team_dishes);

    // Store pairing/clash modifiers - these should be calculated once per battle
    // We need to avoid overwriting combat modifiers that were added later
    // Strategy: Calculate what pairing/clash modifiers SHOULD be, and only
    // apply them if they haven't been set yet OR if we need to update them
    // without overwriting combat modifiers.
    // 
    // For now, we'll only set if modifiers are at default (0), meaning
    // this is the first calculation. Combat modifiers added later will
    // be preserved because this system only runs when InQueue, and combat
    // modifiers are added when dishes are InCombat or later.
    // 
    // However, if combat modifiers were added while dish is still InQueue,
    // we need to preserve them. So we check: if bodyDelta/zingDelta are
    // positive (combat bonuses), we add pairing/clash on top.
    // If they're 0, we set them (initial pairing/clash calculation).
    // If they're negative (clash penalty already applied), we might be
    // recalculating, so we preserve the existing value and add the new one.
    
    // Only set pairing/clash modifiers if not already set
    if (mod.bodyDelta == 0 && mod.zingDelta == 0) {
      // First time - set pairing/clash modifiers
      mod.bodyDelta = pairing_bonus;
      mod.zingDelta = -clash_penalty;
      // quiet
    } else {
      // Modifiers already exist - preserve them
      if (oldBodyDelta != mod.bodyDelta || oldZingDelta != mod.zingDelta) {
        log_error("PAIRINGS_CLASHES: Dish {} - MODIFIERS CHANGED! bodyDelta: {} -> {}, zingDelta: {} -> {}",
                 e.id, oldBodyDelta, mod.bodyDelta, oldZingDelta, mod.zingDelta);
      }
      // quiet
    }

    // quiet
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

