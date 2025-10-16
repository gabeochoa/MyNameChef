#pragma once

#include "../components/battle_result.h"
#include "../components/combat_stats.h"
#include "../components/dish_battle_state.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

struct ResolveCombatTickSystem
    : afterhours::System<DishBattleState, CombatStats> {
  static constexpr float kTickMs = 150.0f / 1000.0f; // 150ms bite cadence

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
    bool should_run = gsm.active_screen == GameStateManager::Screen::Battle;
    return should_run;
  }

  void for_each_with(afterhours::Entity &e, DishBattleState &dbs,
                     CombatStats &cs, float dt) override {
    if (dbs.phase != DishBattleState::Phase::InCombat)
      return;

    dbs.bite_timer += dt;
    if (dbs.bite_timer < kTickMs)
      return;

    dbs.bite_timer = 0.0f; // Reset timer

    // Find opponent dish for this slot
    afterhours::OptEntity opt_opponent = find_opponent_dish(dbs);
    if (!opt_opponent)
      return;
    afterhours::Entity &opponent = opt_opponent.asE();

    DishBattleState &opponent_dbs = opponent.get<DishBattleState>();
    CombatStats &opponent_cs = opponent.get<CombatStats>();

    // Determine who goes first based on dish stats, then alternate turns
    // Tiebreakers: 1) Highest Zing, 2) Highest Body, 3) Deterministic fallback
    bool player_goes_first = determine_first_attacker(cs, opponent_cs);
    
    // Simple alternating turns: first bite uses stats, then alternate
    static bool current_turn_player = true; // Reset per battle
    bool player_turn = current_turn_player;
    
    // For the very first bite of this pairing, use stat-based determination
    if (dbs.bite_timer == 0.0f) {
      player_turn = player_goes_first;
      current_turn_player = player_goes_first;
    }

    bool did_bite = false;
    if (player_turn && dbs.team_side == DishBattleState::TeamSide::Player) {
      // Player bites opponent
      int damage = cs.currentZing;
      if (damage <= 0)
        damage = 1; // minimal damage fallback to avoid stalemates
      opponent_cs.currentBody -= damage;
      did_bite = true;
    } else if (!player_turn &&
               dbs.team_side == DishBattleState::TeamSide::Opponent) {
      // Opponent bites player
      int damage = opponent_cs.currentZing;
      if (damage <= 0)
        damage = 1; // minimal damage fallback to avoid stalemates
      cs.currentBody -= damage;
      did_bite = true;
    }

    if (did_bite) {
      current_turn_player = !current_turn_player; // Alternate turns
    }

    // Check if either dish is defeated
    if (cs.currentBody <= 0 || opponent_cs.currentBody <= 0) {
      finish_course(e, opponent, dbs, opponent_dbs);

      // If one dish is defeated but the other is still alive,
      // the surviving dish should fight the next opponent dish
      if (cs.currentBody > 0 && opponent_cs.currentBody <= 0) {
        // Player dish survived, find next opponent dish
        advance_to_next_opponent(e, dbs);
      } else if (opponent_cs.currentBody > 0 && cs.currentBody <= 0) {
        // Opponent dish survived, find next player dish
        advance_to_next_opponent(opponent, opponent_dbs);
      }
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

  // Advance surviving dish to fight next opponent
  void advance_to_next_opponent(afterhours::Entity &surviving_dish,
                                DishBattleState &surviving_dbs) {
    // Find next available opponent dish
    afterhours::OptEntity next_opponent =
        find_next_available_opponent(surviving_dbs);

    if (next_opponent) {
      afterhours::Entity &opponent = next_opponent.asE();
      DishBattleState &opponent_dbs = opponent.get<DishBattleState>();

      // Set both dishes to InCombat
      surviving_dbs.phase = DishBattleState::Phase::InCombat;
      opponent_dbs.phase = DishBattleState::Phase::InCombat;

      // Reset bite timer for both dishes
      surviving_dbs.bite_timer = 0.0f;
      opponent_dbs.bite_timer = 0.0f;

      log_info(
          "COMBAT: Dish {} advances to fight next opponent {}",
          surviving_dish.id, opponent.id);
    } else {
      // No more opponents, this dish wins the entire battle
      surviving_dbs.phase = DishBattleState::Phase::Finished;
      log_info("COMBAT: Dish {} has no more opponents - battle complete!",
               surviving_dish.id);
    }
  }

  // Find next available opponent dish
  afterhours::OptEntity
  find_next_available_opponent(const DishBattleState &surviving_dbs) {
    DishBattleState::TeamSide opponent_side =
        (surviving_dbs.team_side == DishBattleState::TeamSide::Player)
            ? DishBattleState::TeamSide::Opponent
            : DishBattleState::TeamSide::Player;

    // Look for next opponent dish that's in queue or entering
    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
      DishBattleState &other_dbs = e.get<DishBattleState>();
      if (other_dbs.team_side == opponent_side &&
          (other_dbs.phase == DishBattleState::Phase::InQueue ||
           other_dbs.phase == DishBattleState::Phase::Entering)) {
        return afterhours::OptEntity(e);
      }
    }
    return afterhours::OptEntity();
  }

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

    log_info(
        "COMBAT: Course {} finished - Winner: {}", player_dbs.queue_index + 1,
        (winner == BattleResult::CourseOutcome::Winner::Player)     ? "Player"
        : (winner == BattleResult::CourseOutcome::Winner::Opponent) ? "Opponent"
                                                                    : "Tie");
  }
};
