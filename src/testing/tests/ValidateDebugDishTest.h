#pragma once

#include "../../components/combat_stats.h"
#include "../../components/deferred_flavor_mods.h"
#include "../../components/dish_battle_state.h"
#include "../../components/dish_effect.h"
#include "../../components/dish_level.h"
#include "../../components/is_dish.h"
#include "../../components/pending_combat_mods.h"
#include "../../components/pre_battle_modifiers.h"
#include "../../components/trigger_event.h"
#include "../../components/trigger_queue.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../systems/ApplyPairingsAndClashesSystem.h"
#include "../../systems/ApplyPendingCombatModsSystem.h"
#include "../../systems/ComputeCombatStatsSystem.h"
#include "../../systems/EffectResolutionSystem.h"
#include "../../systems/TriggerDispatchSystem.h"
#include "../test_macros.h"

namespace ValidateDebugDishTestHelpers {

// Helper to navigate to battle screen using TestApp navigation
static void navigate_to_battle_screen(TestApp &app) {
  // Check if we're already on Battle screen (from a previous test iteration)
  GameStateManager::get().update_screen();
  auto &gsm = GameStateManager::get();
  if (gsm.active_screen == GameStateManager::Screen::Battle) {
    app.wait_for_ui_exists("Skip to Results", 5.0f);
    return; // Already on battle screen
  }

  app.launch_game();
  app.wait_for_ui_exists("Play");
  app.click("Play");
  app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
  app.wait_for_ui_exists("Next Round");
  app.click("Next Round");
  app.wait_for_screen(GameStateManager::Screen::Battle, 15.0f);
  app.wait_for_ui_exists("Skip to Results", 5.0f);

  // Ensure screen is updated after navigation
  GameStateManager::get().update_screen();
}

static afterhours::Entity &
add_dish_to_menu(DishType type, DishBattleState::TeamSide team_side,
                 int queue_index = -1,
                 DishBattleState::Phase phase = DishBattleState::Phase::InQueue,
                 bool has_combat_stats = false) {
  auto &dish = afterhours::EntityHelper::createEntity();
  dish.addComponent<IsDish>(type);
  dish.addComponent<DishLevel>(1);

  auto &dbs = dish.addComponent<DishBattleState>();
  dbs.queue_index = (queue_index >= 0) ? queue_index : 0;
  dbs.team_side = team_side;
  dbs.phase = phase;

  if (has_combat_stats) {
    dish.addComponent<CombatStats>();
  }

  return dish;
}

static afterhours::Entity &get_or_create_trigger_queue() {
  afterhours::RefEntity tq_ref =
      afterhours::EntityHelper::get_singleton<TriggerQueue>();
  afterhours::Entity &tq_entity = tq_ref.get();
  if (!tq_entity.has<TriggerQueue>()) {
    tq_entity.addComponent<TriggerQueue>();
    afterhours::EntityHelper::registerSingleton<TriggerQueue>(tq_entity);
  }
  return tq_entity;
}

} // namespace ValidateDebugDishTestHelpers

// Verifies DebugDish can be instantiated with correct name, tier, and effect
// count. Ensures the dish definition is properly structured before testing
// individual effects.
TEST(validate_debug_dish_creation) {
  using namespace ValidateDebugDishTestHelpers;

  app.launch_game();

  log_info("DEBUG_DISH_TEST: Testing DebugDish creation and basic properties");

  auto info = get_dish_info(DishType::DebugDish);
  if (info.name != "Debug Dish") {
    log_error("DEBUG_DISH_TEST: Wrong name: {}", info.name);
    return;
  }

  if (info.tier != 1) {
    log_error("DEBUG_DISH_TEST: Wrong tier: {}", info.tier);
    return;
  }

  if (info.effects.empty()) {
    log_error("DEBUG_DISH_TEST: No effects found");
    return;
  }

  log_info("DEBUG_DISH_TEST: DebugDish created successfully with {} effects",
           info.effects.size());
  log_info("DEBUG_DISH_TEST: DebugDish creation PASSED");
}

