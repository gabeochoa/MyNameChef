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

    // On entering InCombat for the first time, start with a pre-pause
    if (!dbs.first_bite_decided) {
      // Do not start cadence until movement animation has fully finished
      if (dbs.enter_progress < 1.0f ||
          opponent.get<DishBattleState>().enter_progress < 1.0f) {
        return;
      }

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
      auto &anim_player = make_animation_event(AnimationEventType::StatBoost, true);
      auto &animPlayerData = anim_player.get<AnimationEvent>();
      animPlayerData.data = StatBoostData{opponent.id, 0, -player_damage};

      // Opponent's attack visualization on player
      auto &anim_opponent = make_animation_event(AnimationEventType::StatBoost, true);
      auto &animOpponentData = anim_opponent.get<AnimationEvent>();
      animOpponentData.data = StatBoostData{e.id, 0, -opponent_damage};

      // Next state: post-pause
      dbs.bite_cadence = DishBattleState::BiteCadence::PostPause;
      return;
    }

    if (dbs.bite_cadence == DishBattleState::BiteCadence::PostPause) {
      if (dbs.bite_cadence_timer < kPostPauseMs)
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
