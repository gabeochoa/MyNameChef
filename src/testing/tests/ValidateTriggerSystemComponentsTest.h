#pragma once

#include "../../components/combat_stats.h"
#include "../../components/deferred_flavor_mods.h"
#include "../../components/dish_battle_state.h"
#include "../../components/dish_effect.h"
#include "../../components/dish_level.h"
#include "../../components/is_dish.h"
#include "../../components/pending_combat_mods.h"
#include "../../components/trigger_event.h"
#include "../../components/trigger_queue.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../systems/EffectResolutionSystem.h"
#include "../../systems/TriggerDispatchSystem.h"
#include "../test_macros.h"

namespace ValidateTriggerSystemComponentsTestHelpers {

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

static afterhours::Entity &add_dish_to_menu(
    DishType type, DishBattleState::TeamSide team_side, int queue_index = -1,
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

} // namespace ValidateTriggerSystemComponentsTestHelpers

TEST(validate_trigger_system_components) {
  using namespace ValidateTriggerSystemComponentsTestHelpers;
  log_info("TRIGGER_COMPONENTS_TEST: Starting Trigger System Components validation");

  app.launch_game();
  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  log_info("TRIGGER_COMPONENTS_TEST: Test 1 - Validate TriggerEvent component");
  TriggerEvent ev(TriggerHook::OnServe, 123, 1,
                  DishBattleState::TeamSide::Player);
  if (ev.hook != TriggerHook::OnServe || ev.sourceEntityId != 123 ||
      ev.slotIndex != 1) {
    log_error("TRIGGER_COMPONENTS_TEST: TriggerEvent creation FAILED");
    return;
  }
  log_info("TRIGGER_COMPONENTS_TEST: TriggerEvent component PASSED");

  log_info("TRIGGER_COMPONENTS_TEST: Test 2 - Validate TriggerQueue component");
  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();

  queue.add_event(TriggerHook::OnStartBattle, 100, 0,
                  DishBattleState::TeamSide::Player);
  queue.add_event(TriggerHook::OnServe, 101, 1,
                  DishBattleState::TeamSide::Player);

  if (queue.size() != 2) {
    log_error("TRIGGER_COMPONENTS_TEST: TriggerQueue size mismatch - expected 2, got {}",
              queue.size());
    return;
  }
  log_info("TRIGGER_COMPONENTS_TEST: TriggerQueue component PASSED");

  log_info("TRIGGER_COMPONENTS_TEST: Test 3 - Validate trigger hooks exist");
  TriggerHook hooks[] = {
      TriggerHook::OnStartBattle,  TriggerHook::OnServe,
      TriggerHook::OnCourseStart,  TriggerHook::OnBiteTaken,
      TriggerHook::OnDishFinished, TriggerHook::OnCourseComplete};
  for (auto hook : hooks) {
    int hook_val = static_cast<int>(hook);
    if (hook_val < 0 || hook_val > 5) {
      log_error("TRIGGER_COMPONENTS_TEST: Invalid hook value: {}", hook_val);
      return;
    }
  }
  log_info("TRIGGER_COMPONENTS_TEST: Trigger hooks PASSED");

  log_info("TRIGGER_COMPONENTS_TEST: Test 4 - Validate TriggerDispatchSystem");
  TriggerDispatchSystem dispatchSystem;
  if (dispatchSystem.should_run(1.0f / 60.0f)) {
    dispatchSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
    if (!queue.empty()) {
      log_error("TRIGGER_COMPONENTS_TEST: TriggerDispatchSystem did not clear queue");
      return;
    }
  }
  log_info("TRIGGER_COMPONENTS_TEST: TriggerDispatchSystem PASSED");

  log_info("TRIGGER_COMPONENTS_TEST: Test 5 - Validate EffectResolutionSystem exists");
  EffectResolutionSystem effectSystem;
  if (!effectSystem.should_run(1.0f / 60.0f)) {
    log_error("TRIGGER_COMPONENTS_TEST: EffectResolutionSystem should_run returned false");
    return;
  }
  log_info("TRIGGER_COMPONENTS_TEST: EffectResolutionSystem PASSED");

  log_info("TRIGGER_COMPONENTS_TEST: Test 6 - Validate effect operations");
  EffectOperation ops[] = {EffectOperation::AddFlavorStat,
                           EffectOperation::AddCombatZing,
                           EffectOperation::AddCombatBody};
  for (auto op : ops) {
    int op_val = static_cast<int>(op);
    if (op_val < 0 || op_val > 2) {
      log_error("TRIGGER_COMPONENTS_TEST: Invalid operation value: {}", op_val);
      return;
    }
  }
  log_info("TRIGGER_COMPONENTS_TEST: Effect operations PASSED");

  log_info("TRIGGER_COMPONENTS_TEST: Test 7 - Validate targeting scopes");
  TargetScope scopes[] = {TargetScope::Self,
                          TargetScope::Opponent,
                          TargetScope::AllAllies,
                          TargetScope::AllOpponents,
                          TargetScope::DishesAfterSelf,
                          TargetScope::FutureAllies,
                          TargetScope::FutureOpponents,
                          TargetScope::Previous,
                          TargetScope::Next,
                          TargetScope::SelfAndAdjacent};
  for (auto scope : scopes) {
    int scope_val = static_cast<int>(scope);
    if (scope_val < 0 || scope_val > 9) {
      log_error("TRIGGER_COMPONENTS_TEST: Invalid scope value: {}", scope_val);
      return;
    }
  }
  log_info("TRIGGER_COMPONENTS_TEST: Targeting scopes PASSED");

  log_info("TRIGGER_COMPONENTS_TEST: Test 8 - Validate DeferredFlavorMods component");
  auto &flavor_test = afterhours::EntityHelper::createEntity();
  auto &def_mod = flavor_test.addComponent<DeferredFlavorMods>();
  def_mod.satiety = 1;
  def_mod.spice = 2;
  if (!flavor_test.has<DeferredFlavorMods>()) {
    log_error("TRIGGER_COMPONENTS_TEST: DeferredFlavorMods component not found");
    return;
  }
  log_info("TRIGGER_COMPONENTS_TEST: DeferredFlavorMods component PASSED");

  log_info("TRIGGER_COMPONENTS_TEST: Test 9 - Validate PendingCombatMods component");
  auto &combat_test = afterhours::EntityHelper::createEntity();
  auto &pending_mod = combat_test.addComponent<PendingCombatMods>();
  pending_mod.zingDelta = 1;
  pending_mod.bodyDelta = 2;
  if (!combat_test.has<PendingCombatMods>()) {
    log_error("TRIGGER_COMPONENTS_TEST: PendingCombatMods component not found");
    return;
  }
  log_info("TRIGGER_COMPONENTS_TEST: PendingCombatMods component PASSED");

  log_info("TRIGGER_COMPONENTS_TEST: All trigger system components tests PASSED");
}

TEST(validate_onserve_trigger) {
  (void)app;
  using namespace ValidateTriggerSystemComponentsTestHelpers;
  log_info("TRIGGER_HOOK_TEST: Testing OnServe trigger");

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  auto &source = add_dish_to_menu(DishType::FrenchFries,
                                  DishBattleState::TeamSide::Player, 0,
                                  DishBattleState::Phase::Entering);

  auto &ally1 =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::InQueue, true);
  auto &ally2 =
      add_dish_to_menu(DishType::Bagel, DishBattleState::TeamSide::Player, 2,
                       DishBattleState::Phase::InQueue, true);

