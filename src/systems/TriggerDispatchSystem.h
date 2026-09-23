#pragma once

#include "../components/animation_event.h"
#include "../components/combat_stats.h"
#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../shop.h"
#include <afterhours/ah.h>
#include <algorithm>
#include <magic_enum/magic_enum.hpp>
#include <map>

struct TriggerDispatchSystem : afterhours::System<TriggerQueue> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Battle) {
      return false;
    }
    if (isReplayPaused()) {
      return false;
    }

    // Allow this system to run even when animations are active
    // This is needed to process OnServe triggers even when FreshnessChain
    // animations are playing
    return true;
  }

  void for_each_with(afterhours::Entity &, TriggerQueue &queue,
                     float) override {
    sort_events(queue);
  }

  static void sort_events(TriggerQueue &queue) {
    if (queue.empty()) {
      return;
    }

    // Pre-calculate and cache team totals (sum baseZing of all dishes on each
    // team)
    std::map<DishBattleState::TeamSide, int> team_total_zing;
    team_total_zing[DishBattleState::TeamSide::Player] = 0;
    team_total_zing[DishBattleState::TeamSide::Opponent] = 0;

    for (afterhours::Entity &e : EQ({.ignore_temp_warning = true})
                                     .whereHasComponent<IsDish>()
                                     .whereHasComponent<DishBattleState>()
                                     .gen()) {
      const auto &dbs = e.get<DishBattleState>();
      if (!e.has<CombatStats>()) {
        log_warn("TRIGGER_ORDER: Dish {} on team {} missing CombatStats", e.id,
                  dbs.team_side == DishBattleState::TeamSide::Player
                      ? "Player"
                      : "Opponent");
        continue;
      }
      const auto &cs = e.get<CombatStats>();
      team_total_zing[dbs.team_side] += cs.baseZing;
    }

    // Build index order to avoid moving/assigning TriggerEvent
    std::vector<size_t> order;
    order.reserve(queue.events.size());
    for (size_t i = 0; i < queue.events.size(); ++i) {
      order.push_back(i);
    }

    // Issues 54/55: one consistent tuple for every pair + stable (emission order preserved for equal keys)
    TriggerEventComparator comparator(team_total_zing);
    std::stable_sort(order.begin(), order.end(), [&](size_t ia, size_t ib) {
      return comparator(queue.events[ia], queue.events[ib]);
    });

    // Reorder events vector using the sorted indices
    std::vector<TriggerEvent> reordered_events;
    reordered_events.reserve(queue.events.size());
    for (size_t idx : order) {
      reordered_events.push_back(std::move(queue.events[idx]));
    }
    queue.events = std::move(reordered_events);

    // Events are now ordered - EffectResolutionSystem will process them
    // Don't clear queue here - EffectResolutionSystem will clear it after
    // processing
  }

private:
  struct TriggerEventComparator {
    const std::map<DishBattleState::TeamSide, int> &team_total_zing;

    bool operator()(const TriggerEvent &a, const TriggerEvent &b) const {
      // Strict weak ordering: same tuple keys for same- and cross-team pairs.
      if (a.slotIndex != b.slotIndex) return a.slotIndex < b.slotIndex;
      int ta = team_total_zing.count(a.teamSide) ? team_total_zing.at(a.teamSide) : 0;
      int tb = team_total_zing.count(b.teamSide) ? team_total_zing.at(b.teamSide) : 0;
      if (ta != tb) return ta > tb;
      int za = get_entity_zing(a.sourceEntityId), zb = get_entity_zing(b.sourceEntityId);
      if (za != zb) return za > zb;
      if (a.sourceEntityId != b.sourceEntityId) return a.sourceEntityId < b.sourceEntityId;
      return static_cast<int>(a.hook) < static_cast<int>(b.hook);
    }

  private:
    int get_entity_zing(int sourceEntityId) const {
      if (sourceEntityId <= 0)
        return 0;
      if (auto ea = EQ({.ignore_temp_warning = true})
                        .whereID(sourceEntityId)
                        .gen_first()) {
        if (ea->has<CombatStats>()) {
          return ea->get<CombatStats>().baseZing;
        } else {
          log_warn("TRIGGER_ORDER: Source entity {} missing CombatStats",
                    sourceEntityId);
        }
      }
      return 0;
    }
  };
};
