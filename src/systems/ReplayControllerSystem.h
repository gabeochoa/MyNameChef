#pragma once

#include "../components/battle_load_request.h"
#include "../components/battle_session_registry.h"
#include "../components/battle_session_tag.h"
#include "../components/battle_team_tags.h"
#include "../components/combat_queue.h"
#include "../components/replay_state.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include "../log.h"
#include "../query.h"
#include "../render_backend.h"
#include "../rl.h"
#include "../shop.h"
#include <afterhours/ah.h>
#include <cstdint>

struct ReplayControllerSystem : afterhours::System<ReplayState> {
  static constexpr float kTickMs = 150.0f / 1000.0f;

  virtual bool should_run(float) override {
    if (render_backend::is_headless_mode) {
      return false;
    }
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Battle) {
      return false;
    }
    return true;
  }

  void for_each_with(afterhours::Entity &, ReplayState &rs, float dt) override {
    handle_inputs(rs);

    // When paused, battle systems will check isReplayPaused() and not run
    if (rs.paused) {
      return;
    }

    rs.targetMs += static_cast<int64_t>(dt * 1000.0f * rs.timeScale);
    rs.currentFrame++; // Increment frame counter for progress tracking
  }

private:
  void handle_inputs(ReplayState &rs) {
    if (raylib::IsKeyPressed(raylib::KEY_SPACE)) {
      rs.paused = !rs.paused;
      log_info("REPLAY_PAUSE {}", rs.paused);
    }

    if (raylib::IsKeyPressed(raylib::KEY_R)) {
      restart_replay(rs);
    }

    if (raylib::IsKeyPressed(raylib::KEY_ONE) ||
        raylib::IsKeyPressed(raylib::KEY_KP_1)) {
      rs.timeScale = 0.5f;
      log_info("REPLAY_SPEED {}", rs.timeScale);
    }

    if (raylib::IsKeyPressed(raylib::KEY_TWO) ||
        raylib::IsKeyPressed(raylib::KEY_KP_2)) {
      rs.timeScale = 1.0f;
      log_info("REPLAY_SPEED {}", rs.timeScale);
    }

    if (raylib::IsKeyPressed(raylib::KEY_THREE) ||
        raylib::IsKeyPressed(raylib::KEY_KP_3)) {
      rs.timeScale = 2.0f;
      log_info("REPLAY_SPEED {}", rs.timeScale);
    }

    if (raylib::IsKeyPressed(raylib::KEY_FOUR) ||
        raylib::IsKeyPressed(raylib::KEY_KP_4)) {
      rs.timeScale = 4.0f;
      log_info("REPLAY_SPEED {}", rs.timeScale);
    }
  }

  void restart_replay(ReplayState &rs) {
    log_info("REPLAY_RESTART seed={} playerJson={} opponentJson={}", rs.seed,
             rs.playerJsonPath, rs.opponentJsonPath);

    auto registry =
        afterhours::EntityHelper::get_singleton<BattleSessionRegistry>();
    if (registry.get().has<BattleSessionRegistry>()) {
      BattleSessionRegistry &reg = registry.get().get<BattleSessionRegistry>();
      uint32_t oldSession = reg.sessionId;

      int cleanup_count = 0;
      for (int entityId : reg.ownedEntityIds) {
        if (auto opt =
                afterhours::EntityQuery().whereID(entityId).gen_first()) {
          afterhours::Entity &e = *opt.value();
          if (e.has<BattleSessionTag>()) {
            const BattleSessionTag &tag = e.get<BattleSessionTag>();
            if (tag.sessionId == oldSession) {
              e.cleanup = true;
              cleanup_count++;
            }
          }
        }
      }

      log_info("REPLAY_CLEANUP session={} entities={}", oldSession,
               cleanup_count);

      reg.reset();
      reg.sessionId++;
    }

    auto battleRequest =
        afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    if (battleRequest.get().has<BattleLoadRequest>()) {
      BattleLoadRequest &request = battleRequest.get().get<BattleLoadRequest>();
      request.playerJsonPath = rs.playerJsonPath;
      request.opponentJsonPath = rs.opponentJsonPath;
      request.loaded = false;
    }

    afterhours::EntityHelper::merge_entity_arrays();

    auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
    if (tq.get().has<TriggerQueue>()) {
      tq.get().get<TriggerQueue>().clear();
    }

    log_info("REPLAY_INIT seed={} playerJson={} opponentJson={}", rs.seed,
             rs.playerJsonPath, rs.opponentJsonPath);

    // Reset frame counter on restart
    rs.currentFrame = 0;
  }
};
