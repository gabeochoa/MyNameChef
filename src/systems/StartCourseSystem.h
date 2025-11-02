#pragma once

#include "../components/animation_event.h"
#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../components/pairing_clash_modifiers.h"
#include "../components/persistent_combat_modifiers.h"
#include "../components/pre_battle_modifiers.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../render_backend.h"
#include "../shop.h"
#include <afterhours/ah.h>

struct StartCourseSystem : afterhours::System<CombatQueue> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    // TODO: Replace headless mode bypass with --disable-animation flag that calls
    // into vendor library (afterhours::animation) to properly disable animations
    // at the framework level instead of bypassing checks here
    bool animation_blocking = !render_backend::is_headless_mode && hasActiveAnimation();
    return gsm.active_screen == GameStateManager::Screen::Battle &&
           !animation_blocking;
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
    afterhours::OptEntity player_dish =
        find_dish_for_slot(cq.current_index, DishBattleState::TeamSide::Player);
    afterhours::OptEntity opponent_dish = find_dish_for_slot(
        cq.current_index, DishBattleState::TeamSide::Opponent);

    // (quiet)

    // Survivor carryover: if exactly one side is still in combat and the other
    // has finished for this course, bring in the next dish for the defeated
    // side
    if (player_dish && opponent_dish) {
      DishBattleState &player_dbs = player_dish->get<DishBattleState>();
      DishBattleState &opponent_dbs = opponent_dish->get<DishBattleState>();
      bool player_survived =
          player_dbs.phase == DishBattleState::Phase::InCombat &&
          opponent_dbs.phase == DishBattleState::Phase::Finished;
      bool opponent_survived =
          opponent_dbs.phase == DishBattleState::Phase::InCombat &&
          player_dbs.phase == DishBattleState::Phase::Finished;

      if (player_survived || opponent_survived) {
        int next_index = cq.current_index + 1;
        DishBattleState::TeamSide defeated_side =
            player_survived ? DishBattleState::TeamSide::Opponent
                            : DishBattleState::TeamSide::Player;
        auto next_dish = find_dish_for_slot(next_index, defeated_side);
        if (next_dish) {
          // Keep the same course index; retarget next dish into the current
          // slot
          DishBattleState &next_dbs = next_dish->get<DishBattleState>();
          next_dbs.queue_index = cq.current_index;
          next_dbs.phase = DishBattleState::Phase::Entering;
          next_dbs.enter_progress = -0.25f;
          log_info("COMBAT: Survivor carryover - retargeting {} side dish {} "
                   "from slot {} to current slot {}",
                   defeated_side == DishBattleState::TeamSide::Opponent
                       ? "Opponent"
                       : "Player",
                   next_dish->id, next_index, cq.current_index);
          return;
        } else {
          // No next dish available for the defeated side; battle concludes
          cq.complete = true;
          GameStateManager::get().to_results();
          return;
        }
      }
    }

    if (!player_dish || !opponent_dish) {
      // No more dishes, mark combat complete and transition to results
      cq.complete = true;
      log_info("COMBAT: No more dishes for slot {}, transitioning to results",
               cq.current_index);
      GameStateManager::get().to_results();
      return;
    }

    DishBattleState &player_dbs = player_dish->get<DishBattleState>();
    DishBattleState &opponent_dbs = opponent_dish->get<DishBattleState>();

    // Check if both dishes for current course are finished
    if (player_dbs.phase == DishBattleState::Phase::InCombat &&
        opponent_dbs.phase == DishBattleState::Phase::InCombat) {
      // Both dishes are done, wait for combat to finish
      return;
    }

    const float enter_start_delay = 0.25f;

    if (player_dbs.phase == DishBattleState::Phase::InQueue &&
        opponent_dbs.phase == DishBattleState::Phase::InQueue) {
      // Log modifier state before phase transition
      if (player_dish->has<PreBattleModifiers>()) {
        auto &playerPre = player_dish->get<PreBattleModifiers>();
        log_info("PHASE_TRANSITION: Player dish {} InQueue->Entering - "
                 "PreBattleModifiers: bodyDelta={}, zingDelta={}",
                 player_dish->id, playerPre.bodyDelta, playerPre.zingDelta);
      }
      if (player_dish->has<PairingClashModifiers>()) {
        auto &pcm = player_dish->get<PairingClashModifiers>();
        log_info("PHASE_TRANSITION: Player dish {} InQueue->Entering - "
                 "PairingClash: bodyDelta={}, zingDelta={}",
                 player_dish->id, pcm.bodyDelta, pcm.zingDelta);
      }
      if (player_dish->has<PersistentCombatModifiers>()) {
        auto &pm = player_dish->get<PersistentCombatModifiers>();
        log_info("PHASE_TRANSITION: Player dish {} InQueue->Entering - "
                 "Persistent: bodyDelta={}, zingDelta={}",
                 player_dish->id, pm.bodyDelta, pm.zingDelta);
      }
      if (opponent_dish->has<PreBattleModifiers>()) {
        auto &opponentPre = opponent_dish->get<PreBattleModifiers>();
        log_info("PHASE_TRANSITION: Opponent dish {} InQueue->Entering - "
                 "PreBattleModifiers: bodyDelta={}, zingDelta={}",
                 opponent_dish->id, opponentPre.bodyDelta,
                 opponentPre.zingDelta);
      }
      if (opponent_dish->has<PairingClashModifiers>()) {
        auto &pcm = opponent_dish->get<PairingClashModifiers>();
        log_info("PHASE_TRANSITION: Opponent dish {} InQueue->Entering - "
                 "PairingClash: bodyDelta={}, zingDelta={}",
                 opponent_dish->id, pcm.bodyDelta, pcm.zingDelta);
      }
      if (opponent_dish->has<PersistentCombatModifiers>()) {
        auto &pm = opponent_dish->get<PersistentCombatModifiers>();
        log_info("PHASE_TRANSITION: Opponent dish {} InQueue->Entering - "
                 "Persistent: bodyDelta={}, zingDelta={}",
                 opponent_dish->id, pm.bodyDelta, pm.zingDelta);
      }

      player_dbs.phase = DishBattleState::Phase::Entering;
      player_dbs.enter_progress = -enter_start_delay;
      opponent_dbs.phase = DishBattleState::Phase::Entering;
      opponent_dbs.enter_progress = -enter_start_delay;
      log_info("COMBAT: Starting course {} - both dishes entering head to head",
               cq.current_index);

      if (auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
          tq.get().has<TriggerQueue>()) {
        auto &queue = tq.get().get<TriggerQueue>();
        queue.add_event(TriggerHook::OnCourseStart, 0, cq.current_index,
                        DishBattleState::TeamSide::Player);
        log_info("COMBAT: Fired OnCourseStart trigger for slot {}",
                 cq.current_index);
      }
    }
  }