// Tests that OnServe correctly applies all 7 flavor stat types (Satiety,
// Sweetness, etc.) to Self. Validates AddFlavorStat operation works for every
// FlavorStatType enum value.
TEST(validate_debug_dish_onserve_flavor_stats) {
  using namespace ValidateDebugDishTestHelpers;

  navigate_to_battle_screen(app);

  log_info("DEBUG_DISH_TEST: Testing DebugDish OnServe flavor stat effects");

  auto &debug_dish =
      add_dish_to_menu(DishType::DebugDish, DishBattleState::TeamSide::Player,
                       0, DishBattleState::Phase::Entering);

  afterhours::EntityHelper::merge_entity_arrays();

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnServe, debug_dish.id, 0,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  if (!debug_dish.has<DeferredFlavorMods>()) {
    log_error("DEBUG_DISH_TEST: Missing DeferredFlavorMods");
    return;
  }

  auto &mods = debug_dish.get<DeferredFlavorMods>();
  bool all_stats_present = mods.satiety == 1 && mods.sweetness == 1 &&
                           mods.spice == 1 && mods.acidity == 1 &&
                           mods.umami == 1 && mods.richness == 1 &&
                           mods.freshness == 1;

  if (!all_stats_present) {
    log_error(
        "DEBUG_DISH_TEST: Flavor stats mismatch - satiety={}, sweetness={}, "
        "spice={}, acidity={}, umami={}, richness={}, freshness={}",
        mods.satiety, mods.sweetness, mods.spice, mods.acidity, mods.umami,
        mods.richness, mods.freshness);
    return;
  }

  log_info("DEBUG_DISH_TEST: OnServe flavor stats PASSED");
}

// Validates that different target scopes (Opponent, FutureAllies,
// DishesAfterSelf) correctly identify targets. Ensures the targeting system
// can distinguish between various positional relationships.
TEST(validate_debug_dish_onserve_target_scopes) {
  using namespace ValidateDebugDishTestHelpers;

  navigate_to_battle_screen(app);

  log_info("DEBUG_DISH_TEST: Testing DebugDish OnServe target scopes");

  auto &debug_dish =
      add_dish_to_menu(DishType::DebugDish, DishBattleState::TeamSide::Player,
                       1, DishBattleState::Phase::Entering);

  auto &ally_after =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 2,
                       DishBattleState::Phase::InQueue, true);
  auto &future_ally =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 3,
                       DishBattleState::Phase::InQueue, true);
  auto &opponent =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Opponent, 1,
                       DishBattleState::Phase::InCombat, true);

  afterhours::EntityHelper::merge_entity_arrays();

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnServe, debug_dish.id, 1,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  bool valid = true;

  if (!future_ally.has<PendingCombatMods>()) {
    log_error("DEBUG_DISH_TEST: FutureAllies target missing PendingCombatMods");
    valid = false;
  }

  if (!ally_after.has<PendingCombatMods>()) {
    log_error(
        "DEBUG_DISH_TEST: DishesAfterSelf target missing PendingCombatMods");
    valid = false;
  }

  if (!opponent.has<PendingCombatMods>()) {
    log_error("DEBUG_DISH_TEST: Opponent target missing PendingCombatMods");
    valid = false;
  }

  if (valid) {
    log_info("DEBUG_DISH_TEST: OnServe target scopes PASSED");
  } else {
    log_error("DEBUG_DISH_TEST: OnServe target scopes FAILED");
  }
}

