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
    std::sort(order.begin(), order.end(), [&](size_t ia, size_t ib) {
      const TriggerEvent &a = queue.events[ia];
      const TriggerEvent &b = queue.events[ib];
      if (a.slotIndex != b.slotIndex)
        return a.slotIndex < b.slotIndex;
      if (a.teamSide != b.teamSide)
        return a.teamSide == DishBattleState::TeamSide::Player;
      int za = 0;
      if (auto ea = EQ().whereID(a.sourceEntityId).gen_first()) {
        if (ea->has<CombatStats>()) {
          za = ea->get<CombatStats>().currentZing;
        }
      }
      int zb = 0;
      if (auto eb = EQ().whereID(b.sourceEntityId).gen_first()) {
        if (eb->has<CombatStats>()) {
          zb = eb->get<CombatStats>().currentZing;
        }
      }
      if (za != zb)
        return za > zb; // higher zing first
      return a.sourceEntityId < b.sourceEntityId;
    });

    // quiet dispatch order
    for (size_t idx : order) {
      const auto &ev = queue.events[idx];
      // quiet per-event details
      handle_event(ev);
      // After each dispatch, enqueue a small non-blocking animation event if
      // desired
    }
    // quiet queue processed
    queue.clear();
  }

private:
  void handle_event(const TriggerEvent &ev) {
    switch (ev.hook) {
    case TriggerHook::OnStartBattle:
      on_start_battle(ev);
      break;
    case TriggerHook::OnServe:
      on_serve(ev);
      break;
    case TriggerHook::OnCourseStart:
      on_course_start(ev);
      break;
    case TriggerHook::OnBiteTaken:
      on_bite_taken(ev);
      break;
    case TriggerHook::OnDishFinished:
      on_dish_finished(ev);
      break;
    case TriggerHook::OnCourseComplete:
      on_course_complete(ev);
      break;
    }
  }

  void on_start_battle(const TriggerEvent &) {}

  void on_serve(const TriggerEvent &ev) {
    // quiet on_serve

    // Dispatch to dish-defined onServe, if any
    if (auto src = EQ().whereID(ev.sourceEntityId).gen_first()) {
      if (src->has<IsDish>()) {
        const auto &info = get_dish_info(src->get<IsDish>().type);
        if (info.onServe) {
          // quiet handler name
          info.onServe(ev.sourceEntityId);
          // quiet completion
        }
      }
    }
  }

  void on_bite_taken(const TriggerEvent &) {}

  void on_dish_finished(const TriggerEvent &) {}

  void on_course_start(const TriggerEvent &) {}

  void on_course_complete(const TriggerEvent &) {}
};
