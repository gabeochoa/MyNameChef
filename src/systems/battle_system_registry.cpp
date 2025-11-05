#include "battle_system_registry.h"
#include "BattleTeamFileLoaderSystem.h"
#include "InstantiateBattleTeamSystem.h"
#include "TagTeamEntitiesSystem.h"
#include "BattleTeamLoaderSystem.h"
#include "BattleDebugSystem.h"
#include "BattleProcessorSystem.h"
#include "ComputeCombatStatsSystem.h"
#include "TriggerDispatchSystem.h"
#include "EffectResolutionSystem.h"
#include "ApplyPendingCombatModsSystem.h"
#include "InitCombatState.h"
#include "ApplyPairingsAndClashesSystem.h"
#include "StartCourseSystem.h"
#include "BattleEnterAnimationSystem.h"
#include "SimplifiedOnServeSystem.h"
#include "ResolveCombatTickSystem.h"
#include "AdvanceCourseSystem.h"
#include "ReplayControllerSystem.h"
#include "AuditSystem.h"
#include "CleanupBattleEntities.h"
#include "CleanupShopEntities.h"
#include "CleanupDishesEntities.h"
#include "GenerateDishesGallery.h"
#include "LoadBattleResults.h"
#include "ProcessBattleRewards.h"
#include "InitialShopFill.h"
#include "UnifiedAnimationSystem.h"
#include "../render_backend.h"
#include <afterhours/src/plugins/animation.h>
#include <afterhours/src/plugins/texture_manager.h>

namespace battle_systems {
  void register_battle_systems(afterhours::SystemManager &systems) {
    systems.register_update_system(std::make_unique<BattleTeamFileLoaderSystem>());
    systems.register_update_system(std::make_unique<InstantiateBattleTeamSystem>());
    systems.register_update_system(std::make_unique<TagTeamEntitiesSystem>());
    systems.register_update_system(std::make_unique<BattleTeamLoaderSystem>());
    systems.register_update_system(std::make_unique<BattleDebugSystem>());
    systems.register_update_system(std::make_unique<BattleProcessorSystem>());
    systems.register_update_system(std::make_unique<ComputeCombatStatsSystem>());
    systems.register_update_system(std::make_unique<TriggerDispatchSystem>());
    systems.register_update_system(std::make_unique<EffectResolutionSystem>());
    systems.register_update_system(std::make_unique<ApplyPendingCombatModsSystem>());
    systems.register_update_system(std::make_unique<InitCombatState>());
    systems.register_update_system(std::make_unique<ApplyPairingsAndClashesSystem>());
    systems.register_update_system(std::make_unique<StartCourseSystem>());
    systems.register_update_system(std::make_unique<BattleEnterAnimationSystem>());
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
    systems.register_update_system(std::make_unique<SlideInAnimationDriverSystem>());
    
    systems.register_update_system(std::make_unique<LoadBattleResults>());
    systems.register_update_system(std::make_unique<ProcessBattleRewards>());
    systems.register_update_system(std::make_unique<InitialShopFill>());
  }
}

