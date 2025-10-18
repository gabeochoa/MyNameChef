#pragma once

#include "../components/animation_event.h"
#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include "../shop.h"
#include <afterhours/ah.h>

// Component to track OnServe trigger state per course
struct OnServeState : afterhours::BaseComponent {
  int courseIndex = -1;
  bool playerFired = false;
  bool opponentFired = false;
  bool slideInCreated = false;
};

// Simplified system to handle OnServe trigger batching
struct SimplifiedOnServeSystem : afterhours::System<CombatQueue> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &, CombatQueue &cq, float) override {
    if (cq.complete) {
      // Clear OnServe state when combat is complete
      clear_onserve_state();
      return;
    }

    // Get or create OnServe state for current course
    auto onserveState = get_or_create_onserve_state(cq.current_index);
    if (!onserveState) {
      return;
    }

    auto &state = onserveState->get<OnServeState>();

    // Check if both dishes for current course have finished entering
    auto player_dish =
        find_dish_for_slot(cq.current_index, DishBattleState::TeamSide::Player);
    auto opponent_dish = find_dish_for_slot(
        cq.current_index, DishBattleState::TeamSide::Opponent);

    if (!player_dish || !opponent_dish) {
      return;
    }

    auto &player_dbs = player_dish->get<DishBattleState>();
    auto &opponent_dbs = opponent_dish->get<DishBattleState>();

    // Fire OnServe triggers if both dishes are ready and haven't fired yet
    if (player_dbs.phase == DishBattleState::Phase::InCombat &&
        opponent_dbs.phase == DishBattleState::Phase::InCombat &&
        !state.playerFired && !state.opponentFired) {

      log_info("COMBAT: Firing OnServe triggers for course {}",
               cq.current_index);

      // Fire OnServe triggers for both dishes
      fire_onserve_trigger(*player_dish, player_dbs);
      fire_onserve_trigger(*opponent_dish, opponent_dbs);

      // Mark both dishes as having fired OnServe
      player_dbs.onserve_fired = true;
      opponent_dbs.onserve_fired = true;
      state.playerFired = true;
      state.opponentFired = true;
    }

    // Create slide-in animation when all courses are done and no animations are
    // active
    if (!state.slideInCreated && all_courses_complete() &&
        no_animations_active()) {
      log_info("COMBAT: Creating slide-in animation after all OnServe triggers "
               "processed");
      create_slidein_animation();
      state.slideInCreated = true;
    }
  }

private:
  afterhours::Entity *find_dish_for_slot(int slot_index,
                                         DishBattleState::TeamSide team_side) {
    for (afterhours::Entity &e : afterhours::EntityQuery()
                                     .whereHasComponent<IsDish>()
                                     .whereHasComponent<DishBattleState>()
                                     .gen()) {
      auto &dbs = e.get<DishBattleState>();
      if (dbs.team_side == team_side && dbs.queue_index == slot_index) {
        return &e;
      }
    }
    return nullptr;
  }

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

  afterhours::Entity *get_or_create_onserve_state(int courseIndex) {
    // Look for existing state
    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<OnServeState>().gen()) {
      auto &state = e.get<OnServeState>();
      if (state.courseIndex == courseIndex) {
        return &e;
      }
    }

    // Create new state if not found
    auto &newEntity = afterhours::EntityHelper::createEntity();
    newEntity.addComponent<OnServeState>();
    auto &state = newEntity.get<OnServeState>();
    state.courseIndex = courseIndex;
    return &newEntity;
  }

  void clear_onserve_state() {
    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<OnServeState>().gen()) {
      e.removeComponent<OnServeState>();
    }
  }

  bool all_courses_complete() {
    // Check if all dishes have fired OnServe
    for (afterhours::Entity &e : afterhours::EntityQuery()
                                     .whereHasComponent<IsDish>()
                                     .whereHasComponent<DishBattleState>()
                                     .gen()) {
      auto &dbs = e.get<DishBattleState>();
      if (dbs.phase == DishBattleState::Phase::InCombat && !dbs.onserve_fired) {
        return false;
      }
    }
    return true;
  }

  bool no_animations_active() {
    // Check if trigger queue is empty
    if (auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
        tq.get().has<TriggerQueue>()) {
      auto &queue = tq.get().get<TriggerQueue>();
      if (!queue.empty()) {
        return false;
      }
    }

    // Check if any animation events are active
    for (afterhours::Entity &animEntity :
         afterhours::EntityQuery().whereHasComponent<AnimationEvent>().gen()) {
      (void)animEntity; // Suppress unused variable warning
      return false;     // Found an active animation
    }

    return true;
  }

  void create_slidein_animation() {
    // Create a SlideIn animation event
    make_animation_event(AnimationEventType::SlideIn, true);
  }
};
