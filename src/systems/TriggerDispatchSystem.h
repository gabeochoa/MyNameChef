#pragma once

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
    return gsm.active_screen == GameStateManager::Screen::Battle &&
           !hasTriggerAnimationRunning();
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

    log_info("TRIGGER_QUEUE dispatch order ({} events):", order.size());
    for (size_t idx : order) {
      const auto &ev = queue.events[idx];
      log_info("  - hook={} slot={} side={} source={} payloadInt={}",
               magic_enum::enum_name(ev.hook), ev.slotIndex,
               ev.teamSide == DishBattleState::TeamSide::Player ? "Player"
                                                                : "Opponent",
               ev.sourceEntityId, ev.payloadInt);
      handle_event(ev);
    }

    log_info("TRIGGER_QUEUE processed and cleared ({} events)", order.size());
    queue.clear();
  }

private:
  void handle_event(const TriggerEvent &ev) {
    switch (ev.hook) {
    case TriggerHook::OnServe:
      on_serve(ev);
      break;
    case TriggerHook::OnBiteTaken:
      on_bite_taken(ev);
      break;
    case TriggerHook::OnDishFinished:
      on_dish_finished(ev);
      break;
    case TriggerHook::OnCourseStart:
      on_course_start(ev);
      break;
    case TriggerHook::OnCourseComplete:
      on_course_complete(ev);
      break;
    }
  }

  void on_serve(const TriggerEvent &ev) {
    log_info("TRIGGER OnServe slot={} side={}", ev.slotIndex,
             ev.teamSide == DishBattleState::TeamSide::Player ? "Player"
                                                              : "Opponent");

    // Dispatch to dish-defined onServe, if any
    if (auto src = EQ().whereID(ev.sourceEntityId).gen_first()) {
      if (src->has<IsDish>()) {
        const auto &info = get_dish_info(src->get<IsDish>().type);
        if (info.onServe) {
          log_info("TRIGGER OnServe handler: dish={} (id={})", info.name,
                   ev.sourceEntityId);
          info.onServe(ev.sourceEntityId);
          log_info("TRIGGER OnServe handler completed for id={}",
                   ev.sourceEntityId);
        }
      }
    }
  }

  void on_bite_taken(const TriggerEvent &ev) {
    log_info("TRIGGER OnBiteTaken slot={} side={} dmg={}", ev.slotIndex,
             ev.teamSide == DishBattleState::TeamSide::Player ? "Player"
                                                              : "Opponent",
             ev.payloadInt);
  }

  void on_dish_finished(const TriggerEvent &ev) {
    log_info("TRIGGER OnDishFinished slot={} side={}", ev.slotIndex,
             ev.teamSide == DishBattleState::TeamSide::Player ? "Player"
                                                              : "Opponent");
  }

  void on_course_start(const TriggerEvent &ev) {
    log_info("TRIGGER OnCourseStart slot={}", ev.slotIndex);
  }

  void on_course_complete(const TriggerEvent &ev) {
    log_info("TRIGGER OnCourseComplete slot={}", ev.slotIndex);
  }
};
