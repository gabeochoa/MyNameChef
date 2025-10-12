#pragma once

#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
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
    js.post_complete_elapsed = 0.0f;
    js.transitioned = false;
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

    // 1) advance in-flight presentations
    const float present_duration = 0.45f; // seconds
    int num_presenting = 0;
    for (auto &ref :
         afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
      auto &e = ref.get();
      auto &s = e.get<DishBattleState>();
      if (s.phase == DishBattleState::Phase::Presenting) {
        num_presenting++;
        s.phase_progress =
            std::min(1.0f, s.phase_progress + dt / present_duration);
        if (s.phase_progress >= 1.0f) {
          log_info(
              "JUDGING: Entity {} finished presenting, moving to Judged phase",
              e.id);
          s.phase = DishBattleState::Phase::Judged;
          // Increment live totals when a dish finishes judging
          if (e.has<IsDish>()) {
            const auto &dish = e.get<IsDish>();
            DishInfo info = get_dish_info(dish.type);
            const FlavorStats &flavor = info.flavor;
            const int dishScore = flavor.satiety + flavor.sweetness + flavor.spice +
                                  flavor.acidity + flavor.umami + flavor.richness +
                                  flavor.freshness;
            auto jsEnt = afterhours::EntityHelper::get_singleton<JudgingState>();
            if (jsEnt.get().has<JudgingState>()) {
              auto &js_state = jsEnt.get().get<JudgingState>();
              if (s.team_side == DishBattleState::TeamSide::Player) {
                js_state.player_total += dishScore;
                // debug log removed
              } else {
                js_state.opponent_total += dishScore;
                // debug log removed
              }
            }
          }
        }
      }
    }

    // 2) if no one is presenting, after delay pick next per side and start
    if (num_presenting == 0) {
      js.timer += dt;
      if (js.timer >= js.per_course_delay) {
        js.timer = 0.0f;
        auto pick_and_start = [&](DishBattleState::TeamSide side) {
          auto first = afterhours::EntityQuery()
                           .whereHasComponent<DishBattleState>()
                           .whereLambda([&](const afterhours::Entity &e) {
                             const auto &s = e.get<DishBattleState>();
                             return s.team_side == side &&
                                    s.phase == DishBattleState::Phase::InQueue;
                           })
                           .orderByLambda([](const afterhours::Entity &a,
                                             const afterhours::Entity &b) {
                             return a.get<DishBattleState>().queue_index <
                                    b.get<DishBattleState>().queue_index;
                           })
                           .gen_first();
          if (first) {
            auto &s = first->get<DishBattleState>();
            log_info("JUDGING: Starting presentation for entity {} - Side: {}, "
                     "Slot: {}",
                     first->id,
                     side == DishBattleState::TeamSide::Player ? "Player"
                                                               : "Opponent",
                     s.queue_index);
            s.phase = DishBattleState::Phase::Presenting;
            s.phase_progress = 0.0f;
            return true;
          }
          return false;
        };
        bool started = false;
        started |= pick_and_start(DishBattleState::TeamSide::Player);
        started |= pick_and_start(DishBattleState::TeamSide::Opponent);
        if (!started) {
          js.complete = true;
          js.post_complete_elapsed = 0.0f;
        }
      }
    }
  }
};
