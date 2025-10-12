#pragma once

#include "../components/battle_load_request.h"
#include "../components/battle_team_tags.h"
#include "../components/is_dish.h"
#include "../components/judging_state.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

/**
 * BattleDebugSystem - Validates battle screen setup
 *
 * This system runs only on the battle screen and validates that:
 * - Both player and opponent teams exist
 * - Both teams have dishes
 *
 * Only runs when BattleLoadRequest singleton exists to prevent crashes.
 */
struct BattleDebugSystem : afterhours::System<> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Battle) {
      return false;
    }

    // Only run if BattleLoadRequest exists
    auto battleRequest =
        afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    return battleRequest.get().has<BattleLoadRequest>();
  }

  void once(float) override {
    // If judging is in progress, allow teams to be empty without error
    bool judging_active = afterhours::EntityQuery()
                              .whereHasComponent<JudgingState>()
                              .gen_count() > 0;

    validate_two_teams(judging_active);
    validate_teams_have_dishes(judging_active);
  }

private:
  void validate_two_teams(bool judging_active) {
    size_t playerCount = afterhours::EntityQuery()
                             .whereHasComponent<IsPlayerTeamItem>()
                             .gen_count();

    size_t opponentCount = afterhours::EntityQuery()
                               .whereHasComponent<IsOpponentTeamItem>()
                               .gen_count();

    if (playerCount == 0 || opponentCount == 0) {
      if (!judging_active) {
        // Before judging starts we expect both sides; after that empties are ok
        if (playerCount == 0) {
          log_warn("Battle validation: No player team found");
        }
        if (opponentCount == 0) {
          log_warn("Battle validation: No opponent team found");
        }
      }
    }
  }

  void validate_teams_have_dishes(bool judging_active) {
    size_t playerDishCount = afterhours::EntityQuery()
                                 .whereHasComponent<IsPlayerTeamItem>()
                                 .whereHasComponent<IsDish>()
                                 .gen_count();

    size_t opponentDishCount = afterhours::EntityQuery()
                                   .whereHasComponent<IsOpponentTeamItem>()
                                   .whereHasComponent<IsDish>()
                                   .gen_count();

    if (playerDishCount == 0 || opponentDishCount == 0) {
      if (!judging_active) {
        if (playerDishCount == 0) {
          log_warn("Battle validation: Player team has no dishes");
        }
        if (opponentDishCount == 0) {
          log_warn("Battle validation: Opponent team has no dishes");
        }
      }
    }
  }
};