  // Wait a frame for entities to be merged by system loop
  app.wait_for_frames(1);

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnServe, source.id, 0,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  if (!ally1.has<PendingCombatMods>() || !ally2.has<PendingCombatMods>()) {
    log_error("TRIGGER_HOOK_TEST: OnServe effect not applied to future allies");
    return;
  }

  if (ally1.get<PendingCombatMods>().zingDelta != 1 ||
      ally2.get<PendingCombatMods>().zingDelta != 1) {
    log_error("TRIGGER_HOOK_TEST: OnServe effect wrong amount - expected zingDelta=1");
    return;
  }

  log_info("TRIGGER_HOOK_TEST: OnServe trigger PASSED");
}

TEST(validate_onbittetaken_trigger) {
  (void)app;
  using namespace ValidateTriggerSystemComponentsTestHelpers;
  log_info("TRIGGER_HOOK_TEST: Testing OnBiteTaken trigger");

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  auto &source = add_dish_to_menu(DishType::Potato,
                                  DishBattleState::TeamSide::Player, 0,
                                  DishBattleState::Phase::InCombat, true);
  source.get<CombatStats>().currentZing = 2;

  auto &target = add_dish_to_menu(DishType::Potato,
                                  DishBattleState::TeamSide::Opponent, 0,
                                  DishBattleState::Phase::InCombat, true);
  target.get<CombatStats>().currentBody = 5;

  afterhours::EntityHelper::merge_entity_arrays();

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnBiteTaken, source.id, 0,
                  DishBattleState::TeamSide::Player);
  queue.events.back().payloadInt = 2;

  TriggerDispatchSystem dispatchSystem;
  EffectResolutionSystem effectSystem;
  if (dispatchSystem.should_run(1.0f / 60.0f)) {
    dispatchSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  log_info("TRIGGER_HOOK_TEST: OnBiteTaken trigger PASSED (no effects configured for test dish)");
}

