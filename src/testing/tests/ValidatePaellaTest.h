#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/dish_battle_state.h"
#include "../../components/drink_effects.h"
#include "../../components/drink_pairing.h"
#include "../../components/is_dish.h"
#include "../../components/trigger_event.h"
#include "../../components/trigger_queue.h"
#include "../../dish_types.h"
#include "../../drink_types.h"
#include "../../game_state_manager.h"
#include "../../query.h"
#include "../../seeded_rng.h"
#include "../test_app.h"
#include "../test_macros.h"

namespace ValidatePaellaTestHelpers {

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

static void test_paella_copies_effects(TestApp &app) {
  log_info("PAELLA_TEST: Testing Paella copies effects from random ally");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  // Set seed for deterministic testing
  SeededRng::get().set_seed(88888);

  // Create Paella dish
  auto paella_id = app.create_dish(DishType::Paella)
                       .on_team(DishBattleState::TeamSide::Player)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::InQueue)
                       .commit();

  // Create ally dish with drink effect
  auto ally_id = app.create_dish(DishType::Salmon)
                     .on_team(DishBattleState::TeamSide::Player)
                     .at_slot(1)
                     .in_phase(DishBattleState::Phase::InQueue)
                     .commit();

  app.wait_for_frames(5);

  // Add drink effect to ally
  afterhours::OptEntity ally_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(ally_id)
          .gen_first();
  app.expect_true(ally_opt.has_value(), "ally exists");
  ally_opt.asE().addComponent<DrinkPairing>(DrinkType::Coffee);

  app.wait_for_frames(10);

  // Trigger OnStartBattle (Paella's effect triggers here)
  auto &tq_entity = get_or_create_trigger_queue();
  auto &tq = tq_entity.get<TriggerQueue>();
  TriggerEvent start_event;
  start_event.hook = TriggerHook::OnStartBattle;
  start_event.sourceEntityId = paella_id;
  tq.events.push_back(start_event);

  app.wait_for_frames(10);

  // Check that Paella has copied effects (should have DrinkEffects)
  afterhours::OptEntity paella_after_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(paella_id)
          .gen_first();
  app.expect_true(paella_after_opt.has_value(), "paella still exists");

  // Paella should have DrinkEffects with copied effects
  bool has_drink_effects = paella_after_opt.asE().has<DrinkEffects>();
  app.expect_true(has_drink_effects, "paella has DrinkEffects after copy");

  if (has_drink_effects) {
    const auto &effects = paella_after_opt.asE().get<DrinkEffects>();
    // Should have at least the copied effects
    app.expect_true(effects.effects.size() > 0,
                    "paella has copied effects from random ally");
  }

  log_info("PAELLA_TEST: Paella copies effects from random ally PASSED");
}

} // namespace ValidatePaellaTestHelpers

REGISTER_TEST(validate_paella_copies_effects,
              ValidatePaellaTestHelpers::test_paella_copies_effects);
