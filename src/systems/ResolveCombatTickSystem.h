#pragma once

#include "../battle_timing.h"
#include "../components/battle_anim_keys.h"
#include "../components/battle_result.h"
#include "../components/combat_stats.h"
#include "../components/dish_battle_state.h"
#include "../components/transform.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../render_backend.h"
#include "../render_constants.h"
#include "../shop.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/animation.h>
#include <afterhours/src/plugins/texture_manager.h>

struct ResolveCombatTickSystem
    : afterhours::System<DishBattleState, CombatStats> {

private:
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Battle) {
      return false;
    }
    if (isReplayPaused()) {
      return false;
    }
    // Check for blocking animations - in headless mode hasActiveAnimation()
    // returns false
    return !hasActiveAnimation();
  }

  void for_each_with(afterhours::Entity &e, DishBattleState &dbs,
                     CombatStats &cs, float dt) override {
    if (dbs.phase != DishBattleState::Phase::InCombat)
      return;
    // Drive cadence from the Player-side entity only to avoid double
    // progression
    if (dbs.team_side != DishBattleState::TeamSide::Player)
      return;

    // Find opponent dish for this slot
    afterhours::OptEntity opt_opponent = find_opponent_dish(dbs);
    if (!opt_opponent)
      return;
    afterhours::Entity &opponent = opt_opponent.asE();

    // Hard gate: ensure the SlideIn animation has fully finished for both
    // entities In headless mode, animations complete instantly so slide values
    // are always 1.0
    auto sv_player =
        afterhours::animation::get_value(BattleAnimKey::SlideIn, (size_t)e.id);
    auto sv_opponent = afterhours::animation::get_value(BattleAnimKey::SlideIn,
                                                        (size_t)opponent.id);
    const float slide_player = sv_player.has_value() ? sv_player.value() : 1.0f;
    const float slide_opponent =
        sv_opponent.has_value() ? sv_opponent.value() : 1.0f;
    if (slide_player < 1.0f || slide_opponent < 1.0f) {
      return; // do not progress combat until slide-in visually completes
    }

    // On entering InCombat for the first time, start with a pre-pause
    if (!dbs.first_bite_decided) {
      // Do not start cadence until movement animation has fully finished
      // In headless mode, enter_progress is set to 1.0 immediately, so this
      // check passes
      if (dbs.enter_progress < 1.0f ||
          opponent.get<DishBattleState>().enter_progress < 1.0f) {
        return;
      }

      dbs.first_bite_decided = true;
      dbs.bite_cadence = DishBattleState::BiteCadence::PrePause;
      dbs.bite_cadence_timer = 0.0f;
      return; // do not deal damage immediately
    }

    // Ensure forward progress even if dt is zero due to timing anomalies in
    // headless mode
    const float kFallbackDt = 1.0f / 60.0f;
    float effective_dt = dt > 0.0f ? dt : kFallbackDt;
    dbs.bite_cadence_timer += effective_dt;

    DishBattleState &opponent_dbs = opponent.get<DishBattleState>();
    CombatStats &opponent_cs = opponent.get<CombatStats>();

    // Handle cadence state machine
    if (dbs.bite_cadence == DishBattleState::BiteCadence::PrePause) {
      float scaled_pre_pause = BattleTiming::get_pre_pause();
      if (dbs.bite_cadence_timer < scaled_pre_pause)
        return;

      // Time to apply damage - both dishes attack simultaneously
      dbs.bite_cadence_timer = 0.0f;

      // Player dish attacks opponent
      int player_damage = cs.currentZing;
      if (player_damage <= 0)
        player_damage = 1;
      opponent_cs.currentBody -= player_damage;

      // Opponent dish attacks player
      int opponent_damage = opponent_cs.currentZing;
      if (opponent_damage <= 0)
        opponent_damage = 1;
      cs.currentBody -= opponent_damage;

      // Fire OnBiteTaken events for both attacks
      if (auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
          tq.get().has<TriggerQueue>()) {
        auto &queue = tq.get().get<TriggerQueue>();
        // Player's attack on opponent
        queue.add_event(TriggerHook::OnBiteTaken, e.id, dbs.queue_index,
                        DishBattleState::TeamSide::Player);
        queue.events.back().payloadInt = player_damage;
        // Opponent's attack on player
        queue.add_event(TriggerHook::OnBiteTaken, opponent.id, dbs.queue_index,
                        DishBattleState::TeamSide::Opponent);
        queue.events.back().payloadInt = opponent_damage;
      }

      // Emit animation events for both attacks
      // Player's attack visualization on opponent
      auto &anim_player =
          make_animation_event(AnimationEventType::StatBoost, true);
      auto &animPlayerData = anim_player.get<AnimationEvent>();
      animPlayerData.data = StatBoostData{opponent.id, 0, -player_damage};

      // Opponent's attack visualization on player
      auto &anim_opponent =
          make_animation_event(AnimationEventType::StatBoost, true);
      auto &animOpponentData = anim_opponent.get<AnimationEvent>();
      animOpponentData.data = StatBoostData{e.id, 0, -opponent_damage};

      // Next state: post-pause
      dbs.bite_cadence = DishBattleState::BiteCadence::PostPause;
      return;
    }

    if (dbs.bite_cadence == DishBattleState::BiteCadence::PostPause) {
      float scaled_post_pause = BattleTiming::get_post_pause();
      if (dbs.bite_cadence_timer < scaled_post_pause)
        return;

      // Advance to next simultaneous attack
      dbs.bite_cadence_timer = 0.0f;
      dbs.bite_cadence = DishBattleState::BiteCadence::PrePause;
    }

    // Check if either dish is defeated; only mark the defeated dish finished
    if (cs.currentBody <= 0 && opponent_cs.currentBody <= 0) {
      dbs.phase = DishBattleState::Phase::Finished;
      opponent_dbs.phase = DishBattleState::Phase::Finished;

      if (auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
          tq.get().has<TriggerQueue>()) {
        auto &queue = tq.get().get<TriggerQueue>();
        queue.add_event(TriggerHook::OnDishFinished, e.id, dbs.queue_index,
                        DishBattleState::TeamSide::Player);
        queue.add_event(TriggerHook::OnDishFinished, opponent.id,
                        dbs.queue_index, DishBattleState::TeamSide::Opponent);
      }

      log_info("COMBAT: Both dishes defeated at slot {} (tie)",
               dbs.queue_index);

      reorganize_queues();
    } else if (cs.currentBody <= 0) {
      dbs.phase = DishBattleState::Phase::Finished;

      if (auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
          tq.get().has<TriggerQueue>()) {
        auto &queue = tq.get().get<TriggerQueue>();
        queue.add_event(TriggerHook::OnDishFinished, e.id, dbs.queue_index,
                        DishBattleState::TeamSide::Player);
      }

      log_info("COMBAT: Player-side dish {} defeated at slot {}", e.id,
               dbs.queue_index);

      reorganize_queues();
    } else if (opponent_cs.currentBody <= 0) {
      opponent_dbs.phase = DishBattleState::Phase::Finished;

      if (auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
          tq.get().has<TriggerQueue>()) {
        auto &queue = tq.get().get<TriggerQueue>();
        queue.add_event(TriggerHook::OnDishFinished, opponent.id,
                        dbs.queue_index, DishBattleState::TeamSide::Opponent);
      }

      log_info("COMBAT: Opponent-side dish {} defeated at slot {}", opponent.id,
               dbs.queue_index);

      reorganize_queues();
    }
  }

