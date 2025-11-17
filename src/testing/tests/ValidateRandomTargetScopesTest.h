#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/combat_stats.h"
#include "../../components/deferred_flavor_mods.h"
#include "../../components/dish_battle_state.h"
#include "../../components/dish_effect.h"
#include "../../components/drink_effects.h"
#include "../../components/pending_combat_mods.h"
#include "../../components/trigger_event.h"
#include "../../components/trigger_queue.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../query.h"
#include "../../seeded_rng.h"
#include "../test_app.h"
#include "../test_macros.h"

namespace ValidateRandomTargetScopesTestHelpers {

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

static void ensure_battle_load_request_exists() {
  const auto componentId =
      afterhours::components::get_type_id<BattleLoadRequest>();
  auto &singletonMap = afterhours::EntityHelper::get().singletonMap;
  if (singletonMap.contains(componentId)) {
    return;
  }
  auto &requestEntity = afterhours::EntityHelper::createEntity();
  BattleLoadRequest request;
  request.playerJsonPath = "";
  request.opponentJsonPath = "";
  request.loaded = false;
  requestEntity.addComponent<BattleLoadRequest>(std::move(request));
  afterhours::EntityHelper::registerSingleton<BattleLoadRequest>(requestEntity);
}

static void test_random_ally_scope(TestApp &app) {
  log_info("RANDOM_TARGET_TEST: Testing RandomAlly scope");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  // Set seed for deterministic testing
  SeededRng::get().set_seed(12345);

  // Create source dish
  auto source_id = app.create_dish(DishType::Potato)
                       .on_team(DishBattleState::TeamSide::Player)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::InQueue)
                       .commit();

  // Create multiple ally dishes
  auto ally1_id = app.create_dish(DishType::Salmon)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(1)
                      .in_phase(DishBattleState::Phase::InQueue)
                      .commit();

  auto ally2_id = app.create_dish(DishType::Bagel)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(2)
                      .in_phase(DishBattleState::Phase::InQueue)
                      .commit();

  app.wait_for_frames(5);

  // Add effect to source dish that targets RandomAlly
  afterhours::OptEntity source_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(source_id)
          .gen_first();
  app.expect_true(source_opt.has_value(), "source dish exists");

  auto &source = source_opt.asE();
  auto &drink_effects = source.addComponentIfMissing<DrinkEffects>();
  DishEffect random_ally_effect(TriggerHook::OnServe,
                                EffectOperation::AddCombatZing,
                                TargetScope::RandomAlly, 5);
  drink_effects.effects.push_back(random_ally_effect);

  // Trigger OnServe
  auto &tq_entity = get_or_create_trigger_queue();
  auto &tq = tq_entity.get<TriggerQueue>();
  TriggerEvent serve_event;
  serve_event.hook = TriggerHook::OnServe;
  serve_event.sourceEntityId = source_id;
  tq.events.push_back(serve_event);

  app.wait_for_frames(10);

  // Check that exactly one ally received the effect
  afterhours::OptEntity ally1_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(ally1_id)
          .gen_first();
  afterhours::OptEntity ally2_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(ally2_id)
          .gen_first();

  app.expect_true(ally1_opt.has_value(), "ally1 exists");
  app.expect_true(ally2_opt.has_value(), "ally2 exists");

  bool ally1_got_effect =
      ally1_opt.asE().has<PendingCombatMods>() &&
      ally1_opt.asE().get<PendingCombatMods>().zingDelta > 0;
  bool ally2_got_effect =
      ally2_opt.asE().has<PendingCombatMods>() &&
      ally2_opt.asE().get<PendingCombatMods>().zingDelta > 0;

  // Exactly one should have received the effect
  app.expect_true(ally1_got_effect != ally2_got_effect,
                  "exactly one ally received RandomAlly effect");

  log_info("RANDOM_TARGET_TEST: RandomAlly scope PASSED");
}

