#pragma once

#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../utils/battle_fingerprint.h"
#include <afterhours/ah.h>

struct AdvanceCourseSystem : afterhours::System<CombatQueue> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Battle) {
      return false;
    }
    if (isReplayPaused()) {
      return false;
    }
    return true;
  }

  void for_each_with(afterhours::Entity &, CombatQueue &cq, float) override {
    if (cq.complete) {
      return;
    }

    if (both_dishes_finished()) {
      log_info("COMBAT: Course {} finished (both dishes at index 0)",
               cq.current_index);

      if (auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
          tq.get().has<TriggerQueue>()) {
        auto &queue = tq.get().get<TriggerQueue>();
        queue.add_event(TriggerHook::OnCourseComplete, 0, 0,
                        DishBattleState::TeamSide::Player);
        log_info("COMBAT: Fired OnCourseComplete trigger for index 0");
      }

      uint64_t fp = BattleFingerprint::compute();
      log_info("AUDIT_FP checkpoint=course_{} hash={}", cq.current_index, fp);

      reorganize_queues();

      bool has_remaining_dishes = false;
      for (DishBattleState::TeamSide side :
           {DishBattleState::TeamSide::Player,
            DishBattleState::TeamSide::Opponent}) {
        afterhours::RefEntities active =
            EQ({.ignore_temp_warning = true})
                .whereHasComponent<DishBattleState>()
                .whereLambda([side](const afterhours::Entity &e) {
                  const DishBattleState &dbs = e.get<DishBattleState>();
                  return dbs.team_side == side &&
                         dbs.phase != DishBattleState::Phase::Finished;
                })
                .gen();
        if (!active.empty()) {
          has_remaining_dishes = true;
          break;
        }
      }

      if (!has_remaining_dishes) {
        cq.complete = true;
        log_info("COMBAT: All courses complete - no remaining dishes");
      } else if (cq.current_index >= cq.total_courses - 1) {
        cq.complete = true;
        log_info("COMBAT: All courses complete - reached max courses");
      } else {
        cq.current_index++;
        log_info("COMBAT: Advancing to course {}", cq.current_index);
      }
    }
  }

private:
  bool both_dishes_finished() {
    afterhours::OptEntity player_dish =
        EQ({.ignore_temp_warning = true})
            .whereHasComponent<DishBattleState>()
            .whereInSlotIndex(0)
            .whereTeamSide(DishBattleState::TeamSide::Player)
            .gen_first();

    afterhours::OptEntity opponent_dish =
        EQ({.ignore_temp_warning = true})
            .whereHasComponent<DishBattleState>()
            .whereInSlotIndex(0)
            .whereTeamSide(DishBattleState::TeamSide::Opponent)
            .gen_first();

    bool player_finished = player_dish.has_value() &&
                           player_dish.value()->get<DishBattleState>().phase ==
                               DishBattleState::Phase::Finished;
    bool opponent_finished =
        opponent_dish.has_value() &&
        opponent_dish.value()->get<DishBattleState>().phase ==
            DishBattleState::Phase::Finished;

    return player_finished && opponent_finished;
  }

  void reorganize_queues() {
    for (DishBattleState::TeamSide side :
         {DishBattleState::TeamSide::Player,
          DishBattleState::TeamSide::Opponent}) {
      afterhours::RefEntities active_dishes =
          EQ({.ignore_temp_warning = true})
              .whereHasComponent<DishBattleState>()
              .whereTeamSide(side)
              .whereLambda([](const afterhours::Entity &e) {
                const DishBattleState &dbs = e.get<DishBattleState>();
                return dbs.phase == DishBattleState::Phase::InQueue ||
                       dbs.phase == DishBattleState::Phase::Entering ||
                       dbs.phase == DishBattleState::Phase::InCombat;
              })
              .orderByLambda(
                  [](const afterhours::Entity &a, const afterhours::Entity &b) {
                    return a.get<DishBattleState>().queue_index <
                           b.get<DishBattleState>().queue_index;
                  })
              .gen();

      int new_index = 0;
      for (afterhours::Entity &dish : active_dishes) {
        DishBattleState &dbs = dish.get<DishBattleState>();
        bool was_reorganized = dbs.queue_index != new_index;
        if (was_reorganized) {
          log_info("COMBAT: Reorganizing {} side dish {} from index {} to {}",
                   side == DishBattleState::TeamSide::Player ? "Player"
                                                             : "Opponent",
                   dish.id, dbs.queue_index, new_index);
          dbs.queue_index = new_index;
        }

        if (dbs.phase == DishBattleState::Phase::InCombat ||
            dbs.phase == DishBattleState::Phase::Entering) {
          dbs.phase = DishBattleState::Phase::InQueue;
          dbs.enter_progress = 0.0f;
        }

        new_index++;
      }
    }
  }
};
