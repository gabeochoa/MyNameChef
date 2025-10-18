#pragma once

#include "../components/battle_team_tags.h"
#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../components/transform.h"
#include "../components/trigger_animation_state.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/animation.h>
#include <algorithm>

enum struct BattleAnimKey : size_t { SlideIn };

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
      // Increment gate and schedule decrement on completion of the slowest
      // track
      auto gate =
          afterhours::EntityHelper::get_singleton<TriggerAnimationState>();
      if (!gate.get().has<TriggerAnimationState>()) {
        auto &ent = afterhours::EntityHelper::createEntity();
        ent.addComponent<TriggerAnimationState>();
        afterhours::EntityHelper::registerSingleton<TriggerAnimationState>(ent);
        gate = afterhours::EntityHelper::get_singleton<TriggerAnimationState>();
      }
      auto &state = gate.get().get<TriggerAnimationState>();
      state.active += 1;
      state.running = state.active > 0;

      // Use a dedicated manager track (non-one_shot) to represent the slide-in
      // window The per-entity slide-in is 0.18 + 0.08 = 0.26s; hold for ~0.27s.
      afterhours::animation::anim(BattleAnimKey::SlideIn, /*index=*/0)
          .sequence({{.to_value = 1.0f,
                      .duration = 0.27f,
                      .easing = afterhours::animation::EasingType::Hold}})
          .on_complete([&state]() mutable {
            state.active = std::max(0, state.active - 1);
            state.running = state.active > 0;
          });
    }

    started = true;
  }
};
