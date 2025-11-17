#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/combat_stats.h"
#include "../../components/dish_battle_state.h"
#include "../../components/is_dish.h"
#include "../../components/status_effects.h"
#include "../../components/trigger_event.h"
#include "../../components/trigger_queue.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../query.h"
#include "../test_app.h"
#include "../test_macros.h"

namespace ValidateWagyuSteakTestHelpers {

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

static void test_wagyu_steak_applies_debuff(TestApp &app) {
  log_info(
      "WAGYU_TEST: Testing Wagyu Steak applies -1 Zing debuff to opponent");

  ensure_battle_load_request_exists();
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  // Create Wagyu Steak dish
  auto wagyu_id = app.create_dish(DishType::WagyuSteak)
                      .on_team(DishBattleState::TeamSide::Player)
                      .at_slot(0)
                      .in_phase(DishBattleState::Phase::InCombat)
                      .commit();

  // Create opponent dish
  auto opponent_id = app.create_dish(DishType::Potato)
                         .on_team(DishBattleState::TeamSide::Opponent)
                         .at_slot(0)
                         .in_phase(DishBattleState::Phase::InCombat)
                         .commit();

  app.wait_for_frames(5);

  // Get initial opponent stats
  afterhours::OptEntity opponent_before_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(opponent_id)
          .gen_first();
  app.expect_true(opponent_before_opt.has_value(), "opponent exists");

  int initial_zing = 0;
  if (opponent_before_opt.asE().has<CombatStats>()) {
    initial_zing = opponent_before_opt.asE().get<CombatStats>().baseZing;
  }

  // Trigger OnBiteTaken on Wagyu (when it takes damage)
  auto &tq_entity = get_or_create_trigger_queue();
  auto &tq = tq_entity.get<TriggerQueue>();
  TriggerEvent bite_event;
  bite_event.hook = TriggerHook::OnBiteTaken;
  bite_event.sourceEntityId = wagyu_id;
  tq.events.push_back(bite_event);

  app.wait_for_frames(10);

  // Check that opponent has StatusEffects with -1 Zing
  afterhours::OptEntity opponent_after_opt =
      afterhours::EntityQuery({.force_merge = true})
          .whereID(opponent_id)
          .gen_first();
  app.expect_true(opponent_after_opt.has_value(), "opponent still exists");

  bool has_status = opponent_after_opt.asE().has<StatusEffects>();
  app.expect_true(has_status,
                  "opponent has StatusEffects after Wagyu takes damage");

  if (has_status) {
    const auto &status = opponent_after_opt.asE().get<StatusEffects>();
    bool found_debuff = false;
    for (const auto &effect : status.effects) {
      if (effect.zingDelta == -1) {
        found_debuff = true;
        break;
      }
    }
    app.expect_true(found_debuff,
                    "opponent has -1 Zing debuff from Wagyu Steak");
  }

  log_info("WAGYU_TEST: Wagyu Steak applies debuff PASSED");
}

} // namespace ValidateWagyuSteakTestHelpers

REGISTER_TEST(validate_wagyu_steak_applies_debuff,
              ValidateWagyuSteakTestHelpers::test_wagyu_steak_applies_debuff);
