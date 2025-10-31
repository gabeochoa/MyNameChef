#pragma once

#include "../components/animation_event.h"
#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../game_state_manager.h"
#include "../shop.h"
#include <afterhours/ah.h>

struct StartCourseSystem : afterhours::System<CombatQueue> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    bool should_run = gsm.active_screen == GameStateManager::Screen::Battle &&
                      !hasActiveAnimation();
    if (!should_run && gsm.active_screen == GameStateManager::Screen::Battle) {
      log_info("COMBAT: StartCourseSystem paused - animation active");
    }
    return should_run;
  }

  void for_each_with(afterhours::Entity &, CombatQueue &cq, float) override {
    if (cq.complete) {
      // Quiet skip once combat is complete
      return;
    }

    // (quiet)

    if (!prerequisites_complete()) {
      return;
    }

    // Find player and opponent dishes for current slot
    auto player_dish =
        find_dish_for_slot(cq.current_index, DishBattleState::TeamSide::Player);
    auto opponent_dish = find_dish_for_slot(
        cq.current_index, DishBattleState::TeamSide::Opponent);

    // (quiet)

    if (!player_dish || !opponent_dish) {
      // No more dishes, mark combat complete and transition to results
      cq.complete = true;
      log_info("COMBAT: No more dishes for slot {}, transitioning to results",
               cq.current_index);
      GameStateManager::get().to_results();
      return;
    }

    auto &player_dbs = player_dish.value()->get<DishBattleState>();
    auto &opponent_dbs = opponent_dish.value()->get<DishBattleState>();

    // Check if both dishes for current course are finished
    if (player_dbs.phase == DishBattleState::Phase::InCombat &&
        opponent_dbs.phase == DishBattleState::Phase::InCombat) {
      // Both dishes are done, wait for combat to finish
      return;
    }

    const float enter_start_delay = 0.25f;

    if (player_dbs.phase == DishBattleState::Phase::InQueue &&
        opponent_dbs.phase == DishBattleState::Phase::InQueue) {
      player_dbs.phase = DishBattleState::Phase::Entering;
      player_dbs.enter_progress = -enter_start_delay;
      log_info("COMBAT: Starting course {} - player dish entering first",
               cq.current_index);
    } else if (player_dbs.phase == DishBattleState::Phase::InCombat &&
               opponent_dbs.phase == DishBattleState::Phase::InQueue) {
      opponent_dbs.phase = DishBattleState::Phase::Entering;
      opponent_dbs.enter_progress = -enter_start_delay;
      log_info("COMBAT: Starting course {} - opponent dish entering second",
               cq.current_index);
    }
  }

private:
  std::optional<afterhours::Entity *>
  find_dish_for_slot(int slot_index, DishBattleState::TeamSide side) {
    for (auto &ref :
         afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
      auto &e = ref.get();
      auto &dbs = e.get<DishBattleState>();
      if (dbs.team_side == side && dbs.queue_index == slot_index &&
          (dbs.phase == DishBattleState::Phase::InQueue ||
           dbs.phase == DishBattleState::Phase::Entering ||
           dbs.phase == DishBattleState::Phase::InCombat)) {
        return &e;
      }
    }
    return std::nullopt;
  }

  bool prerequisites_complete() {
    bool slideInComplete = true;
    for (afterhours::Entity &animEntity :
         afterhours::EntityQuery()
             .whereHasComponent<AnimationEvent>()
             .whereHasComponent<AnimationTimer>()
             .gen()) {
      auto &ev = animEntity.get<AnimationEvent>();
      if (ev.type == AnimationEventType::SlideIn) {
        slideInComplete = false;
        break;
      }
    }

    if (!slideInComplete) {
      return false;
    }

    for (afterhours::Entity &dish : afterhours::EntityQuery()
                                        .whereHasComponent<IsDish>()
                                        .whereHasComponent<DishBattleState>()
                                        .gen()) {
      auto &dbs = dish.get<DishBattleState>();
      if (dbs.phase == DishBattleState::Phase::InQueue && !dbs.onserve_fired) {
        return false;
      }
    }

    if (hasActiveAnimation()) {
      return false;
    }

    return true;
  }
};
