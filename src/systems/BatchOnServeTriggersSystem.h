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
#include <set>

// System to ensure OnServe triggers are batched per course and processed
// together
struct BatchOnServeTriggersSystem : afterhours::System<CombatQueue> {
  static std::set<int> processed_courses;
  static std::set<int> onserve_fired_courses;
  static int last_course_index;
  static bool global_slidein_created;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Battle) {
      return false;
    }

    // Allow this system to run even when animations are active if we're waiting
    // for OnServe animations to complete This is needed to check if OnServe
    // animations have finished and create the SlideIn animation
    return true;
  }

  void for_each_with(afterhours::Entity &, CombatQueue &cq, float) override {
    if (cq.complete) {
      // Clear processed courses when combat is complete
      processed_courses.clear();
      onserve_fired_courses.clear();
      last_course_index = -1;
      global_slidein_created = false;
      return;
    }

    // Clear processed courses when starting a new course
    if (cq.current_index != last_course_index) {
      if (cq.current_index == 0) {
        // Starting combat - clear everything
        processed_courses.clear();
        onserve_fired_courses.clear();
        global_slidein_created = false;
      }
      last_course_index = cq.current_index;
    }

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

    // Check if both dishes have finished entering but haven't fired OnServe yet
    if (player_dbs.phase == DishBattleState::Phase::InCombat &&
        !player_dbs.onserve_fired &&
        opponent_dbs.phase == DishBattleState::Phase::InCombat &&
        !opponent_dbs.onserve_fired &&
        !has_fired_onserve_for_course(cq.current_index)) {

      log_info("COMBAT: Batching OnServe triggers for course {} - Player dish "
               "{} and Opponent dish {}",
               cq.current_index, player_dish->id, opponent_dish->id);

      // Fire OnServe triggers for both dishes
      fire_onserve_trigger(*player_dish, player_dbs);
      fire_onserve_trigger(*opponent_dish, opponent_dbs);

      // Mark both dishes as having fired OnServe
      player_dbs.onserve_fired = true;
      opponent_dbs.onserve_fired = true;

      // Mark that we've fired OnServe for this course
      mark_onserve_fired_for_course(cq.current_index);
    }
    // Check if all courses have fired OnServe and we need to wait for ALL
    // FreshnessChain animations to complete
    else if (all_courses_have_fired_onserve() &&
             !has_created_global_slidein()) {
      // Check if trigger queue is empty (all OnServe triggers processed)
      if (auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
          tq.get().has<TriggerQueue>()) {
        auto &queue = tq.get().get<TriggerQueue>();
        bool queue_empty = queue.empty();
        bool no_freshness_chains_anywhere = !has_any_freshness_chain_active();

        if (queue_empty && no_freshness_chains_anywhere) {
          log_info("COMBAT: All OnServe triggers and ALL FreshnessChain "
                   "animations processed, "
                   "creating SlideIn animation for all courses");
          create_slidein_animation();
          // Mark that we've created the global SlideIn animation
          mark_global_slidein_created();
        }
      }
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
    // Get the trigger queue singleton
    if (auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
        tq.get().has<TriggerQueue>()) {
      auto &queue = tq.get().get<TriggerQueue>();

      // Add OnServe event for this dish
      queue.add_event(TriggerHook::OnServe, e.id, dbs.queue_index,
                      dbs.team_side);

      log_info("COMBAT: Fired OnServe trigger for dish {} (slot {}, team {})",
               e.id, dbs.queue_index,
               dbs.team_side == DishBattleState::TeamSide::Player ? "Player"
                                                                  : "Opponent");
    }
  }

  bool has_slidein_animation_created() {
    // Check if there's already a SlideIn animation event
    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<AnimationEvent>().gen()) {
      auto &ae = e.get<AnimationEvent>();
      if (ae.type == AnimationEventType::SlideIn) {
        return true;
      }
    }
    return false;
  }

  void mark_slidein_created_for_course(int course_index) {
    // Add a marker component to track that we've created a SlideIn for this
    // course We'll use the CombatQueue entity to store this information
    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<CombatQueue>().gen()) {
      auto &cq = e.get<CombatQueue>();
      if (cq.current_index == course_index) {
        // Add a custom component to mark this course as having SlideIn created
        // For now, we'll use a simple approach by checking if SlideIn animation
        // exists This prevents the infinite loop
        break;
      }
    }
  }

  bool has_processed_course_slidein(int course_index) {
    return processed_courses.find(course_index) != processed_courses.end();
  }

  void mark_course_slidein_processed(int course_index) {
    processed_courses.insert(course_index);
  }

  bool has_fired_onserve_for_course(int course_index) {
    return onserve_fired_courses.find(course_index) !=
           onserve_fired_courses.end();
  }

  void mark_onserve_fired_for_course(int course_index) {
    onserve_fired_courses.insert(course_index);
  }

  bool all_courses_have_fired_onserve() {
    // Check if all courses that have dishes have fired OnServe
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

  bool has_created_global_slidein() { return global_slidein_created; }

  void mark_global_slidein_created() { global_slidein_created = true; }

  bool has_onserve_animations_active(int course_index) {
    // Check if there are any OnServe-related animations still active for the
    // current course
    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<AnimationEvent>().gen()) {
      auto &ae = e.get<AnimationEvent>();
      if (ae.type == AnimationEventType::FreshnessChain ||
          ae.type == AnimationEventType::StatBoost) {
        // Check if this animation is related to the current course
        // For FreshnessChain, check if the source dish is in the current course
        if (ae.type == AnimationEventType::FreshnessChain &&
            e.has<FreshnessChainAnimation>()) {
          auto &fca = e.get<FreshnessChainAnimation>();
          if (is_dish_in_course(fca.sourceEntityId, course_index)) {
            return true;
          }
        }
        // For StatBoost, we'd need to check the source dish too, but for now
        // assume any StatBoost blocks
        else if (ae.type == AnimationEventType::StatBoost) {
          return true;
        }
      }
    }
    return false;
  }

  bool has_any_freshness_chain_active() {
    // Check if there are any FreshnessChain animations active anywhere
    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<AnimationEvent>().gen()) {
      auto &ae = e.get<AnimationEvent>();
      if (ae.type == AnimationEventType::FreshnessChain) {
        return true;
      }
    }
    return false;
  }

  bool is_dish_in_course(int dishEntityId, int course_index) {
    // Check if the dish is in the specified course
    for (afterhours::Entity &e : afterhours::EntityQuery()
                                     .whereHasComponent<IsDish>()
                                     .whereHasComponent<DishBattleState>()
                                     .gen()) {
      if (e.id == dishEntityId) {
        auto &dbs = e.get<DishBattleState>();
        return dbs.queue_index == course_index;
      }
    }
    return false;
  }

  void create_slidein_animation() {
    // Create a SlideIn animation event to block combat until OnServe effects
    // complete
    make_animation_event(AnimationEventType::SlideIn, true);
    log_info("COMBAT: Created SlideIn animation after all OnServe triggers "
             "processed");
  }
};
