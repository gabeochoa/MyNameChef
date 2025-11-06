#pragma once

#include "../../../components/dish_battle_state.h"
#include "../../../components/is_dish.h"
#include "../../../log.h"
#include "../components/battle_info.h"
#include <afterhours/ah.h>

namespace server::async {
struct DetectBattleCompletionSystem : afterhours::System<BattleInfo> {
  void for_each_with(afterhours::Entity &, BattleInfo &info, float) override {
    if (info.status != BattleStatus::Running) {
      return;
    }

    // Check if battle is complete by looking at active dishes in the ECS world
    // Since only one battle runs at a time, we can check the global dish state
    bool player_has_active = false;
    bool opponent_has_active = false;

    for (afterhours::Entity &entity :
         afterhours::EntityQuery({.force_merge = true})
             .whereHasComponent<IsDish>()
             .whereHasComponent<DishBattleState>()
             .gen()) {
      const DishBattleState &dbs = entity.get<DishBattleState>();
      if (dbs.phase != DishBattleState::Phase::Finished) {
        if (dbs.team_side == DishBattleState::TeamSide::Player) {
          player_has_active = true;
        } else {
          opponent_has_active = true;
        }
      }
    }

    bool is_complete = !player_has_active || !opponent_has_active;

    if (is_complete) {
      log_info("SERVER_BATTLE: Battle {} completed", info.battleId);
      info.status = BattleStatus::Complete;
    }
  }
};
} // namespace server::async