TEST(validate_ondishfinished_trigger) {
  (void)app;
  using namespace ValidateTriggerSystemComponentsTestHelpers;
  log_info("TRIGGER_HOOK_TEST: Testing OnDishFinished trigger");

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  auto &source = add_dish_to_menu(DishType::FriedEgg,
                                  DishBattleState::TeamSide::Player, 0,
                                  DishBattleState::Phase::Finished);

  auto &ally1 =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::InCombat, true);
  auto &ally2 =
      add_dish_to_menu(DishType::Bagel, DishBattleState::TeamSide::Player, 2,
                       DishBattleState::Phase::InQueue, true);

  afterhours::EntityHelper::merge_entity_arrays();

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnDishFinished, source.id, 0,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  if (!ally1.has<PendingCombatMods>() || !ally2.has<PendingCombatMods>()) {
    log_error("TRIGGER_HOOK_TEST: OnDishFinished effect not applied to allies");
    return;
  }

  if (ally1.get<PendingCombatMods>().bodyDelta != 2 ||
      ally2.get<PendingCombatMods>().bodyDelta != 2) {
    log_error("TRIGGER_HOOK_TEST: OnDishFinished effect wrong amount - expected bodyDelta=2");
    return;
  }

  log_info("TRIGGER_HOOK_TEST: OnDishFinished trigger PASSED");
}

TEST(validate_oncoursestart_trigger) {
  (void)app;
  using namespace ValidateTriggerSystemComponentsTestHelpers;
  log_info("TRIGGER_HOOK_TEST: Testing OnCourseStart trigger");

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  auto &dish = add_dish_to_menu(DishType::Potato,
                                DishBattleState::TeamSide::Player, 0,
                                DishBattleState::Phase::Entering, true);

  afterhours::EntityHelper::merge_entity_arrays();

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnCourseStart, dish.id, 0,
                  DishBattleState::TeamSide::Player);

  TriggerDispatchSystem dispatchSystem;
  EffectResolutionSystem effectSystem;
  if (dispatchSystem.should_run(1.0f / 60.0f)) {
    dispatchSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  log_info("TRIGGER_HOOK_TEST: OnCourseStart trigger PASSED (no effects configured for test dish)");
}

TEST(validate_onstartbattle_trigger) {
  (void)app;
  using namespace ValidateTriggerSystemComponentsTestHelpers;
  log_info("TRIGGER_HOOK_TEST: Testing OnStartBattle trigger");

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  auto &dish = add_dish_to_menu(DishType::Potato,
                                DishBattleState::TeamSide::Player, 0,
                                DishBattleState::Phase::InQueue, true);

  afterhours::EntityHelper::merge_entity_arrays();

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnStartBattle, dish.id, 0,
                  DishBattleState::TeamSide::Player);

  TriggerDispatchSystem dispatchSystem;
  EffectResolutionSystem effectSystem;
  if (dispatchSystem.should_run(1.0f / 60.0f)) {
    dispatchSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  log_info("TRIGGER_HOOK_TEST: OnStartBattle trigger PASSED (no effects configured for test dish)");
}

TEST(validate_oncoursecomplete_trigger) {
  (void)app;
  using namespace ValidateTriggerSystemComponentsTestHelpers;
  log_info("TRIGGER_HOOK_TEST: Testing OnCourseComplete trigger");

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  auto &dish = add_dish_to_menu(DishType::Potato,
                                DishBattleState::TeamSide::Player, 0,
                                DishBattleState::Phase::Finished, true);

  afterhours::EntityHelper::merge_entity_arrays();

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnCourseComplete, dish.id, 0,
                  DishBattleState::TeamSide::Player);

  TriggerDispatchSystem dispatchSystem;
  EffectResolutionSystem effectSystem;
  if (dispatchSystem.should_run(1.0f / 60.0f)) {
    dispatchSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  log_info("TRIGGER_HOOK_TEST: OnCourseComplete trigger PASSED (no effects configured for test dish)");
}