static void test_random_opponent_scope(TestApp &app) {
  log_info("RANDOM_TARGET_TEST: Testing RandomOpponent scope");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  // Set seed for deterministic testing
  SeededRng::get().set_seed(54321);

  // Create source dish on player team
  auto source_id = app.create_dish(DishType::Potato)
                       .on_team(DishBattleState::TeamSide::Player)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::InQueue)
                       .commit();

  // Create multiple opponent dishes
  auto opp1_id = app.create_dish(DishType::Salmon)
                     .on_team(DishBattleState::TeamSide::Opponent)
                     .at_slot(0)
                     .in_phase(DishBattleState::Phase::InQueue)
                     .commit();

  auto opp2_id = app.create_dish(DishType::Bagel)
                     .on_team(DishBattleState::TeamSide::Opponent)
                     .at_slot(1)
                     .in_phase(DishBattleState::Phase::InQueue)
                     .commit();

  app.wait_for_frames(5);

  // Add effect to source dish that targets RandomOpponent
  afterhours::OptEntity source_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(source_id)
          .gen_first();
  app.expect_true(source_opt.has_value(), "source dish exists");

  auto &source = source_opt.asE();
  auto &drink_effects = source.addComponentIfMissing<DrinkEffects>();
  DishEffect random_opp_effect(TriggerHook::OnServe,
                               EffectOperation::AddCombatZing,
                               TargetScope::RandomOpponent, 5);
  drink_effects.effects.push_back(random_opp_effect);

  // Trigger OnServe
  auto &tq_entity = get_or_create_trigger_queue();
  auto &tq = tq_entity.get<TriggerQueue>();
  TriggerEvent serve_event;
  serve_event.hook = TriggerHook::OnServe;
  serve_event.sourceEntityId = source_id;
  tq.events.push_back(serve_event);

  app.wait_for_frames(10);

  // Check that exactly one opponent received the effect
  afterhours::OptEntity opp1_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(opp1_id)
          .gen_first();
  afterhours::OptEntity opp2_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(opp2_id)
          .gen_first();

  app.expect_true(opp1_opt.has_value(), "opp1 exists");
  app.expect_true(opp2_opt.has_value(), "opp2 exists");

  bool opp1_got_effect = opp1_opt.asE().has<PendingCombatMods>() &&
                         opp1_opt.asE().get<PendingCombatMods>().zingDelta > 0;
  bool opp2_got_effect = opp2_opt.asE().has<PendingCombatMods>() &&
                         opp2_opt.asE().get<PendingCombatMods>().zingDelta > 0;

  // Exactly one should have received the effect
  app.expect_true(opp1_got_effect != opp2_got_effect,
                  "exactly one opponent received RandomOpponent effect");

  log_info("RANDOM_TARGET_TEST: RandomOpponent scope PASSED");
}

static void test_random_dish_scope(TestApp &app) {
  log_info("RANDOM_TARGET_TEST: Testing RandomDish scope");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  // Set seed for deterministic testing
  SeededRng::get().set_seed(99999);

  // Create source dish
  auto source_id = app.create_dish(DishType::Potato)
                       .on_team(DishBattleState::TeamSide::Player)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::InQueue)
                       .commit();

  // Create dishes on both teams
  auto ally_id = app.create_dish(DishType::Salmon)
                     .on_team(DishBattleState::TeamSide::Player)
                     .at_slot(1)
                     .in_phase(DishBattleState::Phase::InQueue)
                     .commit();

  auto opp_id = app.create_dish(DishType::Bagel)
                    .on_team(DishBattleState::TeamSide::Opponent)
                    .at_slot(0)
                    .in_phase(DishBattleState::Phase::InQueue)
                    .commit();

  app.wait_for_frames(5);

  // Add effect to source dish that targets RandomDish
  afterhours::OptEntity source_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(source_id)
          .gen_first();
  app.expect_true(source_opt.has_value(), "source dish exists");

  auto &source = source_opt.asE();
  auto &drink_effects = source.addComponentIfMissing<DrinkEffects>();
  DishEffect random_dish_effect(TriggerHook::OnServe,
                                EffectOperation::AddCombatZing,
                                TargetScope::RandomDish, 5);
  drink_effects.effects.push_back(random_dish_effect);

  // Trigger OnServe
  auto &tq_entity = get_or_create_trigger_queue();
  auto &tq = tq_entity.get<TriggerQueue>();
  TriggerEvent serve_event;
  serve_event.hook = TriggerHook::OnServe;
  serve_event.sourceEntityId = source_id;
  tq.events.push_back(serve_event);

  app.wait_for_frames(10);

  // Check that exactly one dish (not source) received the effect
  afterhours::OptEntity ally_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(ally_id)
          .gen_first();
  afterhours::OptEntity opp_opt = afterhours::EntityQuery({.force_merge = true})
                                      .whereID(opp_id)
                                      .gen_first();

  app.expect_true(ally_opt.has_value(), "ally exists");
  app.expect_true(opp_opt.has_value(), "opp exists");

  bool ally_got_effect = ally_opt.asE().has<PendingCombatMods>() &&
                         ally_opt.asE().get<PendingCombatMods>().zingDelta > 0;
  bool opp_got_effect = opp_opt.asE().has<PendingCombatMods>() &&
                        opp_opt.asE().get<PendingCombatMods>().zingDelta > 0;

  // Exactly one should have received the effect (not source)
  app.expect_true(ally_got_effect != opp_got_effect,
                  "exactly one dish (not source) received RandomDish effect");

  // Source should not have received the effect
  afterhours::OptEntity source_after_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(source_id)
          .gen_first();
  app.expect_true(source_after_opt.has_value(), "source still exists");
  bool source_got_effect =
      source_after_opt.asE().has<PendingCombatMods>() &&
      source_after_opt.asE().get<PendingCombatMods>().zingDelta > 0;
  app.expect_false(source_got_effect,
                   "source did not receive RandomDish effect");

  log_info("RANDOM_TARGET_TEST: RandomDish scope PASSED");
}

