#pragma once

#include "../components/animation_event.h"
#include "../components/battle_load_request.h"
#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../components/replay_state.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../shop.h"
#include "../utils/battle_fingerprint.h"
#include <afterhours/ah.h>

struct InitCombatState : afterhours::System<CombatQueue> {
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    GameStateManager &gsm = GameStateManager::get();
    bool entering_battle =
        last_screen != GameStateManager::Screen::Battle &&
        gsm.active_screen == GameStateManager::Screen::Battle;
    last_screen = gsm.active_screen;
    return entering_battle;
  }

  void for_each_with(afterhours::Entity &, CombatQueue &cq, float) override {
    // Reset combat queue
    cq.reset();

    log_info("COMBAT: Reset combat queue - current_index: {}, total_courses: "
             "{}, complete: {}",
             cq.current_index, cq.total_courses, cq.complete);

    // Set all dishes to InQueue phase
    int dish_count = 0;
    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
      DishBattleState &dbs = e.get<DishBattleState>();
      dbs.phase = DishBattleState::Phase::InQueue;
      dbs.enter_progress = 0.0f;
      dbs.bite_timer = 0.0f;
      dbs.onserve_fired = false;
      dish_count++;

      log_info(
          "COMBAT: Reset dish {} - team: {}, slot: {}, phase: InQueue", e.id,
          (dbs.team_side == DishBattleState::TeamSide::Player) ? "Player"
                                                               : "Opponent",
          dbs.queue_index);
    }

    log_info(
        "COMBAT: Initialized combat state - {} courses, {} dishes in queue",
        cq.total_courses, dish_count);

    log_info("COMBAT: Creating SlideIn animation at battle start");
    make_animation_event(AnimationEventType::SlideIn, true);

    if (auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
        tq.get().has<TriggerQueue>()) {
      auto &queue = tq.get().get<TriggerQueue>();
      queue.add_event(TriggerHook::OnStartBattle, 0, 0,
                      DishBattleState::TeamSide::Player);
      log_info("COMBAT: Fired OnStartBattle trigger");
    }

    capture_replay_snapshot();

    uint64_t fp = BattleFingerprint::compute();
    log_info("AUDIT_FP checkpoint=start hash={}", fp);
  }

private:
  void capture_replay_snapshot() {
    auto replayState = afterhours::EntityHelper::get_singleton<ReplayState>();
    if (!replayState.get().has<ReplayState>()) {
      return;
    }

    ReplayState &rs = replayState.get().get<ReplayState>();

    auto battleRequest =
        afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    if (battleRequest.get().has<BattleLoadRequest>()) {
      const BattleLoadRequest &request =
          battleRequest.get().get<BattleLoadRequest>();
      rs.playerJsonPath = request.playerJsonPath;
      rs.opponentJsonPath = request.opponentJsonPath;
      // TODO: Replay state will come from the server simulation results
      // Server runs faster than client, so we need to capture:
      // - seed (already deterministic)
      // - totalFrames (from server simulation completion)
      // - Any other state needed for accurate replay
      rs.seed = 1234567890;
      rs.active = true;
      rs.paused = false;
      rs.timeScale = 1.0f;
      rs.clockMs = 0;
      rs.targetMs = 0;
      rs.currentFrame = 0;
      rs.totalFrames = 0; // Will be set from server simulation results

      log_info("REPLAY_INIT seed={} playerJson={} opponentJson={}", rs.seed,
               rs.playerJsonPath, rs.opponentJsonPath);
    }
  }
};