// Tests OnServe effects applying AddCombatZing and AddCombatBody to different
// targets. Verifies combat stat modifications (zing/body deltas) are
// correctly applied via PendingCombatMods.
TEST(validate_debug_dish_onserve_combat_mods) {
  using namespace ValidateDebugDishTestHelpers;

  navigate_to_battle_screen(app);

  log_info("DEBUG_DISH_TEST: Testing DebugDish OnServe combat mods");

  auto &debug_dish =
      add_dish_to_menu(DishType::DebugDish, DishBattleState::TeamSide::Player,
                       0, DishBattleState::Phase::Entering, true);
  auto &ally =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::InQueue, true);

  afterhours::EntityHelper::merge_entity_arrays();

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnServe, debug_dish.id, 0,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  if (!debug_dish.has<PendingCombatMods>()) {
    log_error("DEBUG_DISH_TEST: Self missing PendingCombatMods");
    return;
  }

  auto &self_mods = debug_dish.get<PendingCombatMods>();
  if (self_mods.bodyDelta != 2) {
    log_error("DEBUG_DISH_TEST: Self bodyDelta wrong: {}", self_mods.bodyDelta);
    return;
  }

  if (!ally.has<PendingCombatMods>()) {
    log_error("DEBUG_DISH_TEST: Ally missing PendingCombatMods");
    return;
  }

  auto &ally_mods = ally.get<PendingCombatMods>();
  // DebugDish has multiple overlapping effects (AllAllies, FutureAllies,
  // etc.) so values will be higher than individual effect amounts
  if (ally_mods.zingDelta <= 0 || ally_mods.bodyDelta <= 0) {
    log_error("DEBUG_DISH_TEST: Ally mods wrong - zing={}, body={} (expected "
              "positive values)",
              ally_mods.zingDelta, ally_mods.bodyDelta);
    return;
  }

  log_info(
      "DEBUG_DISH_TEST: OnServe combat mods PASSED (ally zing={}, body={})",
      ally_mods.zingDelta, ally_mods.bodyDelta);
}

// Verifies effects trigger at battle start (OnStartBattle hook).
// Tests that battle-initialization effects apply correctly before combat
// begins.
TEST(validate_debug_dish_onstartbattle) {
  using namespace ValidateDebugDishTestHelpers;

  navigate_to_battle_screen(app);

  log_info("DEBUG_DISH_TEST: Testing DebugDish OnStartBattle effects");

  auto &debug_dish =
      add_dish_to_menu(DishType::DebugDish, DishBattleState::TeamSide::Player,
                       0, DishBattleState::Phase::InQueue, true);
  auto &ally =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::InQueue, true);

  afterhours::EntityHelper::merge_entity_arrays();

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnStartBattle, debug_dish.id, 0,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  if (!debug_dish.has<PendingCombatMods>()) {
    log_error("DEBUG_DISH_TEST: OnStartBattle Self missing PendingCombatMods");
    return;
  }

  if (!ally.has<PendingCombatMods>()) {
    log_error("DEBUG_DISH_TEST: OnStartBattle Ally missing PendingCombatMods");
    return;
  }

  auto &ally_mods = ally.get<PendingCombatMods>();
  if (ally_mods.bodyDelta != 1) {
    log_error("DEBUG_DISH_TEST: OnStartBattle Ally bodyDelta wrong: {}",
              ally_mods.bodyDelta);
    return;
  }

  log_info("DEBUG_DISH_TEST: OnStartBattle PASSED");
}

// Tests effects that trigger when a course begins (OnCourseStart hook).
// Ensures course-start effects fire at the correct timing during battle
// progression.
TEST(validate_debug_dish_oncoursestart) {
  using namespace ValidateDebugDishTestHelpers;

  navigate_to_battle_screen(app);

  log_info("DEBUG_DISH_TEST: Testing DebugDish OnCourseStart effects");

  auto &debug_dish =
      add_dish_to_menu(DishType::DebugDish, DishBattleState::TeamSide::Player,
                       0, DishBattleState::Phase::InCombat, true);
  auto &ally =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::InCombat, true);

  afterhours::EntityHelper::merge_entity_arrays();

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnCourseStart, debug_dish.id, 0,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  if (!debug_dish.has<PendingCombatMods>()) {
    log_error("DEBUG_DISH_TEST: OnCourseStart Self missing PendingCombatMods");
    return;
  }

  if (!ally.has<PendingCombatMods>()) {
    log_error("DEBUG_DISH_TEST: OnCourseStart Ally missing PendingCombatMods");
    return;
  }

  log_info("DEBUG_DISH_TEST: OnCourseStart PASSED");
}

