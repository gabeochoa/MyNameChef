#include "server_context.h"
#include "../components/battle_result.h"
#include "../components/battle_team_tags.h"
#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../preload.h"
#include "../query.h"
#include "../seeded_rng.h"
#include "../shop.h"
#include "../systems/battle_system_registry.h"
#include <afterhours/ah.h>

namespace server {
ServerContext ServerContext::initialize() {
  ServerContext ctx;

  render_backend::is_headless_mode = true;

  Preload::get().init("battle_server", true).make_singleton();

  auto &entity = afterhours::EntityHelper::createEntity();
  ctx.manager_entity = &entity;

  ctx.initialize_singletons();

  ctx.register_battle_systems();

  return ctx;
}

void ServerContext::initialize_singletons() {
  make_combat_manager(*manager_entity);
  make_battle_processor_manager(*manager_entity);

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
      bool player_has_active = false;
      bool opponent_has_active = false;

      for (afterhours::Entity &entity :
           afterhours::EntityQuery()
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
  }
  return false;
}
} // namespace server
