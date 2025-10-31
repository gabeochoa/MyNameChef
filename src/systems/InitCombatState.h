#pragma once

#include "../components/animation_event.h"
#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../game_state_manager.h"
#include "../shop.h"
#include <afterhours/ah.h>

struct InitCombatState : afterhours::System<CombatQueue> {
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    GameStateManager &gsm = GameStateManager::get();
    bool entering_battle =
        last_screen != GameStateManager::Screen::Battle &&
        gsm.active_screen == GameStateManager::Screen::Battle;
    last_screen = gsm.active_screen;
    return entering_battle;
  }

  void for_each_with(afterhours::Entity &, CombatQueue &cq, float) override {
    // Reset combat queue
    cq.reset();

    log_info("COMBAT: Reset combat queue - current_index: {}, total_courses: "
             "{}, complete: {}",
             cq.current_index, cq.total_courses, cq.complete);

    // Set all dishes to InQueue phase
    int dish_count = 0;
    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
      DishBattleState &dbs = e.get<DishBattleState>();
      dbs.phase = DishBattleState::Phase::InQueue;
      dbs.enter_progress = 0.0f;
      dbs.bite_timer = 0.0f;
      dbs.onserve_fired = false;
      dish_count++;

      log_info(
          "COMBAT: Reset dish {} - team: {}, slot: {}, phase: InQueue", e.id,
          (dbs.team_side == DishBattleState::TeamSide::Player) ? "Player"
                                                               : "Opponent",
          dbs.queue_index);
    }

    log_info(
        "COMBAT: Initialized combat state - {} courses, {} dishes in queue",
        cq.total_courses, dish_count);

    log_info("COMBAT: Creating SlideIn animation at battle start");
    make_animation_event(AnimationEventType::SlideIn, true);
  }
};