static void test_random_other_ally_scope(TestApp &app) {
  log_info(
      "RANDOM_TARGET_TEST: Testing RandomOtherAlly scope (same as RandomAlly)");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  // Set seed for deterministic testing
  SeededRng::get().set_seed(11111);

  // Create source dish
  auto source_id = app.create_dish(DishType::Potato)
                       .on_team(DishBattleState::TeamSide::Player)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::InQueue)
                       .commit();

  // Create multiple ally dishes
  auto ally1_id = app.create_dish(DishType::Salmon)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(1)
                      .in_phase(DishBattleState::Phase::InQueue)
                      .commit();

  auto ally2_id = app.create_dish(DishType::Bagel)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(2)
                      .in_phase(DishBattleState::Phase::InQueue)
                      .commit();

  app.wait_for_frames(5);

  // Add effect to source dish that targets RandomOtherAlly
  afterhours::OptEntity source_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(source_id)
          .gen_first();
  app.expect_true(source_opt.has_value(), "source dish exists");

  auto &source = source_opt.asE();
  auto &drink_effects = source.addComponentIfMissing<DrinkEffects>();
  DishEffect random_other_ally_effect(TriggerHook::OnServe,
                                      EffectOperation::AddCombatZing,
                                      TargetScope::RandomOtherAlly, 5);
  drink_effects.effects.push_back(random_other_ally_effect);

  // Trigger OnServe
  auto &tq_entity = get_or_create_trigger_queue();
  auto &tq = tq_entity.get<TriggerQueue>();
  TriggerEvent serve_event;
  serve_event.hook = TriggerHook::OnServe;
  serve_event.sourceEntityId = source_id;
  tq.events.push_back(serve_event);

  app.wait_for_frames(10);

  // Check that exactly one ally received the effect
  afterhours::OptEntity ally1_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(ally1_id)
          .gen_first();
  afterhours::OptEntity ally2_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(ally2_id)
          .gen_first();

  app.expect_true(ally1_opt.has_value(), "ally1 exists");
  app.expect_true(ally2_opt.has_value(), "ally2 exists");

  bool ally1_got_effect =
      ally1_opt.asE().has<PendingCombatMods>() &&
      ally1_opt.asE().get<PendingCombatMods>().zingDelta > 0;
  bool ally2_got_effect =
      ally2_opt.asE().has<PendingCombatMods>() &&
      ally2_opt.asE().get<PendingCombatMods>().zingDelta > 0;

  // Exactly one should have received the effect
  app.expect_true(ally1_got_effect != ally2_got_effect,
                  "exactly one ally received RandomOtherAlly effect");

  log_info("RANDOM_TARGET_TEST: RandomOtherAlly scope PASSED");
}

static void test_random_target_edge_cases(TestApp &app) {
  log_info("RANDOM_TARGET_TEST: Testing edge cases (no valid targets)");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  // Set seed for deterministic testing
  SeededRng::get().set_seed(22222);

  // Create source dish with no allies
  auto source_id = app.create_dish(DishType::Potato)
                       .on_team(DishBattleState::TeamSide::Player)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::InQueue)
                       .commit();

  app.wait_for_frames(5);

  // Add effect to source dish that targets RandomAlly (no allies exist)
  afterhours::OptEntity source_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(source_id)
          .gen_first();
  app.expect_true(source_opt.has_value(), "source dish exists");

  auto &source = source_opt.asE();
  auto &drink_effects = source.addComponentIfMissing<DrinkEffects>();
  DishEffect random_ally_effect(TriggerHook::OnServe,
                                EffectOperation::AddCombatZing,
                                TargetScope::RandomAlly, 5);
  drink_effects.effects.push_back(random_ally_effect);

  // Trigger OnServe
  auto &tq_entity = get_or_create_trigger_queue();
  auto &tq = tq_entity.get<TriggerQueue>();
  TriggerEvent serve_event;
  serve_event.hook = TriggerHook::OnServe;
  serve_event.sourceEntityId = source_id;
  tq.events.push_back(serve_event);

  app.wait_for_frames(10);

  // Effect should silently skip (no error, no effect applied)
  // Source should not have received the effect
  afterhours::OptEntity source_after_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(source_id)
          .gen_first();
  app.expect_true(source_after_opt.has_value(), "source still exists");
  bool source_got_effect =
      source_after_opt.asE().has<PendingCombatMods>() &&
      source_after_opt.asE().get<PendingCombatMods>().zingDelta > 0;
  app.expect_false(source_got_effect,
                   "RandomAlly with no allies silently skips (no error)");

  log_info("RANDOM_TARGET_TEST: Edge cases PASSED");
}

} // namespace ValidateRandomTargetScopesTestHelpers

REGISTER_TEST(validate_random_target_scopes_random_ally,
              ValidateRandomTargetScopesTestHelpers::test_random_ally_scope);

REGISTER_TEST(
    validate_random_target_scopes_random_opponent,
    ValidateRandomTargetScopesTestHelpers::test_random_opponent_scope);

REGISTER_TEST(validate_random_target_scopes_random_dish,
              ValidateRandomTargetScopesTestHelpers::test_random_dish_scope);

REGISTER_TEST(
    validate_random_target_scopes_random_other_ally,
    ValidateRandomTargetScopesTestHelpers::test_random_other_ally_scope);

REGISTER_TEST(
    validate_random_target_scopes_edge_cases,
    ValidateRandomTargetScopesTestHelpers::test_random_target_edge_cases);
