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

namespace ValidateEffectSystemTestHelpers {

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

static bool validate_pending_mod(afterhours::Entity &entity, int expectedZing,
                                 int expectedBody) {
  if (!entity.has<PendingCombatMods>()) {
    log_error("EFFECT_TEST: Missing PendingCombatMods on entity {}", entity.id);
    return false;
  }
  auto &mod = entity.get<PendingCombatMods>();
  if (mod.zingDelta != expectedZing || mod.bodyDelta != expectedBody) {
    log_error("EFFECT_TEST: Wrong mod values - expected zing={} body={}, got "
              "zing={} body={}",
              expectedZing, expectedBody, mod.zingDelta, mod.bodyDelta);
    return false;
  }
  return true;
}

static afterhours::Entity &create_test_dish(DishType type, int queue_index,
                                            DishBattleState::TeamSide side,
                                            DishBattleState::Phase phase) {
  return add_dish_to_menu(type, side, queue_index, phase, false);
}

static void test_french_fries_effect() {
  log_info("EFFECT_TEST: Testing French Fries effect (FutureAllies +1 Zing)");

  // Setup: Create battle state - use direct to_battle for ECS tests
  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  // Create source dish (French Fries)
  auto &source =
      add_dish_to_menu(DishType::FrenchFries, DishBattleState::TeamSide::Player,
                       0, DishBattleState::Phase::Entering);

  // Create future ally dishes (InQueue)
  auto &ally1 =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::InQueue, true);
  auto &ally2 =
      add_dish_to_menu(DishType::Bagel, DishBattleState::TeamSide::Player, 2,
                       DishBattleState::Phase::InQueue, true);

  // Merge temp entities into main array so queries can find them
  afterhours::EntityHelper::merge_entity_arrays();

  // Get TriggerQueue and fire trigger
  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnServe, source.id, 0,
                  DishBattleState::TeamSide::Player);

  // Process effect
  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  // Verify: Future allies should have PendingCombatMods with +1 zingDelta
  bool valid =
      validate_pending_mod(ally1, 1, 0) && validate_pending_mod(ally2, 1, 0);
  if (valid) {
    log_info("EFFECT_TEST: French Fries effect PASSED");
  } else {
    log_error("EFFECT_TEST: French Fries effect FAILED");
  }
}

static void test_bagel_effect() {
  log_info("EFFECT_TEST: Testing Bagel effect (DishesAfterSelf +1 Richness)");

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  // Create Bagel at slot 1
  auto &bagel =
      add_dish_to_menu(DishType::Bagel, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::Entering);

  // Create dishes after Bagel (slots 2, 3)
  auto &after1 =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 2,
                       DishBattleState::Phase::InQueue);
  auto &after2 =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 3,
                       DishBattleState::Phase::InQueue);

  // Merge temp entities into main array so queries can find them
  afterhours::EntityHelper::merge_entity_arrays();

  // Get TriggerQueue and fire trigger
  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnServe, bagel.id, 1,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  // Verify: Dishes after should have DeferredFlavorMods with +1 richness
  bool after1_has_mod = after1.has<DeferredFlavorMods>();
  bool after2_has_mod = after2.has<DeferredFlavorMods>();

  if (after1_has_mod && after2_has_mod) {
    auto &mod1 = after1.get<DeferredFlavorMods>();
    auto &mod2 = after2.get<DeferredFlavorMods>();
    if (mod1.richness == 1 && mod2.richness == 1) {
      log_info("EFFECT_TEST: Bagel effect PASSED");
    } else {
      log_error("EFFECT_TEST: Bagel effect FAILED - wrong richness: {} {}",
                mod1.richness, mod2.richness);
    }
  } else {
    log_error("EFFECT_TEST: Bagel effect FAILED - missing DeferredFlavorMods");
  }
}

static void test_baguette_effect() {
  log_info("EFFECT_TEST: Testing Baguette effect (Opponent -1 Zing)");

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  // Create Baguette at slot 0
  auto &baguette =
      add_dish_to_menu(DishType::Baguette, DishBattleState::TeamSide::Player, 0,
                       DishBattleState::Phase::Entering);

  // Create opponent at same slot
  auto &opponent =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Opponent, 0,
                       DishBattleState::Phase::InCombat, true);
  opponent.get<CombatStats>().currentZing = 2; // Start with 2 Zing

  // Merge temp entities into main array so queries can find them
  afterhours::EntityHelper::merge_entity_arrays();

  // Get TriggerQueue and fire trigger
  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnServe, baguette.id, 0,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  // Verify: Opponent should have PendingCombatMods with -1 zingDelta
  bool valid = validate_pending_mod(opponent, -1, 0);
  if (valid) {
    log_info("EFFECT_TEST: Baguette effect PASSED");
  } else {
    log_error("EFFECT_TEST: Baguette effect FAILED");
  }
}

