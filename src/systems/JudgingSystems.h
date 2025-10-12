#pragma once

#include "../components/battle_team_tags.h"
#include "../components/is_dish.h"
#include "../components/judged.h"
#include "../components/judging_state.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

struct InitJudgingState : afterhours::System<JudgingState> {
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    bool entering_battle =
        last_screen != GameStateManager::Screen::Battle &&
        gsm.active_screen == GameStateManager::Screen::Battle;
    last_screen = gsm.active_screen;
    return entering_battle;
  }
  void for_each_with(afterhours::Entity &, JudgingState &js, float) override {
    js.current_index = -1;
    js.total_courses = 7;
    js.player_total = 0;
    js.opponent_total = 0;
    js.complete = false;
    js.timer = 0.0f;
  }
};

struct AdvanceJudging : afterhours::System<JudgingState> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Battle)
      return false;
    return true;
  }

  void for_each_with(afterhours::Entity &, JudgingState &js,
                     float dt) override {
    if (js.complete) {
      if (!js.transitioned) {
        js.post_complete_elapsed += dt;
        if (js.post_complete_elapsed >= 0.5f) {
          GameStateManager::get().to_results();
          js.transitioned = true;
        }
      }
      return;
    }

    js.timer += dt;
    if (js.timer < js.per_course_delay)
      return;
    js.timer = 0.0f;

    // Score and then remove first dish per side; entities shift visually
    bool any_scored = false;
    {
      auto first = afterhours::EntityQuery()
                       .whereHasComponent<IsPlayerTeamItem>()
                       .whereHasComponent<IsDish>()
                       .whereLambda([](const afterhours::Entity &e) {
                         return !e.has<Judged>();
                       })
                       .gen_first();
      if (first) {
        const auto &dish = first->get<IsDish>();
        DishInfo info = get_dish_info(dish.type);
        const auto &f = info.flavor;
        js.player_total += f.satiety + f.sweetness + f.spice + f.acidity +
                           f.umami + f.richness + f.freshness;
        first->addComponent<Judged>();
        any_scored = true;
      }
    }
    {
      auto first = afterhours::EntityQuery()
                       .whereHasComponent<IsOpponentTeamItem>()
                       .whereHasComponent<IsDish>()
                       .whereLambda([](const afterhours::Entity &e) {
                         return !e.has<Judged>();
                       })
                       .gen_first();
      if (first) {
        const auto &dish = first->get<IsDish>();
        DishInfo info = get_dish_info(dish.type);
        const auto &f = info.flavor;
        js.opponent_total += f.satiety + f.sweetness + f.spice + f.acidity +
                             f.umami + f.richness + f.freshness;
        first->addComponent<Judged>();
        any_scored = true;
      }
    }
    if (!any_scored) {
      js.complete = true;
      js.post_complete_elapsed = 0.0f;
    }

    js.current_index++;
    if (js.current_index >= js.total_courses) {
      js.complete = true;
      js.post_complete_elapsed = 0.0f;
    }
  }
};
