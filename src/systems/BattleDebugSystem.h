#pragma once

#include "../components/battle_load_request.h"
#include "../components/battle_team_tags.h"
#include "../components/combat_queue.h"
#include "../components/is_dish.h"
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

    // Only run if BattleLoadRequest exists - check singleton map first to avoid crash
    // get_singleton() will crash if the singleton doesn't exist, so we must check first
    const auto componentId =
        afterhours::components::get_type_id<BattleLoadRequest>();
    auto &singletonMap = afterhours::EntityHelper::get().singletonMap;
    auto it = singletonMap.find(componentId);
    if (it == singletonMap.end() || it->second == nullptr) {
      return false;
    }

    // Safe to access now - use the entity pointer we found
    afterhours::Entity *entity = it->second;
    if (entity == nullptr) {
      return false;
    }
    return entity->has<BattleLoadRequest>();
  }

  void once(float) override {
    // Check if combat is active instead of judging
    bool combat_active =
        afterhours::EntityQuery().whereHasComponent<CombatQueue>().gen_count() >
        0;

    validate_two_teams(combat_active);
    validate_teams_have_dishes(combat_active);
  }

private:
  void validate_two_teams(bool combat_active) {
    size_t playerCount = afterhours::EntityQuery()
                             .whereHasComponent<IsPlayerTeamItem>()
                             .gen_count();

    size_t opponentCount = afterhours::EntityQuery()
                               .whereHasComponent<IsOpponentTeamItem>()
                               .gen_count();

    if (playerCount == 0 || opponentCount == 0) {
      if (!combat_active) {
        // Before combat starts we expect both sides; after that empties are ok
        if (playerCount == 0) {
          log_warn("Battle validation: No player team found");
        }
        if (opponentCount == 0) {
          log_warn("Battle validation: No opponent team found");
        }
      }
    }
  }

  void validate_teams_have_dishes(bool combat_active) {
    size_t playerDishCount = afterhours::EntityQuery()
                                 .whereHasComponent<IsPlayerTeamItem>()
                                 .whereHasComponent<IsDish>()
                                 .gen_count();

    size_t opponentDishCount = afterhours::EntityQuery()
                                   .whereHasComponent<IsOpponentTeamItem>()
                                   .whereHasComponent<IsDish>()
                                   .gen_count();

    if (playerDishCount == 0 || opponentDishCount == 0) {
      if (!combat_active) {
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