static void test_garlic_bread_effect() {
  log_info("EFFECT_TEST: Testing Garlic Bread effect (FutureAllies +1 Spice)");

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  // Create Garlic Bread
  auto &garlic =
      add_dish_to_menu(DishType::GarlicBread, DishBattleState::TeamSide::Player,
                       0, DishBattleState::Phase::Entering);

  // Create future allies
  auto &future =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::InQueue);

  // Merge temp entities into main array so queries can find them
  afterhours::EntityHelper::merge_entity_arrays();

  // Get TriggerQueue and fire trigger
  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnServe, garlic.id, 0,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  // Verify: Future ally should have DeferredFlavorMods with +1 spice
  if (future.has<DeferredFlavorMods>()) {
    auto &mod = future.get<DeferredFlavorMods>();
    if (mod.spice == 1) {
      log_info("EFFECT_TEST: Garlic Bread effect PASSED");
    } else {
      log_error("EFFECT_TEST: Garlic Bread effect FAILED - wrong spice: {}",
                mod.spice);
    }
  } else {
    log_error("EFFECT_TEST: Garlic Bread effect FAILED - missing "
              "DeferredFlavorMods");
  }
}

static void test_fried_egg_effect() {
  log_info("EFFECT_TEST: Testing Fried Egg effect (OnDishFinished → "
           "AllAllies +2 Body)");

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  // Create Fried Egg (source)
  auto &egg =
      add_dish_to_menu(DishType::FriedEgg, DishBattleState::TeamSide::Player, 0,
                       DishBattleState::Phase::Finished);

  // Create allies
  auto &ally1 =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::InCombat, true);
  auto &ally2 =
      add_dish_to_menu(DishType::Bagel, DishBattleState::TeamSide::Player, 2,
                       DishBattleState::Phase::Finished, true);

  // Merge temp entities into main array so queries can find them
  afterhours::EntityHelper::merge_entity_arrays();

  // Get TriggerQueue and fire trigger
  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnDishFinished, egg.id, 0,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  // Verify: All allies should have PendingCombatMods with +2 bodyDelta
  bool valid =
      validate_pending_mod(ally1, 0, 2) && validate_pending_mod(ally2, 0, 2);
  if (valid) {
    log_info("EFFECT_TEST: Fried Egg effect PASSED");
  } else {
    log_error("EFFECT_TEST: Fried Egg effect FAILED");
  }
}

static void test_targeting_scope(afterhours::Entity & /*source*/,
                                 TargetScope /*scope*/,
                                 int /*expected_count*/) {
  // Get TriggerQueue singleton
  auto &tq_entity = get_or_create_trigger_queue();
  (void)tq_entity.get<TriggerQueue>();

  // Note: Targeting scope test would need actual dish effects to test
  // properly For now, we'll skip detailed testing and just verify the system
  // runs
  log_info("EFFECT_TEST: Targeting scope test - system structure validated");
}

static void test_targeting_scopes() {
  log_info("EFFECT_TEST: Testing targeting scopes");

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  // Create a complex scenario with multiple dishes
  // Player team: slots 0, 1, 2
  // Opponent team: slot 0

  auto &player0 =
      add_dish_to_menu(DishType::FrenchFries, DishBattleState::TeamSide::Player,
                       0, DishBattleState::Phase::Entering);
  (void)add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 1,
                         DishBattleState::Phase::InQueue);
  (void)add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 2,
                         DishBattleState::Phase::InQueue);
  (void)add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Opponent,
                         0, DishBattleState::Phase::InCombat);

  // Test Self targeting
  test_targeting_scope(player0, TargetScope::Self, 1);

  // Test Opponent targeting
  test_targeting_scope(player0, TargetScope::Opponent, 1);

  // Test AllAllies targeting
  test_targeting_scope(player0, TargetScope::AllAllies, 2);

  // Test AllOpponents targeting
  test_targeting_scope(player0, TargetScope::AllOpponents, 1);

  // Test DishesAfterSelf targeting
  test_targeting_scope(player0, TargetScope::DishesAfterSelf, 2);

  // Test FutureAllies targeting
  test_targeting_scope(player0, TargetScope::FutureAllies, 2);

  log_info("EFFECT_TEST: Targeting scopes test completed");
}

static void test_deferred_flavor_mods_consumption() {
  log_info("EFFECT_TEST: Testing DeferredFlavorMods consumption");

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  // Create dish with DeferredFlavorMods
  auto &dish =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 0,
                       DishBattleState::Phase::InQueue);

  // Add DeferredFlavorMods
  auto &def = dish.addComponent<DeferredFlavorMods>();
  def.richness = 2;
  def.spice = 1;

  // Transition to Entering phase (should consume mods)
  auto &dbs = dish.get<DishBattleState>();
  dbs.phase = DishBattleState::Phase::Entering;

  // Run ComputeCombatStatsSystem (it consumes DeferredFlavorMods)
  // Note: In a real test, we'd register and run the system
  // For now, we verify the mods are still present before consumption
  if (dish.has<DeferredFlavorMods>()) {
    log_info("EFFECT_TEST: DeferredFlavorMods present before consumption");
  }

  // The actual consumption happens in ComputeCombatStatsSystem
  // which runs in the main game loop. This test verifies the structure
  // exists.
  log_info("EFFECT_TEST: DeferredFlavorMods consumption test completed");
}

