#include "server_context.h"
#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../game_state_manager.h"
#include "../render_backend.h"
#include "../seeded_rng.h"
#include "../shop.h"
#include "../systems/battle_system_registry.h"
#include <afterhours/ah.h>

namespace server {
ServerContext ServerContext::initialize() {
  ServerContext ctx;

  render_backend::is_headless_mode = true;

  // Manager entity is not needed - singletons are initialized in main()
  ctx.manager_entity = nullptr;

  GameStateManager::get().set_next_screen(GameStateManager::Screen::Battle);
  GameStateManager::get().update_screen();

  ctx.initialize_singletons();

  ctx.register_battle_systems();

  return ctx;
}

void ServerContext::initialize_singletons() {
  // Combat and battle processor managers are initialized once in main()
  // We just ensure the manager entity exists for any additional components
  // (currently none, but kept for future extensibility)

  (void)SeededRng::get();
}

void ServerContext::register_battle_systems() {
  battle_systems::register_battle_systems(systems);
}

bool ServerContext::is_battle_complete() const {
  afterhours::RefEntity cq_entity =
      afterhours::EntityHelper::get_singleton<CombatQueue>();
  if (cq_entity.get().has<CombatQueue>()) {
    const CombatQueue &cq = cq_entity.get().get<CombatQueue>();
    if (cq.complete) {
      return true;
    }

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

    return !player_has_active || !opponent_has_active;
  }
  return false;
}
} // namespace server
