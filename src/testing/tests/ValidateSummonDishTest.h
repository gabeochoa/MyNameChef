#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/dish_battle_state.h"
#include "../../components/dish_effect.h"
#include "../../components/drink_effects.h"
#include "../../components/is_dish.h"
#include "../../components/trigger_event.h"
#include "../../components/trigger_queue.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../query.h"
#include "../test_app.h"
#include "../test_macros.h"

namespace ValidateSummonDishTestHelpers {

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

static void test_summon_dish_creates_entity(TestApp &app) {
  log_info("SUMMON_TEST: Testing SummonDish creates valid battle entity");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  // Create source dish
  auto source_id = app.create_dish(DishType::Potato)
                       .on_team(DishBattleState::TeamSide::Player)
                       .at_slot(0)
                       .in_phase(DishBattleState::Phase::InQueue)
                       .commit();

  app.wait_for_frames(5);

  // Add SummonDish effect to source dish
  afterhours::OptEntity source_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(source_id)
          .gen_first();
  app.expect_true(source_opt.has_value(), "source dish exists");

  auto &source = source_opt.asE();
  auto &drink_effects = source.addComponentIfMissing<DrinkEffects>();
  DishEffect summon_effect(TriggerHook::OnServe, EffectOperation::SummonDish,
                           TargetScope::Self, 0);
  summon_effect.summonDishType = DishType::Salmon;
  drink_effects.effects.push_back(summon_effect);

  // Count dishes before summon
  int dishes_before = afterhours::EntityQuery({.force_merge = true})
                          .whereHasComponent<IsDish>()
                          .gen_count();

  // Trigger OnServe
  auto &tq_entity = get_or_create_trigger_queue();
  auto &tq = tq_entity.get<TriggerQueue>();
  TriggerEvent serve_event;
  serve_event.hook = TriggerHook::OnServe;
  serve_event.sourceEntityId = source_id;
  tq.events.push_back(serve_event);

  app.wait_for_frames(10);

  // Count dishes after summon
  int dishes_after = afterhours::EntityQuery({.force_merge = true})
                         .whereHasComponent<IsDish>()
                         .gen_count();

  // Should have one more dish (summoned)
  app.expect_true(dishes_after == dishes_before + 1,
                  "SummonDish created new dish entity");

  // Check that summoned dish exists and has correct type
  bool found_summoned = false;
  for (auto &ref : afterhours::EntityQuery({.force_merge = true})
                       .whereHasComponent<IsDish>()
                       .gen()) {
    if (ref.get().id != source_id) {
      const auto &dish = ref.get().get<IsDish>();
      if (dish.type == DishType::Salmon) {
        found_summoned = true;
        // Check it has DishBattleState
        app.expect_true(ref.get().has<DishBattleState>(),
                        "summoned dish has DishBattleState");
        break;
      }
    }
  }
  app.expect_true(found_summoned, "summoned dish exists with correct type");

  log_info("SUMMON_TEST: SummonDish creates valid battle entity PASSED");
}

} // namespace ValidateSummonDishTestHelpers

REGISTER_TEST(validate_summon_dish_creates_entity,
              ValidateSummonDishTestHelpers::test_summon_dish_creates_entity);