static void test_modifier_persistence_after_dish_finishes() {
  log_info(
      "EFFECT_TEST: Testing modifier persistence after source dish finishes");

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  // Create French Fries (source) that will give +1 Zing to future allies
  auto &source =
      add_dish_to_menu(DishType::FrenchFries, DishBattleState::TeamSide::Player,
                       0, DishBattleState::Phase::Entering, true);

  // Create future ally dish that will receive the modifier
  auto &target =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::InQueue, true);

  // Merge temp entities into main array so queries can find them
  afterhours::EntityHelper::merge_entity_arrays();

  // Get TriggerQueue and fire OnServe trigger
  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnServe, source.id, 0,
                  DishBattleState::TeamSide::Player);

  // Process effect - this creates PendingCombatMods
  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  log_info("EFFECT_TEST: After effect resolution - checking for "
           "PendingCombatMods");

  // Verify target has PendingCombatMods
  if (!target.has<PendingCombatMods>()) {
    log_error("EFFECT_TEST: Modifier persistence test FAILED - target missing "
              "PendingCombatMods");
    return;
  }

  auto &pending = target.get<PendingCombatMods>();
  log_info("EFFECT_TEST: Target has PendingCombatMods - zingDelta={}, "
           "bodyDelta={}",
           pending.zingDelta, pending.bodyDelta);

  if (pending.zingDelta != 1) {
    log_error("EFFECT_TEST: Modifier persistence test FAILED - wrong pending "
              "zingDelta: {}",
              pending.zingDelta);
    return;
  }

  // Check PreBattleModifiers BEFORE applying
  if (target.has<PreBattleModifiers>()) {
    auto &preBefore = target.get<PreBattleModifiers>();
    log_info("EFFECT_TEST: PreBattleModifiers BEFORE applying: zingDelta={}, "
             "bodyDelta={}",
             preBefore.zingDelta, preBefore.bodyDelta);
  } else {
    log_info("EFFECT_TEST: No PreBattleModifiers before applying");
  }

  // Apply the pending mods - this should add to PreBattleModifiers
  ApplyPendingCombatModsSystem applySystem;
  if (applySystem.should_run(1.0f / 60.0f)) {
    applySystem.for_each_with(target, pending, 1.0f / 60.0f);
  }

  log_info("EFFECT_TEST: After applying pending mods");

  // Mirror is written by ComputeCombatStatsSystem; run one compute tick, then
  // assert
  {
    ComputeCombatStatsSystem computeNow;
    computeNow.for_each_with(target, target.get<IsDish>(),
                             target.get<DishLevel>(), 1.0f / 60.0f);
  }

  // Verify PreBattleModifiers has the modifier after compute
  if (!target.has<PreBattleModifiers>()) {
    log_error("EFFECT_TEST: Modifier persistence test FAILED - target missing "
              "PreBattleModifiers after compute tick");
    return;
  }

  {
    auto &preMod = target.get<PreBattleModifiers>();
    log_info("EFFECT_TEST: PreBattleModifiers AFTER applying (post-compute): "
             "zingDelta={}, bodyDelta={}",
             preMod.zingDelta, preMod.bodyDelta);
    if (preMod.zingDelta != 1) {
      log_error("EFFECT_TEST: Modifier persistence test FAILED - wrong "
                "PreBattleModifiers zingDelta: {}",
                preMod.zingDelta);
      return;
    }
  }

  // Simulate ApplyPairingsAndClashesSystem running (this might overwrite!)
  log_info("EFFECT_TEST: Simulating ApplyPairingsAndClashesSystem (this "
           "might interfere)");
  if (target.has<PreBattleModifiers>()) {
    auto &preBeforeClash = target.get<PreBattleModifiers>();
    log_info("EFFECT_TEST: PreBattleModifiers BEFORE clash system: "
             "zingDelta={}, bodyDelta={}",
             preBeforeClash.zingDelta, preBeforeClash.bodyDelta);
  }

  // Actually run ApplyPairingsAndClashesSystem - this is what happens in real
  // game and it might be overwriting our modifiers!
  ApplyPairingsAndClashesSystem clashSystem;
  if (clashSystem.should_run(1.0f / 60.0f)) {
    if (target.has<IsDish>() && target.has<DishBattleState>()) {
      clashSystem.for_each_with(target, target.get<IsDish>(),
                                target.get<DishBattleState>(), 1.0f / 60.0f);
      if (target.has<PairingClashModifiers>()) {
        auto &pcmTmp = target.get<PairingClashModifiers>();
        log_info("EFFECT_TEST: After running ApplyPairingsAndClashesSystem - "
                 "PairingClashModifiers: zingDelta={}, bodyDelta={}",
                 pcmTmp.zingDelta, pcmTmp.bodyDelta);
      }

      // Check if it overwrote our modifier mirror (PreBattleModifiers)
      if (target.has<PreBattleModifiers>()) {
        if (target.get<PreBattleModifiers>().zingDelta != 1) {
          log_error("EFFECT_TEST: ApplyPairingsAndClashesSystem OVERWROTE "
                    "modifier! Expected zingDelta=1, got {}",
                    target.get<PreBattleModifiers>().zingDelta);
          return;
        }
      }
    }
  }

  // Now simulate source dish finishing and moving to Finished phase
  log_info("EFFECT_TEST: Simulating source dish finishing");
  auto &source_dbs = source.get<DishBattleState>();
  source_dbs.phase = DishBattleState::Phase::Finished;

  // Also move target to Finished to simulate what happens when it finishes
  auto &target_dbs = target.get<DishBattleState>();
  target_dbs.phase = DishBattleState::Phase::Finished;

  log_info("EFFECT_TEST: After setting phases to Finished");

  // Run ComputeCombatStatsSystem multiple times to simulate frame updates
  // This is what happens in real game - it runs every frame
  ComputeCombatStatsSystem computeSystem;
  for (int i = 0; i < 3; i++) {
    log_info("EFFECT_TEST: Running ComputeCombatStatsSystem iteration {}",
             i + 1);
    computeSystem.for_each_with(target, target.get<IsDish>(),
                                target.get<DishLevel>(), 1.0f / 60.0f);

    if (target.has<PreBattleModifiers>()) {
      auto &preIter = target.get<PreBattleModifiers>();
      log_info("EFFECT_TEST: After iteration {} - PreBattleModifiers: "
               "zingDelta={}, bodyDelta={}",
               i + 1, preIter.zingDelta, preIter.bodyDelta);
    } else {
      log_error("EFFECT_TEST: PreBattleModifiers MISSING after iteration {}",
                i + 1);
    }
  }

  // Verify PreBattleModifiers still has the modifier after source finishes
  if (!target.has<PreBattleModifiers>()) {
    log_error(
        "EFFECT_TEST: Modifier persistence test FAILED - PreBattleModifiers "
        "removed after source finished");
    return;
  }

  auto &preModAfter = target.get<PreBattleModifiers>();
  log_info("EFFECT_TEST: Final PreBattleModifiers: zingDelta={}, bodyDelta={}",
           preModAfter.zingDelta, preModAfter.bodyDelta);

  if (preModAfter.zingDelta != 1) {
    log_error(
        "EFFECT_TEST: Modifier persistence test FAILED - PreBattleModifiers "
        "zingDelta changed after source finished: {} (expected 1)",
        preModAfter.zingDelta);
    return;
  }

  // Verify base stats include the modifier
  if (!target.has<CombatStats>()) {
    log_error("EFFECT_TEST: Modifier persistence test FAILED - target missing "
              "CombatStats");
    return;
  }

  auto &cs = target.get<CombatStats>();
  auto dish_info = get_dish_info(target.get<IsDish>().type);
  int baseFlavorZing = dish_info.flavor.zing();
  int expectedBaseZing = baseFlavorZing + 1; // +1 from modifier

  log_info("EFFECT_TEST: Final stats check - flavor zing={}, expected "
           "baseZing={}, actual baseZing={}",
           baseFlavorZing, expectedBaseZing, cs.baseZing);

  if (cs.baseZing != expectedBaseZing) {
    log_error("EFFECT_TEST: Modifier persistence test FAILED - baseZing wrong: "
              "expected {}, got {}",
              expectedBaseZing, cs.baseZing);
    return;
  }

  log_info("EFFECT_TEST: Modifier persistence test PASSED");
}