private:
  afterhours::OptEntity find_dish_for_slot(int slot_index,
                                           DishBattleState::TeamSide side) {
    return EQ()
        .whereHasComponent<DishBattleState>()
        .whereInSlotIndex(slot_index)
        .whereTeamSide(side)
        .whereLambda([](const afterhours::Entity &e) {
          const DishBattleState &dbs = e.get<DishBattleState>();
          return dbs.phase == DishBattleState::Phase::InQueue ||
                 dbs.phase == DishBattleState::Phase::Entering ||
                 dbs.phase == DishBattleState::Phase::InCombat;
        })
        .gen_first();
  }

  bool prerequisites_complete() {
    // TODO: Replace headless mode bypass with --disable-animation flag that calls
    // into vendor library (afterhours::animation) to properly disable animations
    // at the framework level instead of bypassing checks here
    if (render_backend::is_headless_mode) {
      // In headless mode, skip animation checks and only check if OnServe fired
      afterhours::RefEntities unfinishedDishes =
          afterhours::EntityQuery()
              .whereHasComponent<IsDish>()
              .whereHasComponent<DishBattleState>()
              .whereLambda([](const afterhours::Entity &e) {
                const DishBattleState &dbs = e.get<DishBattleState>();
                return dbs.phase == DishBattleState::Phase::InQueue &&
                       !dbs.onserve_fired;
              })
              .gen();
      return unfinishedDishes.empty();
    }

    bool slideInComplete = true;
    for (afterhours::Entity &animEntity :
         afterhours::EntityQuery()
             .whereHasComponent<AnimationEvent>()
             .whereHasComponent<AnimationTimer>()
             .gen()) {
      AnimationEvent &ev = animEntity.get<AnimationEvent>();
      if (ev.type == AnimationEventType::SlideIn) {
        slideInComplete = false;
        break;
      }
    }

    if (!slideInComplete) {
      return false;
    }

    afterhours::RefEntities unfinishedDishes =
        afterhours::EntityQuery()
            .whereHasComponent<IsDish>()
            .whereHasComponent<DishBattleState>()
            .whereLambda([](const afterhours::Entity &e) {
              const DishBattleState &dbs = e.get<DishBattleState>();
              return dbs.phase == DishBattleState::Phase::InQueue &&
                     !dbs.onserve_fired;
            })
            .gen();
    if (!unfinishedDishes.empty()) {
      return false;
    }

    if (hasActiveAnimation()) {
      return false;
    }

    return true;
  }
};
