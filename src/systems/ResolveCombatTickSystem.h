#pragma once

#include "../components/battle_anim_keys.h"
#include "../components/battle_result.h"
#include "../components/combat_stats.h"
#include "../components/dish_battle_state.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../shop.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/animation.h>

struct ResolveCombatTickSystem
    : afterhours::System<DishBattleState, CombatStats> {
  static constexpr float kTickMs = 150.0f / 1000.0f; // 150ms bite cadence
  static constexpr float kPrePauseMs = 0.35f;        // pause before damage
  static constexpr float kPostPauseMs = 0.35f;       // pause after damage

private:
  // Determine who goes first based on dish stats
  // Tiebreakers: 1) Highest Zing, 2) Highest Body, 3) Deterministic fallback
  bool determine_first_attacker(const CombatStats &player_cs,
                                const CombatStats &opponent_cs) {
    // Use base stats for tiebreakers (initial values, not current combat
    // values) Tiebreaker 1: Highest Zing
    if (player_cs.baseZing > opponent_cs.baseZing) {
      return true; // Player goes first
    } else if (opponent_cs.baseZing > player_cs.baseZing) {
      return false; // Opponent goes first
    }

    // Tiebreaker 2: Highest Body (if Zing is tied)
    if (player_cs.baseBody > opponent_cs.baseBody) {
      return true; // Player goes first
    } else if (opponent_cs.baseBody > player_cs.baseBody) {
      return false; // Opponent goes first
    }

    // Tiebreaker 3: Deterministic fallback (if both Zing and Body are tied)
    // Since we don't have separate health, use a deterministic rule
    // Player goes first as a consistent fallback
    return true;
  }

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    bool should_run = gsm.active_screen == GameStateManager::Screen::Battle &&
                      !hasActiveAnimation();
    return should_run;
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
    // entities
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

    // On entering InCombat for the first time, decide turn and start with a
    // pre-pause
    if (!dbs.first_bite_decided) {
      // Do not start cadence until movement animation has fully finished
      if (dbs.enter_progress < 1.0f ||
          opponent.get<DishBattleState>().enter_progress < 1.0f) {
        return;
      }

      CombatStats &opponent_cs = opponent.get<CombatStats>();
      bool player_goes_first = determine_first_attacker(cs, opponent_cs);
      dbs.players_turn = player_goes_first;
      dbs.first_bite_decided = true;
      dbs.bite_cadence = DishBattleState::BiteCadence::PrePause;
      dbs.bite_cadence_timer = 0.0f;
      return; // do not deal damage immediately
    }

    dbs.bite_cadence_timer += dt;

    DishBattleState &opponent_dbs = opponent.get<DishBattleState>();
    CombatStats &opponent_cs = opponent.get<CombatStats>();

    // Handle cadence state machine
    if (dbs.bite_cadence == DishBattleState::BiteCadence::PrePause) {
      if (dbs.bite_cadence_timer < kPrePauseMs)
        return;

      // Time to apply damage and trigger animation
      dbs.bite_cadence_timer = 0.0f;

      bool player_turn = dbs.players_turn;
      int damage = 0;
      int target_id = -1;
      DishBattleState::TeamSide attacker_side =
          DishBattleState::TeamSide::Player;
      if (player_turn) {
        damage = cs.currentZing;
        if (damage <= 0)
          damage = 1;
        opponent_cs.currentBody -= damage;
        target_id = opponent.id;
        attacker_side = DishBattleState::TeamSide::Player;
      } else {
        damage = opponent_cs.currentZing;
        if (damage <= 0)
          damage = 1;
        cs.currentBody -= damage;
        target_id = e.id;
        attacker_side = DishBattleState::TeamSide::Opponent;
      }

      if (auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
          tq.get().has<TriggerQueue>()) {
        auto &queue = tq.get().get<TriggerQueue>();
        int attacker_id = player_turn ? e.id : opponent.id;
        queue.add_event(TriggerHook::OnBiteTaken, attacker_id, dbs.queue_index,
                        attacker_side);
        queue.events.back().payloadInt = damage;
      }

      // Emit a blocking animation event to visualize damage (negative
      // bodyDelta)
      auto &anim = make_animation_event(AnimationEventType::StatBoost, true);
      auto &animData = anim.get<AnimationEvent>();
      animData.data = StatBoostData{target_id, 0, -damage};

      // Next state: post-pause, then flip turn
      dbs.bite_cadence = DishBattleState::BiteCadence::PostPause;
      return;
    }

    if (dbs.bite_cadence == DishBattleState::BiteCadence::PostPause) {
      if (dbs.bite_cadence_timer < kPostPauseMs)
        return;

      // Advance turn and go back to pre-pause
      dbs.bite_cadence_timer = 0.0f;
      dbs.players_turn = !dbs.players_turn;
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
    }
  }

private:
  afterhours::OptEntity find_opponent_dish(const DishBattleState &dbs) {
    DishBattleState::TeamSide opponent_side =
        (dbs.team_side == DishBattleState::TeamSide::Player)
            ? DishBattleState::TeamSide::Opponent
            : DishBattleState::TeamSide::Player;

    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
      DishBattleState &other_dbs = e.get<DishBattleState>();
      if (other_dbs.team_side == opponent_side &&
          other_dbs.queue_index == dbs.queue_index &&
          other_dbs.phase == DishBattleState::Phase::InCombat) {
        return afterhours::OptEntity(e);
      }
    }
    return afterhours::OptEntity();
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
