#pragma once

#include "../components/animation_event.h"
#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../render_backend.h"
#include "../shop.h"
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

    static int onserve_check_count = 0;
    onserve_check_count++;
    
    bool slide_complete = slide_in_complete();
    bool has_blocking = hasActiveAnimation();
    
    if (!slide_complete) {
      if (onserve_check_count % 60 == 0 || onserve_check_count <= 30) {
        log_info("ONSERVE: SlideIn not complete, skipping - call={}", onserve_check_count);
      }
      return;
    }

    // Check for blocking animations - in headless mode hasActiveAnimation() returns false
    if (has_blocking) {
      if (onserve_check_count % 60 == 0 || onserve_check_count <= 30) {
        log_info("ONSERVE: Blocking animations active, skipping - call={}", onserve_check_count);
      }
      return;
    }

    // Fire OnServe for all dishes at InQueue that haven't fired yet
    afterhours::RefEntities unfiredDishes =
        afterhours::EntityQuery({.ignore_temp_warning = true})
            .whereHasComponent<IsDish>()
            .whereHasComponent<DishBattleState>()
            .whereLambda([](const afterhours::Entity &e) {
              const DishBattleState &dbs = e.get<DishBattleState>();
              return dbs.phase == DishBattleState::Phase::InQueue &&
                     !dbs.onserve_fired;
            })
            .gen();

    if (!unfiredDishes.empty()) {
      if (onserve_check_count <= 30) {
        log_info("ONSERVE: Firing OnServe for {} dishes - call={}", unfiredDishes.size(), onserve_check_count);
      }
      for (afterhours::Entity &dish : unfiredDishes) {
        DishBattleState &dbs = dish.get<DishBattleState>();
        fire_onserve_trigger(dish, dbs);
        dbs.onserve_fired = true;
        log_info("COMBAT: Fired OnServe for dish {} (slot {}, team {})",
                 dish.id, dbs.queue_index,
                 (dbs.team_side == DishBattleState::TeamSide::Player)
                     ? "Player"
                     : "Opponent");
      }
    } else if (onserve_check_count <= 30) {
      log_info("ONSERVE: No unfired dishes found - call={}", onserve_check_count);
    }

    // Check if all dishes have fired OnServe
    bool hasUnfiredDishes =
        EQ({.ignore_temp_warning = true})
            .whereHasComponent<IsDish>()
            .whereHasComponent<DishBattleState>()
            .whereLambda([](const afterhours::Entity &e) {
              const DishBattleState &dbs = e.get<DishBattleState>();
              return dbs.phase == DishBattleState::Phase::InQueue &&
                     !dbs.onserve_fired;
            })
            .has_values();

    if (!hasUnfiredDishes) {
      state.allFired = true;
      log_info("COMBAT: All OnServe triggers fired");
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
         afterhours::EntityQuery({.ignore_temp_warning = true})
             .whereHasComponent<OnServeState>()
             .gen()) {
      return &e;
    }

    auto &newEntity = afterhours::EntityHelper::createEntity();
    newEntity.addComponent<OnServeState>();
    return &newEntity;
  }

  void clear_onserve_state() {
    for (afterhours::Entity &e :
         afterhours::EntityQuery({.ignore_temp_warning = true})
             .whereHasComponent<OnServeState>()
             .gen()) {
      e.removeComponent<OnServeState>();
    }
  }

  bool slide_in_complete() {
    // Check if any slide-in animations are still active
    // In headless mode, animations complete instantly so this will always return true
    return !afterhours::EntityQuery({.ignore_temp_warning = true})
                .whereHasComponent<AnimationEvent>()
                .whereHasComponent<AnimationTimer>()
                .whereLambda([](const afterhours::Entity &e) {
                  const AnimationEvent &ev = e.get<AnimationEvent>();
                  return ev.type == AnimationEventType::SlideIn;
                })
                .has_values();
  }
};
