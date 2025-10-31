#pragma once

#include "../components/animation_event.h"
#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../shop.h"
#include <afterhours/ah.h>

struct OnServeState : afterhours::BaseComponent {
  int currentSlot = 0;
  bool allFired = false;
};

struct SimplifiedOnServeSystem : afterhours::System<CombatQueue> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &, CombatQueue &cq, float) override {
    if (cq.complete) {
      clear_onserve_state();
      return;
    }

    auto onserveState = get_or_create_onserve_state();
    if (!onserveState) {
      return;
    }

    auto &state = onserveState->get<OnServeState>();

    if (state.allFired) {
      return;
    }

    if (!slide_in_complete()) {
      return;
    }

    if (hasActiveAnimation()) {
      return;
    }

    afterhours::RefEntities dishes =
        afterhours::EntityQuery()
            .whereHasComponent<IsDish>()
            .whereHasComponent<DishBattleState>()
            .whereLambda([](const afterhours::Entity &e) {
              const DishBattleState &dbs = e.get<DishBattleState>();
              return dbs.phase == DishBattleState::Phase::InQueue &&
                     !dbs.onserve_fired;
            })
            .orderByLambda(
                [](const afterhours::Entity &a, const afterhours::Entity &b) {
                  const DishBattleState &dbs_a = a.get<DishBattleState>();
                  const DishBattleState &dbs_b = b.get<DishBattleState>();
                  return dbs_a.queue_index < dbs_b.queue_index;
                })
            .gen();

    if (dishes.empty()) {
      state.allFired = true;
      log_info("COMBAT: All OnServe triggers fired");
      return;
    }

    afterhours::RefEntities currentSlotDishes =
        afterhours::EntityQuery()
            .whereHasComponent<IsDish>()
            .whereHasComponent<DishBattleState>()
            .whereLambda([&state](const afterhours::Entity &e) {
              const DishBattleState &dbs = e.get<DishBattleState>();
              return dbs.phase == DishBattleState::Phase::InQueue &&
                     !dbs.onserve_fired && dbs.queue_index == state.currentSlot;
            })
            .gen();

    bool firedThisSlot = false;
    for (afterhours::Entity &dish : currentSlotDishes) {
      DishBattleState &dbs = dish.get<DishBattleState>();
      fire_onserve_trigger(dish, dbs);
      dbs.onserve_fired = true;
      firedThisSlot = true;
      log_info("COMBAT: Fired OnServe for slot {} (left-to-right)",
               state.currentSlot);
    }

    if (firedThisSlot) {
      auto maxSlotOpt =
          EQ().whereHasComponent<IsDish>()
              .whereHasComponent<DishBattleState>()
              .whereLambda([](const afterhours::Entity &e) {
                const DishBattleState &dbs = e.get<DishBattleState>();
                return dbs.phase == DishBattleState::Phase::InQueue &&
                       !dbs.onserve_fired;
              })
              .gen_max_value<int>([](const afterhours::Entity &e) {
                return e.get<DishBattleState>().queue_index;
              });

      if (maxSlotOpt && state.currentSlot >= *maxSlotOpt) {
        state.allFired = true;
        log_info("COMBAT: All OnServe triggers fired");
      } else {
        state.currentSlot++;
      }
    }
  }

private:
  void fire_onserve_trigger(afterhours::Entity &e, DishBattleState &dbs) {
    if (auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
        tq.get().has<TriggerQueue>()) {
      auto &queue = tq.get().get<TriggerQueue>();
      queue.add_event(TriggerHook::OnServe, e.id, dbs.queue_index,
                      dbs.team_side);

      log_info("COMBAT: Fired OnServe trigger for dish {} (slot {}, team {})",
               e.id, dbs.queue_index,
               dbs.team_side == DishBattleState::TeamSide::Player ? "Player"
                                                                  : "Opponent");
    }
  }

  afterhours::Entity *get_or_create_onserve_state() {
    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<OnServeState>().gen()) {
      return &e;
    }

    auto &newEntity = afterhours::EntityHelper::createEntity();
    newEntity.addComponent<OnServeState>();
    return &newEntity;
  }

  void clear_onserve_state() {
    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<OnServeState>().gen()) {
      e.removeComponent<OnServeState>();
    }
  }

  bool slide_in_complete() {
    return !afterhours::EntityQuery()
                .whereHasComponent<AnimationEvent>()
                .whereHasComponent<AnimationTimer>()
                .whereLambda([](const afterhours::Entity &e) {
                  const AnimationEvent &ev = e.get<AnimationEvent>();
                  return ev.type == AnimationEventType::SlideIn;
                })
                .has_values();
  }
};
