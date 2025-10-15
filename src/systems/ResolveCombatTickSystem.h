#pragma once

#include "../components/battle_result.h"
#include "../components/combat_stats.h"
#include "../components/dish_battle_state.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

struct ResolveCombatTickSystem
    : afterhours::System<DishBattleState, CombatStats> {
  static constexpr float kTickMs = 150.0f / 1000.0f; // 150ms bite cadence

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    bool should_run = gsm.active_screen == GameStateManager::Screen::Battle;
    // Debug: comment out to reduce spam
    // log_info("COMBAT: ResolveCombatTickSystem should_run: {} (screen: {})",
    //          should_run,
    //          (gsm.active_screen == GameStateManager::Screen::Battle) ?
    //          "Battle"
    //                                                                  :
    //                                                                  "Other");
    return should_run;
  }

  void for_each_with(afterhours::Entity &e, DishBattleState &dbs,
                     CombatStats &cs, float dt) override {
    // Debug: comment out to reduce spam
    // log_info("COMBAT: ResolveCombatTickSystem processing entity {} - phase:
    // {}",
    //          e.id,
    //          (dbs.phase == DishBattleState::Phase::InCombat) ? "InCombat"
    //                                                          : "Other");

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

    // Alternate turns: player goes first, then opponent
    static bool player_turn = true;

    bool did_bite = false;
    if (player_turn && dbs.team_side == DishBattleState::TeamSide::Player) {
      // Player bites opponent
      int damage = cs.currentZing;
      if (damage <= 0)
        damage = 1; // minimal damage fallback to avoid stalemates
      opponent_cs.currentBody -= damage;
      did_bite = true;
      // log_info("COMBAT: Bite slot {} -> P zing:{} body:{} | O zing:{} body:{} (P->O dmg:{})",
      //          dbs.queue_index, cs.currentZing, cs.currentBody,
      //          opponent_cs.currentZing, opponent_cs.currentBody, damage);
    } else if (!player_turn &&
               dbs.team_side == DishBattleState::TeamSide::Opponent) {
      // Opponent bites player
      int damage = opponent_cs.currentZing;
      if (damage <= 0)
        damage = 1; // minimal damage fallback to avoid stalemates
      cs.currentBody -= damage;
      did_bite = true;
      // log_info("COMBAT: Bite slot {} -> P zing:{} body:{} | O zing:{} body:{} (O->P dmg:{})",
      //          dbs.queue_index, cs.currentZing, cs.currentBody,
      //          opponent_cs.currentZing, opponent_cs.currentBody, damage);
    }

    if (did_bite) {
      player_turn = !player_turn; // Alternate turns only after a bite
    }

    // Check if either dish is defeated
    if (cs.currentBody <= 0 || opponent_cs.currentBody <= 0) {
      finish_course(e, opponent, dbs, opponent_dbs);
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
