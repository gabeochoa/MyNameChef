#pragma once

#include "../../components/battle_load_request.h"
#include "../../components/combat_queue.h"
#include "../../components/combat_stats.h"
#include "../../components/dish_battle_state.h"
#include "../../components/status_effects.h"
#include "../../components/transform.h"
#include "../../game_state_manager.h"
#include "../../log.h"
#include "../../query.h"
#include "../test_app.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

namespace ValidateDurationEffectsTestHelpers {
static void ensure_battle_load_request_exists() {
  if (afterhours::EntityHelper::has_singleton<BattleLoadRequest>()) {
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
} // namespace ValidateDurationEffectsTestHelpers

TEST(validate_duration_effect_expires_after_two_courses) {
  log_info("TEST: Starting validate_duration_effect_expires_after_two_courses");

  app.launch_game();
  app.wait_for_frames(1);

  app.clear_battle_dishes();

  ValidateDurationEffectsTestHelpers::ensure_battle_load_request_exists();

  afterhours::EntityID garlic_id = app.create_dish(DishType::GarlicBread)
                                       .on_team(DishBattleState::TeamSide::Player)
                                       .at_slot(0)
                                       .with_combat_stats()
                                       .commit();

  afterhours::EntityID potato_one_id = app.create_dish(DishType::Potato)
                                           .on_team(DishBattleState::TeamSide::Player)
                                           .at_slot(1)
                                           .with_combat_stats()
                                           .commit();

  afterhours::EntityID potato_two_id = app.create_dish(DishType::Potato)
                                           .on_team(DishBattleState::TeamSide::Player)
                                           .at_slot(2)
                                           .with_combat_stats()
                                           .commit();

  afterhours::EntityID wagyu_id = app.create_dish(DishType::WagyuSteak)
                                      .on_team(DishBattleState::TeamSide::Opponent)
                                      .at_slot(0)
                                      .with_combat_stats()
                                      .commit();

  app.wait_for_frames(5);

  app.setup_battle();
  app.wait_for_frames(1);

  app.wait_for_battle_initialized(10.0f);
  app.wait_for_course_complete(0, 60.0f);
  app.wait_for_reorganization(5.0f);

  app.run_once("duration_after_course_0", [&app, wagyu_id]() {
    auto *wagyu = app.find_entity_by_id(wagyu_id);
    app.expect_true(wagyu != nullptr, "Wagyu still exists after course 0");
    if (wagyu != nullptr) {
      app.expect_true(wagyu->has<StatusEffects>(),
                      "Wagyu has StatusEffects after GarlicBread OnServe");
      if (wagyu->has<StatusEffects>()) {
        const auto &effects = wagyu->get<StatusEffects>().effects;
        app.expect_eq(static_cast<int>(effects.size()), 1,
                      "one status effect after course 0");
        if (!effects.empty()) {
          app.expect_true(effects[0].type == StatusEffectType::Fatigue,
                          "applied effect is Fatigue");
          app.expect_eq(effects[0].duration, 2,
                        "Fatigue lasts 2 courses");
          app.expect_eq(effects[0].courseApplied, 0,
                        "Fatigue recorded at course 0");
        }
      }
      app.expect_true(wagyu->has<CombatStats>(), "Wagyu has CombatStats");
      if (wagyu->has<CombatStats>()) {
        app.expect_eq(wagyu->get<CombatStats>().baseBody, 4,
                      "Fatigue reduces Wagyu base Body from 5 to 4");
      }
    }
  });

  app.wait_for_course_complete(1, 60.0f);
  app.wait_for_reorganization(5.0f);

  app.run_once("duration_after_course_1", [&app, wagyu_id]() {
    auto *wagyu = app.find_entity_by_id(wagyu_id);
    app.expect_true(wagyu != nullptr, "Wagyu still exists after course 1");
    if (wagyu != nullptr) {
      app.expect_true(!wagyu->has<StatusEffects>(),
                      "Fatigue expired after 2 courses");
      app.expect_true(wagyu->has<CombatStats>(), "Wagyu has CombatStats");
      if (wagyu->has<CombatStats>()) {
        app.expect_eq(wagyu->get<CombatStats>().baseBody, 5,
                      "Wagyu base Body returns to 5 after Fatigue expires");
      }
    }
  });

  app.expect_true(garlic_id != 0, "GarlicBread was created");
  app.expect_true(potato_one_id != 0, "first Potato was created");
  app.expect_true(potato_two_id != 0, "second Potato was created");
}
