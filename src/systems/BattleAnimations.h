#pragma once

#include "../components/animation_event.h"
#include "../components/battle_anim_keys.h"
#include "../components/battle_team_tags.h"
#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../shop.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/animation.h>

struct TriggerBattleSlideIn : afterhours::System<> {
  bool started = false;
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    if (last_screen == GameStateManager::Screen::Battle &&
        gsm.active_screen != GameStateManager::Screen::Battle) {
      started = false;
    }
    last_screen = gsm.active_screen;
    return gsm.active_screen == GameStateManager::Screen::Battle && !started;
  }

  void once(float) override {
    // Mark animations as running (increment gate) if any slide-ins will start
    int started_count = 0;
    for (auto &ref : afterhours::EntityQuery()
                         .whereHasComponent<IsDish>()
                         .whereHasComponent<IsPlayerTeamItem>()
                         .whereHasComponent<Transform>()
                         .whereHasComponent<DishBattleState>()
                         .whereLambda([&](const afterhours::Entity &e) {
                           const auto &dbs = e.get<DishBattleState>();
                           return dbs.phase == DishBattleState::Phase::InQueue;
                         })
                         .gen()) {
      auto &e = ref.get();
      size_t id = static_cast<size_t>(e.id);
      started_count++;
      afterhours::animation::one_shot(BattleAnimKey::SlideIn, id, [](auto h) {
        h.from(0.0f).sequence({
            {.to_value = 1.1f,
             .duration = 0.18f,
             .easing = afterhours::animation::EasingType::EaseOutQuad},
            {.to_value = 1.0f,
             .duration = 0.08f,
             .easing = afterhours::animation::EasingType::EaseOutQuad},
        });
      });
    }

    for (auto &ref : afterhours::EntityQuery()
                         .whereHasComponent<IsDish>()
                         .whereHasComponent<IsOpponentTeamItem>()
                         .whereHasComponent<Transform>()
                         .whereHasComponent<DishBattleState>()
                         .whereLambda([&](const afterhours::Entity &e) {
                           const auto &dbs = e.get<DishBattleState>();
                           return dbs.phase == DishBattleState::Phase::InQueue;
                         })
                         .gen()) {
      auto &e = ref.get();
      size_t id = static_cast<size_t>(e.id);
      started_count++;
      afterhours::animation::one_shot(BattleAnimKey::SlideIn, id, [](auto h) {
        h.from(0.0f).sequence({
            {.to_value = 1.1f,
             .duration = 0.18f,
             .easing = afterhours::animation::EasingType::EaseOutQuad},
            {.to_value = 1.0f,
             .duration = 0.08f,
             .easing = afterhours::animation::EasingType::EaseOutQuad},
        });
      });
    }

    if (started_count > 0) {
      // Enqueue a blocking SlideIn animation event
      log_info("ANIMATION: Creating SlideIn animation event for {} dishes",
               started_count);
      make_animation_event(AnimationEventType::SlideIn, true);
    }

    started = true;
  }
};
