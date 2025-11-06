#include "battle_system_registry.h"
#include "../render_backend.h"
#include "AdvanceCourseSystem.h"
#include "ApplyPairingsAndClashesSystem.h"
#include "ApplyPendingCombatModsSystem.h"
#include "AuditSystem.h"
#include "BattleDebugSystem.h"
#include "BattleEnterAnimationSystem.h"
#include "BattleProcessorSystem.h"
#include "BattleTeamFileLoaderSystem.h"
#include "BattleTeamLoaderSystem.h"
#include "CleanupBattleEntities.h"
#include "CleanupDishesEntities.h"
#include "CleanupShopEntities.h"
#include "ComputeCombatStatsSystem.h"
#include "EffectResolutionSystem.h"
#include "GenerateDishesGallery.h"
#include "InitCombatState.h"
#include "InitialShopFill.h"
#include "InstantiateBattleTeamSystem.h"
#include "LoadBattleResults.h"
#include "ProcessBattleRewards.h"
#include "ReplayControllerSystem.h"
#include "ResolveCombatTickSystem.h"
#include "ServerBattleRequestSystem.h"
#include "SimplifiedOnServeSystem.h"
#include "StartCourseSystem.h"
#include "TagTeamEntitiesSystem.h"
#include "TriggerDispatchSystem.h"
#include "UnifiedAnimationSystem.h"
#include <afterhours/src/plugins/animation.h>
#include <afterhours/src/plugins/texture_manager.h>

namespace battle_systems {
void register_battle_systems(afterhours::SystemManager &systems) {
  systems.register_update_system(std::make_unique<ServerBattleRequestSystem>());
  systems.register_update_system(
      std::make_unique<BattleTeamFileLoaderSystem>());
  systems.register_update_system(
      std::make_unique<InstantiateBattleTeamSystem>());
  systems.register_update_system(std::make_unique<TagTeamEntitiesSystem>());
  systems.register_update_system(std::make_unique<BattleTeamLoaderSystem>());
  systems.register_update_system(std::make_unique<BattleDebugSystem>());
  systems.register_update_system(std::make_unique<BattleProcessorSystem>());
  systems.register_update_system(std::make_unique<ComputeCombatStatsSystem>());
  systems.register_update_system(std::make_unique<TriggerDispatchSystem>());
  systems.register_update_system(std::make_unique<EffectResolutionSystem>());
  systems.register_update_system(
      std::make_unique<ApplyPendingCombatModsSystem>());
  systems.register_update_system(std::make_unique<InitCombatState>());
  systems.register_update_system(
      std::make_unique<ApplyPairingsAndClashesSystem>());
  systems.register_update_system(std::make_unique<StartCourseSystem>());
  systems.register_update_system(
      std::make_unique<BattleEnterAnimationSystem>());
  systems.register_update_system(std::make_unique<ComputeCombatStatsSystem>());
  systems.register_update_system(std::make_unique<SimplifiedOnServeSystem>());
  systems.register_update_system(std::make_unique<ResolveCombatTickSystem>());
  systems.register_update_system(std::make_unique<AdvanceCourseSystem>());
  systems.register_update_system(std::make_unique<ReplayControllerSystem>());
  systems.register_update_system(std::make_unique<AuditSystem>());
  systems.register_update_system(std::make_unique<CleanupBattleEntities>());
  systems.register_update_system(std::make_unique<CleanupShopEntities>());
  systems.register_update_system(std::make_unique<CleanupDishesEntities>());
  systems.register_update_system(std::make_unique<GenerateDishesGallery>());

  texture_manager::register_update_systems(systems);
  afterhours::animation::register_update_systems<
      afterhours::animation::CompositeKey>(systems);
  afterhours::animation::register_update_systems<BattleAnimKey>(systems);
  systems.register_update_system(std::make_unique<AnimationSchedulerSystem>());
  systems.register_update_system(std::make_unique<AnimationTimerSystem>());
  systems.register_update_system(
      std::make_unique<SlideInAnimationDriverSystem>());

  systems.register_update_system(std::make_unique<LoadBattleResults>());
  systems.register_update_system(std::make_unique<ProcessBattleRewards>());
  systems.register_update_system(std::make_unique<InitialShopFill>());
}
} // namespace battle_systems
