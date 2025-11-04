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
    if (queue.empty()) {
      return;
    }

    // Build index order to avoid moving/assigning TriggerEvent
    std::vector<size_t> order;
    order.reserve(queue.events.size());
    for (size_t i = 0; i < queue.events.size(); ++i) {
      order.push_back(i);
    }

    // Deterministic ordering: (slotIndex asc, Player then Opponent, Zing desc,
    // sourceEntityId asc)
    // TODO: Update to use highest total Zing team first
    std::sort(order.begin(), order.end(), [&](size_t ia, size_t ib) {
      const TriggerEvent &a = queue.events[ia];
      const TriggerEvent &b = queue.events[ib];
      if (a.slotIndex != b.slotIndex)
        return a.slotIndex < b.slotIndex;
      if (a.teamSide != b.teamSide)
        return a.teamSide == DishBattleState::TeamSide::Player;
      int za = 0;
      if (auto ea = EQ({.ignore_temp_warning = true})
                        .whereID(a.sourceEntityId)
                        .gen_first()) {
        if (ea->has<CombatStats>()) {
          za = ea->get<CombatStats>().currentZing;
        }
      }
      int zb = 0;
      if (auto eb = EQ({.ignore_temp_warning = true})
                        .whereID(b.sourceEntityId)
                        .gen_first()) {
        if (eb->has<CombatStats>()) {
          zb = eb->get<CombatStats>().currentZing;
        }
      }
      if (za != zb)
        return za > zb; // higher zing first
      return a.sourceEntityId < b.sourceEntityId;
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
};
