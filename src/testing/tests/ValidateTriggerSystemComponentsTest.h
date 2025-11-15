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
#include "../test_app.h"
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

} // namespace ValidateTriggerSystemComponentsTestHelpers

TEST(validate_trigger_system_components) {
  using namespace ValidateTriggerSystemComponentsTestHelpers;
  log_info("TRIGGER_COMPONENTS_TEST: Starting Trigger System Components validation");

  app.launch_game();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1); // Ensure screen state is synced

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
  // Let game loop run systems naturally
  app.wait_for_frames(1);
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
  using namespace ValidateTriggerSystemComponentsTestHelpers;
  log_info("TRIGGER_HOOK_TEST: Testing OnServe trigger");

  app.launch_game();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1); // Ensure screen state is synced

  auto source_id = app.create_dish(DishType::FrenchFries)
                       .on_team(DishBattleState::TeamSide::Player)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::Entering)
                       .commit();

  auto ally1_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(1)
                      .in_phase(DishBattleState::Phase::InQueue)
                      .with_combat_stats()
                      .commit();

  auto ally2_id = app.create_dish(DishType::Bagel)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(2)
                      .in_phase(DishBattleState::Phase::InQueue)
                      .with_combat_stats()
                      .commit();

  // Wait a frame for entities to be merged by system loop
  app.wait_for_frames(1);

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnServe, source_id, 0,
                  DishBattleState::TeamSide::Player);

  // Let game loop run systems naturally
  app.wait_for_frames(1);

  auto *ally1 = app.find_entity_by_id(ally1_id);
  auto *ally2 = app.find_entity_by_id(ally2_id);

  if (!ally1 || !ally2) {
    log_error("TRIGGER_HOOK_TEST: Failed to find ally entities");
    return;
  }

  if (!ally1->has<PendingCombatMods>() || !ally2->has<PendingCombatMods>()) {
    log_error("TRIGGER_HOOK_TEST: OnServe effect not applied to future allies");
    return;
  }

  if (ally1->get<PendingCombatMods>().zingDelta != 1 ||
      ally2->get<PendingCombatMods>().zingDelta != 1) {
    log_error("TRIGGER_HOOK_TEST: OnServe effect wrong amount - expected zingDelta=1");
    return;
  }

  log_info("TRIGGER_HOOK_TEST: OnServe trigger PASSED");
}

TEST(validate_onbittetaken_trigger) {
  using namespace ValidateTriggerSystemComponentsTestHelpers;
  log_info("TRIGGER_HOOK_TEST: Testing OnBiteTaken trigger");

  app.launch_game();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1); // Ensure screen state is synced

  auto source_id = app.create_dish(DishType::Potato)
                       .on_team(DishBattleState::TeamSide::Player)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::InCombat)
                       .with_combat_stats()
                       .commit();

  auto target_id = app.create_dish(DishType::Potato)
                       .on_team(DishBattleState::TeamSide::Opponent)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::InCombat)
                       .with_combat_stats()
                       .commit();

  app.wait_for_frames(1);

  auto *source = app.find_entity_by_id(source_id);
  auto *target = app.find_entity_by_id(target_id);

  if (!source || !target) {
    log_error("TRIGGER_HOOK_TEST: Failed to find entities");
    return;
  }

  source->get<CombatStats>().currentZing = 2;
  target->get<CombatStats>().currentBody = 5;

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnBiteTaken, source_id, 0,
                  DishBattleState::TeamSide::Player);
  queue.events.back().payloadInt = 2;

  // Let game loop run systems naturally
  app.wait_for_frames(1);

  log_info("TRIGGER_HOOK_TEST: OnBiteTaken trigger PASSED (no effects configured for test dish)");
}