private:
  afterhours::OptEntity find_opponent_dish(const DishBattleState &dbs) {
    DishBattleState::TeamSide opponent_side =
        (dbs.team_side == DishBattleState::TeamSide::Player)
            ? DishBattleState::TeamSide::Opponent
            : DishBattleState::TeamSide::Player;

    // Always look for opponent at index 0 (queues are reorganized when dishes
    // finish)
    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
      DishBattleState &other_dbs = e.get<DishBattleState>();
      if (other_dbs.team_side == opponent_side && other_dbs.queue_index == 0 &&
          other_dbs.phase == DishBattleState::Phase::InCombat) {
        return afterhours::OptEntity(e);
      }
    }
    return afterhours::OptEntity();
  }

  void reorganize_queues() {
    for (DishBattleState::TeamSide side :
         {DishBattleState::TeamSide::Player,
          DishBattleState::TeamSide::Opponent}) {
      afterhours::RefEntities active_dishes =
          EQ({.ignore_temp_warning = true})
              .whereHasComponent<DishBattleState>()
              .whereTeamSide(side)
              .whereLambda([](const afterhours::Entity &e) {
                const DishBattleState &dbs = e.get<DishBattleState>();
                return dbs.phase == DishBattleState::Phase::InQueue ||
                       dbs.phase == DishBattleState::Phase::Entering ||
                       dbs.phase == DishBattleState::Phase::InCombat;
              })
              .orderByLambda(
                  [](const afterhours::Entity &a, const afterhours::Entity &b) {
                    return a.get<DishBattleState>().queue_index <
                           b.get<DishBattleState>().queue_index;
                  })
              .gen();

      int new_index = 0;
      for (afterhours::Entity &dish : active_dishes) {
        DishBattleState &dbs = dish.get<DishBattleState>();
        bool was_reorganized = dbs.queue_index != new_index;
        if (was_reorganized) {
          log_info("COMBAT: Reorganizing {} side dish {} from index {} to {}",
                   side == DishBattleState::TeamSide::Player ? "Player"
                                                             : "Opponent",
                   dish.id, dbs.queue_index, new_index);
          dbs.queue_index = new_index;

          if (dish.has<Transform>()) {
            float y = (side == DishBattleState::TeamSide::Player) ? 150.0f : 500.0f;
            float x = 120.0f + new_index * 100.0f;
            Transform &transform = dish.get<Transform>();
            transform.position = afterhours::vec2{x, y};

            if (dish.has<afterhours::texture_manager::HasSprite>()) {
              dish.get<afterhours::texture_manager::HasSprite>().update_transform(
                  transform.position, transform.size, transform.angle);
            }
          }
        }

        // Reset dishes that were InCombat or Entering to InQueue so they can
        // start new fights (survivors from previous combat need to reset combat
        // state)
        if (dbs.phase == DishBattleState::Phase::InCombat ||
            dbs.phase == DishBattleState::Phase::Entering) {
          const char *old_phase =
              (dbs.phase == DishBattleState::Phase::InCombat) ? "InCombat"
                                                              : "Entering";
          dbs.phase = DishBattleState::Phase::InQueue;
          dbs.enter_progress = 0.0f;
          dbs.first_bite_decided = false;
          dbs.bite_cadence = DishBattleState::BiteCadence::PrePause;
          dbs.bite_cadence_timer = 0.0f;
          log_info("COMBAT: Resetting {} side dish {} from {} to InQueue after "
                   "reorganization",
                   side == DishBattleState::TeamSide::Player ? "Player"
                                                             : "Opponent",
                   dish.id, old_phase);
        }

        new_index++;
      }
    }
  }

  // (no intra-course chaining; course advancement handled by Start/Advance
  // systems)

  void finish_course(afterhours::Entity &player, afterhours::Entity &opponent,
                     DishBattleState &player_dbs,
                     DishBattleState &opponent_dbs) {
    // Determine winner
    auto &player_cs = player.get<CombatStats>();
    auto &opponent_cs = opponent.get<CombatStats>();

    BattleResult::CourseOutcome::Winner winner;
    if (player_cs.currentBody <= 0 && opponent_cs.currentBody <= 0) {
      winner = BattleResult::CourseOutcome::Winner::Tie;
    } else if (player_cs.currentBody <= 0) {
      winner = BattleResult::CourseOutcome::Winner::Opponent;
    } else {
      winner = BattleResult::CourseOutcome::Winner::Player;
    }

    // Set both dishes to finished
    player_dbs.phase = DishBattleState::Phase::Finished;
    opponent_dbs.phase = DishBattleState::Phase::Finished;

    // Log modifier state when dishes finish
    if (player.has<PreBattleModifiers>()) {
      auto &playerPre = player.get<PreBattleModifiers>();
      log_info("COMBAT_FINISH: Player dish {} finished - PreBattleModifiers: "
               "zingDelta={}, bodyDelta={}",
               player.id, playerPre.zingDelta, playerPre.bodyDelta);
    }
    if (opponent.has<PreBattleModifiers>()) {
      auto &opponentPre = opponent.get<PreBattleModifiers>();
      log_info("COMBAT_FINISH: Opponent dish {} finished - PreBattleModifiers: "
               "zingDelta={}, bodyDelta={}",
               opponent.id, opponentPre.zingDelta, opponentPre.bodyDelta);
    }

    log_info(
        "COMBAT: Course {} finished - Winner: {}", player_dbs.queue_index + 1,
        (winner == BattleResult::CourseOutcome::Winner::Player)     ? "Player"
        : (winner == BattleResult::CourseOutcome::Winner::Opponent) ? "Opponent"
                                                                    : "Tie");
  }
};