// Validates effects that trigger when damage is dealt (OnBiteTaken hook).
// Tests reactive effects that activate during active combat exchanges.
TEST(validate_debug_dish_onbitetaken) {
  using namespace ValidateDebugDishTestHelpers;

  navigate_to_battle_screen(app);

  log_info("DEBUG_DISH_TEST: Testing DebugDish OnBiteTaken effects");

  auto &debug_dish =
      add_dish_to_menu(DishType::DebugDish, DishBattleState::TeamSide::Player,
                       0, DishBattleState::Phase::InCombat, true);
  auto &ally =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::InCombat, true);

  afterhours::EntityHelper::merge_entity_arrays();

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnBiteTaken, debug_dish.id, 0,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  if (!debug_dish.has<PendingCombatMods>()) {
    log_error("DEBUG_DISH_TEST: OnBiteTaken Self missing PendingCombatMods");
    return;
  }

  if (!ally.has<PendingCombatMods>()) {
    log_error("DEBUG_DISH_TEST: OnBiteTaken Ally missing PendingCombatMods");
    return;
  }

  log_info("DEBUG_DISH_TEST: OnBiteTaken PASSED");
}

// Tests effects that trigger when a dish is defeated (OnDishFinished hook).
// Verifies "death effects" that provide bonuses to allies when the source
// dish finishes.
TEST(validate_debug_dish_ondishfinished) {
  using namespace ValidateDebugDishTestHelpers;

  navigate_to_battle_screen(app);

  log_info("DEBUG_DISH_TEST: Testing DebugDish OnDishFinished effects");

  auto &debug_dish =
      add_dish_to_menu(DishType::DebugDish, DishBattleState::TeamSide::Player,
                       0, DishBattleState::Phase::Finished, true);
  auto &ally =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::InCombat, true);

  afterhours::EntityHelper::merge_entity_arrays();

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnDishFinished, debug_dish.id, 0,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  if (!ally.has<PendingCombatMods>()) {
    log_error("DEBUG_DISH_TEST: OnDishFinished Ally missing PendingCombatMods");
    return;
  }

  auto &ally_mods = ally.get<PendingCombatMods>();
  if (ally_mods.zingDelta != 1 || ally_mods.bodyDelta != 2) {
    log_error(
        "DEBUG_DISH_TEST: OnDishFinished Ally mods wrong - zing={}, body={}",
        ally_mods.zingDelta, ally_mods.bodyDelta);
    return;
  }

  log_info("DEBUG_DISH_TEST: OnDishFinished PASSED");
}

// Validates effects that trigger when a course ends (OnCourseComplete hook).
// Tests effects that fire after both dishes in a course are finished.
TEST(validate_debug_dish_oncoursecomplete) {
  using namespace ValidateDebugDishTestHelpers;

  navigate_to_battle_screen(app);

  log_info("DEBUG_DISH_TEST: Testing DebugDish OnCourseComplete effects");

  auto &debug_dish =
      add_dish_to_menu(DishType::DebugDish, DishBattleState::TeamSide::Player,
                       0, DishBattleState::Phase::Finished, true);
  auto &ally =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::InCombat, true);

  afterhours::EntityHelper::merge_entity_arrays();

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnCourseComplete, debug_dish.id, 0,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  if (!ally.has<PendingCombatMods>()) {
    log_error(
        "DEBUG_DISH_TEST: OnCourseComplete Ally missing PendingCombatMods");
    return;
  }

  auto &ally_mods = ally.get<PendingCombatMods>();
  if (ally_mods.zingDelta != 1 || ally_mods.bodyDelta != 1) {
    log_error("DEBUG_DISH_TEST: OnCourseComplete Ally mods wrong - zing={}, "
              "body={}",
              ally_mods.zingDelta, ally_mods.bodyDelta);
    return;
  }

  log_info("DEBUG_DISH_TEST: OnCourseComplete PASSED");
}