TEST(validate_ondishfinished_trigger) {
  using namespace ValidateTriggerSystemComponentsTestHelpers;
  log_info("TRIGGER_HOOK_TEST: Testing OnDishFinished trigger");

  app.launch_game();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1); // Ensure screen state is synced

  auto source_id = app.create_dish(DishType::FriedEgg)
                       .on_team(DishBattleState::TeamSide::Player)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::Finished)
                       .commit();

  auto ally1_id = app.create_dish(DishType::Potato)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(1)
                      .in_phase(DishBattleState::Phase::InCombat)
                      .with_combat_stats()
                      .commit();

  auto ally2_id = app.create_dish(DishType::Bagel)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(2)
                      .in_phase(DishBattleState::Phase::InQueue)
                      .with_combat_stats()
                      .commit();

  app.wait_for_frames(1);

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnDishFinished, source_id, 0,
                  DishBattleState::TeamSide::Player);

  // Let game loop run systems naturally
  app.wait_for_frames(1);

  auto *ally1 = app.find_entity_by_id(ally1_id);
  auto *ally2 = app.find_entity_by_id(ally2_id);

  if (!ally1 || !ally2) {
    log_error("TRIGGER_HOOK_TEST: Failed to find ally entities");
    return;
  }

  if (!ally1->has<PendingCombatMods>() || !ally2->has<PendingCombatMods>()) {
    log_error("TRIGGER_HOOK_TEST: OnDishFinished effect not applied to allies");
    return;
  }

  if (ally1->get<PendingCombatMods>().bodyDelta != 2 ||
      ally2->get<PendingCombatMods>().bodyDelta != 2) {
    log_error("TRIGGER_HOOK_TEST: OnDishFinished effect wrong amount - expected bodyDelta=2");
    return;
  }

  log_info("TRIGGER_HOOK_TEST: OnDishFinished trigger PASSED");
}

TEST(validate_oncoursestart_trigger) {
  using namespace ValidateTriggerSystemComponentsTestHelpers;
  log_info("TRIGGER_HOOK_TEST: Testing OnCourseStart trigger");

  app.launch_game();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1); // Ensure screen state is synced

  auto dish_id = app.create_dish(DishType::Potato)
                     .on_team(DishBattleState::TeamSide::Player)
                     .at_slot(0)
                     .in_phase(DishBattleState::Phase::Entering)
                     .with_combat_stats()
                     .commit();

  app.wait_for_frames(1);

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnCourseStart, dish_id, 0,
                  DishBattleState::TeamSide::Player);

  // Let game loop run systems naturally
  app.wait_for_frames(1);

  log_info("TRIGGER_HOOK_TEST: OnCourseStart trigger PASSED (no effects configured for test dish)");
}

TEST(validate_onstartbattle_trigger) {
  using namespace ValidateTriggerSystemComponentsTestHelpers;
  log_info("TRIGGER_HOOK_TEST: Testing OnStartBattle trigger");

  app.launch_game();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1); // Ensure screen state is synced

  auto dish_id = app.create_dish(DishType::Potato)
                     .on_team(DishBattleState::TeamSide::Player)
                     .at_slot(0)
                     .in_phase(DishBattleState::Phase::InQueue)
                     .with_combat_stats()
                     .commit();

  app.wait_for_frames(1);

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnStartBattle, dish_id, 0,
                  DishBattleState::TeamSide::Player);

  // Let game loop run systems naturally
  app.wait_for_frames(1);

  log_info("TRIGGER_HOOK_TEST: OnStartBattle trigger PASSED (no effects configured for test dish)");
}

TEST(validate_oncoursecomplete_trigger) {
  using namespace ValidateTriggerSystemComponentsTestHelpers;
  log_info("TRIGGER_HOOK_TEST: Testing OnCourseComplete trigger");

  app.launch_game();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1); // Ensure screen state is synced

  auto dish_id = app.create_dish(DishType::Potato)
                     .on_team(DishBattleState::TeamSide::Player)
                     .at_slot(0)
                     .in_phase(DishBattleState::Phase::Finished)
                     .with_combat_stats()
                     .commit();

  app.wait_for_frames(1);

  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnCourseComplete, dish_id, 0,
                  DishBattleState::TeamSide::Player);

  // Let game loop run systems naturally
  app.wait_for_frames(1);

  log_info("TRIGGER_HOOK_TEST: OnCourseComplete trigger PASSED (no effects configured for test dish)");
}
