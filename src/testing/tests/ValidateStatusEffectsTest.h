#pragma once

#include "../../components/combat_stats.h"
#include "../../components/status_effects.h"
#include "../../game_state_manager.h"
#include "../test_macros.h"

TEST(validate_status_effects_fatigue) {
  GameStateManager::get().to_battle();
  app.wait_for_frames(1);

  app.run_once("status_create", [&app]() {
    auto garlic_id = app.create_dish(DishType::GarlicBread)
                         .on_team(DishBattleState::TeamSide::Player)
                         .at_slot(0)
                         .in_phase(DishBattleState::Phase::Entering)
                         .commit();
    auto opponent_id = app.create_dish(DishType::FrenchFries)
                         .on_team(DishBattleState::TeamSide::Opponent)
                         .at_slot(0)
                         .in_phase(DishBattleState::Phase::InQueue)
                         .commit();
    app.set_test_int("status_garlic_id", static_cast<int>(garlic_id));
    app.set_test_int("status_opponent_id", static_cast<int>(opponent_id));
  });
  app.wait_for_frames(1);

  app.run_once("status_fire_once", [&app]() {
    auto garlic_id = static_cast<afterhours::EntityID>(
        app.get_test_int("status_garlic_id").value());
    app.fire_trigger(TriggerHook::OnServe, garlic_id, 0,
                     DishBattleState::TeamSide::Player);
  });
  app.wait_for_frames(2);

  app.run_once("status_verify_once", [&app]() {
    auto opponent_id = static_cast<afterhours::EntityID>(
        app.get_test_int("status_opponent_id").value());
    auto *opponent = app.find_entity_by_id(opponent_id);
    app.expect_true(opponent != nullptr, "opponent dish still exists");
    if (opponent != nullptr) {
      app.expect_true(opponent->has<StatusEffects>(),
                      "opponent has StatusEffects after GarlicBread OnServe");
      if (opponent->has<StatusEffects>()) {
        const auto &effects = opponent->get<StatusEffects>().effects;
        app.expect_eq(static_cast<int>(effects.size()), 1,
                      "one status effect after first OnServe");
        if (!effects.empty()) {
          app.expect_true(effects[0].type == StatusEffectType::Fatigue,
                          "applied effect is Fatigue");
          app.expect_eq(effects[0].zingDelta, 0,
                        "Fatigue has no zing delta");
          app.expect_eq(effects[0].bodyDelta, -1,
                        "Fatigue reduces Body by 1");
          app.expect_eq(effects[0].courseApplied, 0,
                        "Fatigue recorded at course 0");
        }
      }
      app.expect_true(opponent->has<CombatStats>(), "opponent has CombatStats");
      if (opponent->has<CombatStats>()) {
        app.expect_eq(opponent->get<CombatStats>().baseBody, 1,
                      "Fatigue reduces opponent base Body from 2 to 1");
      }
    }
  });

  app.run_once("status_fire_twice", [&app]() {
    auto garlic_id = static_cast<afterhours::EntityID>(
        app.get_test_int("status_garlic_id").value());
    app.fire_trigger(TriggerHook::OnServe, garlic_id, 0,
                     DishBattleState::TeamSide::Player);
  });
  app.wait_for_frames(2);

  app.run_once("status_verify_twice", [&app]() {
    auto opponent_id = static_cast<afterhours::EntityID>(
        app.get_test_int("status_opponent_id").value());
    auto *opponent = app.find_entity_by_id(opponent_id);
    app.expect_true(opponent != nullptr, "opponent dish still exists");
    if (opponent != nullptr) {
      if (opponent->has<StatusEffects>()) {
        const auto &effects = opponent->get<StatusEffects>().effects;
        app.expect_eq(static_cast<int>(effects.size()), 2,
                      "status effects stack on re-application");
      }
      if (opponent->has<CombatStats>()) {
        app.expect_eq(opponent->get<CombatStats>().baseBody, 0,
                      "stacked Fatigue reduces opponent base Body to 0");
      }
    }
  });
}
