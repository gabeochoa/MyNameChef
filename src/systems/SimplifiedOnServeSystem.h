#pragma once

#include "../components/animation_event.h"
#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

struct OnServeState : afterhours::BaseComponent {
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

    log_info("COMBAT: SlideIn complete, firing all OnServe triggers");

    for (afterhours::Entity &dish : afterhours::EntityQuery()
                                        .whereHasComponent<IsDish>()
                                        .whereHasComponent<DishBattleState>()
                                        .gen()) {
      auto &dbs = dish.get<DishBattleState>();
      if (dbs.phase == DishBattleState::Phase::InQueue && !dbs.onserve_fired) {
        fire_onserve_trigger(dish, dbs);
        dbs.onserve_fired = true;
      }
    }

    state.allFired = true;
    log_info("COMBAT: All OnServe triggers fired");
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
    for (afterhours::Entity &animEntity :
         afterhours::EntityQuery()
             .whereHasComponent<AnimationEvent>()
             .whereHasComponent<AnimationTimer>()
             .gen()) {
      auto &ev = animEntity.get<AnimationEvent>();
      if (ev.type == AnimationEventType::SlideIn) {
        return false;
      }
    }

    return true;
  }
};