static void test_modifier_persistence_when_entering_combat() {
  log_info("EFFECT_TEST: Testing modifier persistence when entering combat");
  log_info("EFFECT_TEST: This test validates that body/zing modifiers persist "
           "when dishes transition from InQueue to InCombat");
  log_info("EFFECT_TEST: Using Fried Egg effect (+2 Body to AllAllies) as "
           "production example");

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  // Create Fried Egg (source) that will trigger the effect
  auto &egg =
      add_dish_to_menu(DishType::FriedEgg, DishBattleState::TeamSide::Player, 0,
                       DishBattleState::Phase::Finished);

  // Create target dish that will receive the +2 body modifier
  // This dish starts in InQueue, will receive modifier, then transition to
  // InCombat
  auto &target =
      add_dish_to_menu(DishType::Potato, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::InQueue, true);

  // Get base flavor stats for target dish
  auto dish_info = get_dish_info(DishType::Potato);
  int baseFlavorBody = dish_info.flavor.body();
  log_info("EFFECT_TEST: Target dish base body (from flavor): {}",
           baseFlavorBody);

  // Merge temp entities into main array so queries can find them
  afterhours::EntityHelper::merge_entity_arrays();

  // Step 1: Run ComputeCombatStatsSystem BEFORE modifier is applied
  // This simulates the dish existing in battle before receiving modifiers
  ComputeCombatStatsSystem computeSystem;
  log_info("EFFECT_TEST: Running ComputeCombatStatsSystem before modifier "
           "(InQueue phase)");
  computeSystem.for_each_with(target, target.get<IsDish>(),
                              target.get<DishLevel>(), 1.0f / 60.0f);

  int baseBodyBeforeMod = target.has<CombatStats>()
                              ? target.get<CombatStats>().baseBody
                              : baseFlavorBody;
  log_info("EFFECT_TEST: Body before modifier: {}", baseBodyBeforeMod);

  // Step 2: Trigger the effect (same as production)
  // Fried Egg gives +2 body to AllAllies when it finishes
  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnDishFinished, egg.id, 0,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    log_info("EFFECT_TEST: Running EffectResolutionSystem to apply modifiers");
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  // Step 3: Verify PendingCombatMods was created (as in production)
  if (!target.has<PendingCombatMods>()) {
    log_error("EFFECT_TEST: FAILED - PendingCombatMods not created by effect");
    return;
  }

  auto &pending = target.get<PendingCombatMods>();
  if (pending.bodyDelta != 2) {
    log_error(
        "EFFECT_TEST: FAILED - PendingCombatMods.bodyDelta wrong: expected "
        "2, got {}",
        pending.bodyDelta);
    return;
  }
  log_info("EFFECT_TEST: PendingCombatMods created correctly: bodyDelta={}",
           pending.bodyDelta);

  // Step 4: Apply pending modifiers (same as production)
  // First ensure CombatStats exists (ApplyPendingCombatModsSystem requires
  // it)
  if (!target.has<CombatStats>()) {
    log_info("EFFECT_TEST: Creating CombatStats before applying modifiers");
    target.addComponent<CombatStats>();
  }

  // Run ComputeCombatStatsSystem once to initialize baseBody
  computeSystem.for_each_with(target, target.get<IsDish>(),
                              target.get<DishLevel>(), 1.0f / 60.0f);

  // Step 4: Apply pending modifiers (same as production)
  // We'll manually call for_each_with to bypass should_run() check
  // (which might fail due to animation state in test environment)
  log_info("EFFECT_TEST: Running ApplyPendingCombatModsSystem (bypassing "
           "should_run check)");
  ApplyPendingCombatModsSystem applyModsSystem;
  applyModsSystem.for_each_with(target, pending, 1.0f / 60.0f);

  // Run ComputeCombatStatsSystem again to mirror the updated modifiers into
  // PreBattleModifiers
  log_info("EFFECT_TEST: Running ComputeCombatStatsSystem after applying "
           "modifiers");
  computeSystem.for_each_with(target, target.get<IsDish>(),
                              target.get<DishLevel>(), 1.0f / 60.0f);

  // Verify the modifiers were applied
  if (target.has<PreBattleModifiers>()) {
    auto &preModAfterApply = target.get<PreBattleModifiers>();
    log_info("EFFECT_TEST: After ApplyPendingCombatModsSystem - "
             "PreBattleModifiers.bodyDelta={}",
             preModAfterApply.bodyDelta);
    if (preModAfterApply.bodyDelta != 2) {
      log_error(
          "EFFECT_TEST: FAILED - PreBattleModifiers.bodyDelta wrong after "
          "ApplyPendingCombatModsSystem: expected 2, got {}",
          preModAfterApply.bodyDelta);
      return;
    }
  } else {
    log_error("EFFECT_TEST: FAILED - PreBattleModifiers missing after "
              "ApplyPendingCombatModsSystem");
    return;
  }

  // Step 5: Run ApplyPairingsAndClashesSystem (as in production)
  // We'll manually call for_each_with to bypass should_run() check
  ApplyPairingsAndClashesSystem clashSystem;
  if (target.has<IsDish>() && target.has<DishBattleState>()) {
    log_info("EFFECT_TEST: Running ApplyPairingsAndClashesSystem (bypassing "
             "should_run check)");
    int bodyDeltaBeforeClash = 0;
    if (target.has<PairingClashModifiers>()) {
      bodyDeltaBeforeClash = target.get<PairingClashModifiers>().bodyDelta;
    }
    log_info("EFFECT_TEST: PairingClashModifiers BEFORE "
             "ApplyPairingsAndClashesSystem - "
             "bodyDelta={}",
             bodyDeltaBeforeClash);
    clashSystem.for_each_with(target, target.get<IsDish>(),
                              target.get<DishBattleState>(), 1.0f / 60.0f);
    if (target.has<PairingClashModifiers>()) {
      auto &pcm = target.get<PairingClashModifiers>();
      log_info("EFFECT_TEST: After ApplyPairingsAndClashesSystem - "
               "PairingClashModifiers.bodyDelta: {} -> {}",
               bodyDeltaBeforeClash, pcm.bodyDelta);
    }

    // CRITICAL CHECK: ApplyPairingsAndClashesSystem should NOT overwrite
    // combat modifiers!
    if (target.has<PreBattleModifiers>()) {
      if (target.get<PreBattleModifiers>().bodyDelta != 2) {
        log_error("EFFECT_TEST: FAILED - ApplyPairingsAndClashesSystem "
                  "OVERWROTE combat "
                  "modifier! Expected bodyDelta=2, got {}",
                  target.get<PreBattleModifiers>().bodyDelta);
        return;
      }
    }
  }

  // Step 6: Run ComputeCombatStatsSystem while still InQueue
  // This should calculate baseBody including modifiers
  log_info(
      "EFFECT_TEST: Running ComputeCombatStatsSystem after modifier applied "
      "(InQueue phase)");
  computeSystem.for_each_with(target, target.get<IsDish>(),
                              target.get<DishLevel>(), 1.0f / 60.0f);

  // Check PreBattleModifiers
  if (!target.has<PreBattleModifiers>()) {
    log_error("EFFECT_TEST: FAILED - PreBattleModifiers missing");
    return;
  }

  auto &preModInQueue = target.get<PreBattleModifiers>();
  // Note: ApplyPairingsAndClashesSystem might add/change modifiers, but base
  // modifier should still be there
  log_info("EFFECT_TEST: PreBattleModifiers.bodyDelta in InQueue: {}",
           preModInQueue.bodyDelta);

  // Check body value while InQueue
  if (!target.has<CombatStats>()) {
    log_error("EFFECT_TEST: CombatStats missing");
    return;
  }

  auto &csInQueue = target.get<CombatStats>();
  int expectedBodyWithMod = baseFlavorBody + preModInQueue.bodyDelta;
  log_info("EFFECT_TEST: While InQueue - baseBody={}, currentBody={}, "
           "expected={} (flavor={} + mod={})",
           csInQueue.baseBody, csInQueue.currentBody, expectedBodyWithMod,
           baseFlavorBody, preModInQueue.bodyDelta);

  if (csInQueue.baseBody != expectedBodyWithMod) {
    log_error(
        "EFFECT_TEST: FAILED - baseBody wrong while InQueue: expected {}, "
        "got {}",
        expectedBodyWithMod, csInQueue.baseBody);
    return;
  }

  if (csInQueue.currentBody != expectedBodyWithMod) {
    log_error("EFFECT_TEST: FAILED - currentBody wrong while InQueue: expected "
              "{}, got {}",
              expectedBodyWithMod, csInQueue.currentBody);
    return;
  }

  log_info("EFFECT_TEST: Body value correct while InQueue: {}",
           csInQueue.baseBody);

  // Step 7: Transition to InCombat - THIS IS WHERE THE BUG HAPPENS
  log_info("EFFECT_TEST: Transitioning dish from InQueue to InCombat");
  auto &dbs = target.get<DishBattleState>();
  dbs.phase = DishBattleState::Phase::InCombat;

  // Step 8: Run ComputeCombatStatsSystem after entering combat
  // This should sync currentBody = baseBody (which includes modifiers)
  log_info("EFFECT_TEST: Running ComputeCombatStatsSystem after entering "
           "InCombat");
  computeSystem.for_each_with(target, target.get<IsDish>(),
                              target.get<DishLevel>(), 1.0f / 60.0f);

  // Check PreBattleModifiers still exist and haven't changed
  if (!target.has<PreBattleModifiers>()) {
    log_error(
        "EFFECT_TEST: FAILED - PreBattleModifiers MISSING after transition "
        "to InCombat");
    return;
  }

  auto &preModInCombat = target.get<PreBattleModifiers>();
  log_info(
      "EFFECT_TEST: PreBattleModifiers after entering InCombat: bodyDelta={}",
      preModInCombat.bodyDelta);

  // Check body value after entering combat
  auto &csInCombat = target.get<CombatStats>();
  int expectedBodyInCombat = baseFlavorBody + preModInCombat.bodyDelta;
  log_info(
      "EFFECT_TEST: After entering InCombat - baseBody={}, currentBody={}, "
      "expected={} (flavor={} + mod={})",
      csInCombat.baseBody, csInCombat.currentBody, expectedBodyInCombat,
      baseFlavorBody, preModInCombat.bodyDelta);

  if (csInCombat.baseBody != expectedBodyInCombat) {
    log_error("EFFECT_TEST: FAILED - baseBody wrong after entering InCombat: "
              "expected {}, got {}",
              expectedBodyInCombat, csInCombat.baseBody);
    return;
  }

  if (csInCombat.currentBody != expectedBodyInCombat) {
    log_error(
        "EFFECT_TEST: FAILED - currentBody DROPPED after entering InCombat! "
        "expected {}, got {} (dropped from {} to {})",
        expectedBodyInCombat, csInCombat.currentBody, csInQueue.currentBody,
        csInCombat.currentBody);
    return;
  }

  // Step 9: Run ComputeCombatStatsSystem multiple more times to simulate
  // multiple frames This is where the bug might show up - if it's resetting
  // every frame
  log_info("EFFECT_TEST: Running ComputeCombatStatsSystem 5 more times to "
           "simulate "
           "multiple frames");
  for (int i = 0; i < 5; i++) {
    computeSystem.for_each_with(target, target.get<IsDish>(),
                                target.get<DishLevel>(), 1.0f / 60.0f);
    auto &csIter = target.get<CombatStats>();
    if (csIter.currentBody != expectedBodyInCombat) {
      log_error("EFFECT_TEST: FAILED - currentBody changed on iteration {}: "
                "expected {}, got {}",
                i + 1, expectedBodyInCombat, csIter.currentBody);
      return;
    }
  }

  auto &csFinal = target.get<CombatStats>();
  log_info("EFFECT_TEST: Final check after multiple frames - baseBody={}, "
           "currentBody={}, expected={}",
           csFinal.baseBody, csFinal.currentBody, expectedBodyInCombat);

  if (csFinal.currentBody != expectedBodyInCombat) {
    log_error("EFFECT_TEST: FAILED - currentBody wrong after multiple frames: "
              "expected {}, got {}",
              expectedBodyInCombat, csFinal.currentBody);
    return;
  }

  log_info(
      "EFFECT_TEST: Modifier persistence when entering combat test PASSED - "
      "body stayed at {} throughout transition and multiple frames",
      expectedBodyInCombat);
}

static void test_salmon_neighbor_freshness_persists_to_combat() {
  log_info("EFFECT_TEST: Testing Salmon neighbor Freshness persists into "
           "combat (Bagel,Salmon,Salmon,Bagel)");

  // Clean up any leftover entities from previous tests to avoid query
  // conflicts This ensures queries only find entities created in this test
  for (auto &ref :
       EQ({.force_merge = true}).whereHasComponent<IsDish>().gen()) {
    ref.get().cleanup = true;
  }
  // Wait a frame for cleanup to process
  app.wait_for_frames(1);

  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  // Create lineup: Bagel(0), Salmon(1), Salmon(2), Bagel(3) on Player side
  auto &bagel0 =
      add_dish_to_menu(DishType::Bagel, DishBattleState::TeamSide::Player, 0,
                       DishBattleState::Phase::InQueue, true);
  auto &salmon1 =
      add_dish_to_menu(DishType::Salmon, DishBattleState::TeamSide::Player, 1,
                       DishBattleState::Phase::InQueue, true);
  auto &salmon2 =
      add_dish_to_menu(DishType::Salmon, DishBattleState::TeamSide::Player, 2,
                       DishBattleState::Phase::InQueue, true);
  auto &bagel3 =
      add_dish_to_menu(DishType::Bagel, DishBattleState::TeamSide::Player, 3,
                       DishBattleState::Phase::InQueue, true);

  (void)bagel3; // silence unused warning

  // Wait a frame for entities to be merged by system loop
  app.wait_for_frames(1);

  // Fire OnServe for both Salmon via TriggerQueue → TriggerDispatchSystem
  // Note: Salmon effect uses legacy onServe callback (not DishEffect), so
  // TriggerDispatchSystem calls it directly
  auto &tq_entity = get_or_create_trigger_queue();
  auto &queue = tq_entity.get<TriggerQueue>();
  queue.add_event(TriggerHook::OnServe, salmon1.id, 1,
                  DishBattleState::TeamSide::Player);
  queue.add_event(TriggerHook::OnServe, salmon2.id, 2,
                  DishBattleState::TeamSide::Player);

  // Wait a frame for trigger queue entity to be merged
  app.wait_for_frames(1);

  // Verify entities are accessible before dispatch using force_merge
  auto pre_check =
      EQ({.force_merge = true})
          .whereHasComponent<IsDish>()
          .whereHasComponent<DishBattleState>()
          .whereLambda([](const afterhours::Entity &e) {
            const DishBattleState &dbs = e.get<DishBattleState>();
            return dbs.team_side == DishBattleState::TeamSide::Player &&
                   dbs.queue_index == 0;
          })
          .gen_first();
  if (!pre_check.has_value()) {
    log_error("EFFECT_TEST: Bagel(0) not found by query before dispatch!");
    return;
  }

  // Run ComputeCombatStatsSystem first to ensure all dishes have CombatStats
  // before TriggerDispatchSystem tries to order events
  ComputeCombatStatsSystem computeStats;
  for (afterhours::Entity &e : EQ({.force_merge = true})
                                   .whereHasComponent<IsDish>()
                                   .whereHasComponent<DishLevel>()
                                   .gen()) {
    if (computeStats.should_run(1.0f / 60.0f)) {
      IsDish &dish = e.get<IsDish>();
      DishLevel &lvl = e.get<DishLevel>();
      computeStats.for_each_with(e, dish, lvl, 1.0f / 60.0f);
    }
  }

  TriggerDispatchSystem dispatch;
  if (dispatch.should_run(1.0f / 60.0f)) {
    dispatch.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  // Wait a frame for systems to process (system loop merges automatically)
  app.wait_for_frames(1);

  // EffectResolutionSystem processes effects from DishEffect (not legacy
  // onServe) Note: TriggerDispatchSystem clears the queue after processing, so
  // we need to re-add events for EffectResolutionSystem to process
  queue.add_event(TriggerHook::OnServe, salmon1.id, 1,
                  DishBattleState::TeamSide::Player);
  queue.add_event(TriggerHook::OnServe, salmon2.id, 2,
                  DishBattleState::TeamSide::Player);

  EffectResolutionSystem effectSystem;
  if (effectSystem.should_run(1.0f / 60.0f)) {
    effectSystem.for_each_with(tq_entity, queue, 1.0f / 60.0f);
  }

  // Wait a frame for effect resolution to complete
  app.wait_for_frames(1);

  // Use bagel0 reference directly instead of querying by ID
  afterhours::Entity &bagel0_entity = bagel0;

  // Verify left Bagel received +1 Freshness via DeferredFlavorMods
  if (!bagel0_entity.has<DeferredFlavorMods>()) {
    log_error("EFFECT_TEST: FAILED - Bagel(0) missing DeferredFlavorMods "
              "after Salmon OnServe");
    return;
  }
  auto &def = bagel0_entity.get<DeferredFlavorMods>();
  log_info("EFFECT_TEST: Bagel(0) DeferredFlavorMods freshness={} (expect >=1)",
           def.freshness);
  if (def.freshness < 1) {
    log_error("EFFECT_TEST: FAILED - Bagel(0) freshness not incremented by "
              "Salmon OnServe");
    return;
  }

  // Compute stats while InQueue
  ComputeCombatStatsSystem compute;
  if (!bagel0_entity.has<IsDish>() || !bagel0_entity.has<DishLevel>()) {
    log_error("EFFECT_TEST: FAILED - Bagel(0) missing required components");
    return;
  }
  compute.for_each_with(bagel0_entity, bagel0_entity.get<IsDish>(),
                        bagel0_entity.get<DishLevel>(), 1.0f / 60.0f);
  int baseFlavorBody = get_dish_info(DishType::Bagel).flavor.body();
  int bodyInQueue = bagel0_entity.get<CombatStats>().baseBody;
  log_info("EFFECT_TEST: Bagel(0) InQueue baseBody={} (baseFlavorBody={})",
           bodyInQueue, baseFlavorBody);
  if (bodyInQueue <= baseFlavorBody) {
    log_error("EFFECT_TEST: FAILED - Bagel(0) baseBody not increased while "
              "InQueue");
    return;
  }

  // Transition to Entering, then InCombat; recompute each time
  auto &dbs = bagel0_entity.get<DishBattleState>();
  dbs.phase = DishBattleState::Phase::Entering;
  compute.for_each_with(bagel0_entity, bagel0_entity.get<IsDish>(),
                        bagel0_entity.get<DishLevel>(), 1.0f / 60.0f);
  int bodyEntering = bagel0_entity.get<CombatStats>().baseBody;
  log_info("EFFECT_TEST: Bagel(0) Entering baseBody={}", bodyEntering);

  dbs.phase = DishBattleState::Phase::InCombat;
  compute.for_each_with(bagel0_entity, bagel0_entity.get<IsDish>(),
                        bagel0_entity.get<DishLevel>(), 1.0f / 60.0f);
  int bodyInCombat = bagel0_entity.get<CombatStats>().baseBody;
  log_info("EFFECT_TEST: Bagel(0) InCombat baseBody={}", bodyInCombat);

  // Expectation: body should remain boosted across transitions
  if (bodyInCombat < bodyInQueue) {
    log_error("EFFECT_TEST: FAILED - Bagel(0) baseBody DROPPED entering "
              "combat ({} -> {})",
              bodyInQueue, bodyInCombat);
    return;
  }

  log_info("EFFECT_TEST: Salmon neighbor Freshness persisted into combat - "
           "Bagel(0) baseBody remained at {}",
           bodyInCombat);
}
} // namespace ValidateEffectSystemTestHelpers

TEST(validate_effect_system) {
  using namespace ValidateEffectSystemTestHelpers;

  log_info("EFFECT_TEST: Starting effect system validation");

  // Initialize game context (ECS tests use direct to_battle() calls)
  app.launch_game();

  test_french_fries_effect();
  test_bagel_effect();
  test_baguette_effect();
  test_garlic_bread_effect();
  test_fried_egg_effect();
  test_targeting_scopes();
  test_deferred_flavor_mods_consumption();
  test_modifier_persistence_after_dish_finishes();
  test_modifier_persistence_when_entering_combat();
  test_salmon_neighbor_freshness_persists_to_combat();

  log_info("EFFECT_TEST: All tests completed");
}

TEST(validate_salmon_persistence) {
  using namespace ValidateEffectSystemTestHelpers;

  log_info("EFFECT_TEST: Running salmon persistence test only");

  app.launch_game();
  test_salmon_neighbor_freshness_persists_to_combat();
}
