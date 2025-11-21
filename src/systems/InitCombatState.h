#pragma once

#include "../components/animation_event.h"
#include "../components/battle_load_request.h"
#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../components/pending_combat_mods.h"
#include "../components/replay_state.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../shop.h"
#include "../systems/SimplifiedOnServeSystem.h"
#include "../utils/battle_fingerprint.h"
#include <afterhours/ah.h>

struct InitCombatState : afterhours::System<CombatQueue> {
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;
  std::string last_battle_path = ""; // Track last battle path to detect new battles
  bool last_initialized = false; // Track if we've initialized for the current battle path

  virtual bool should_run(float) override {
    GameStateManager &gsm = GameStateManager::get();
    
    // CRITICAL: Clear last_battle_path and initialization flag when leaving Results screen (battle finished)
    // This ensures next battle is detected as new even if path is the same
    // We clear when leaving Results because that's when we know a battle has fully finished
    if (last_screen == GameStateManager::Screen::Results && 
        gsm.active_screen != GameStateManager::Screen::Results) {
      last_battle_path = "";
      last_initialized = false;
    }
    
    // Also clear when leaving Battle screen (transitioning to Results or Shop)
    // This ensures that when we come back to Battle, it's treated as a new battle
    if (last_screen == GameStateManager::Screen::Battle && 
        gsm.active_screen != GameStateManager::Screen::Battle) {
      last_initialized = false;
      // Don't clear last_battle_path here - we want to keep it to detect if it's the same battle
      // But we clear it when leaving Results (above) to ensure next battle is detected as new
    }
    
    bool entering_battle =
        last_screen != GameStateManager::Screen::Battle &&
        gsm.active_screen == GameStateManager::Screen::Battle;
    
    // Also check if a new battle is starting (new BattleLoadRequest loaded)
    // CRITICAL: We need to run InitCombatState for Battle 2 even if paths match
    // because animation events from Battle 1 need to be cleaned up
    bool new_battle_starting = false;
    if (gsm.active_screen == GameStateManager::Screen::Battle && !entering_battle) {
      auto battleRequest = afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
      if (battleRequest.get().has<BattleLoadRequest>()) {
        const BattleLoadRequest &request = battleRequest.get().get<BattleLoadRequest>();
        if (request.loaded && !request.playerJsonPath.empty()) {
          // CRITICAL: Check if there are blocking animation events that need cleanup
          // BUT only if we haven't initialized yet - once initialized, new animations are expected
          bool has_blocking_animations = false;
          if (!last_initialized) {
            // Only check for blocking animations if we haven't initialized yet
            // This prevents running InitCombatState repeatedly when it creates new animations
            if (afterhours::EntityQuery()
                    .whereHasComponent<IsBlockingAnimationEvent>()
                    .has_values()) {
              has_blocking_animations = true;
            }
          }
          
          // CRITICAL: If last_battle_path is empty, this is definitely a new battle
          // OR if paths are different, it's a new battle
          // OR if paths are the same BUT we haven't initialized yet (last_initialized is false)
          // OR if there are blocking animations from a previous battle that need cleanup (and we haven't initialized)
          bool is_new_battle = last_battle_path.empty() || 
                            (last_battle_path != request.playerJsonPath) ||
                            !last_initialized ||
                            has_blocking_animations;
          
                      if (is_new_battle) {
                        new_battle_starting = true;
                      }
        }
      }
    }
    
    if (entering_battle) {
      // When entering battle, reset initialization flag so we can initialize
      // This ensures that even if paths match, we'll initialize for the new battle
      last_initialized = false;
      // Update last_battle_path when entering battle
      auto battleRequest = afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
      if (battleRequest.get().has<BattleLoadRequest>()) {
        const BattleLoadRequest &request = battleRequest.get().get<BattleLoadRequest>();
        if (request.loaded) {
          last_battle_path = request.playerJsonPath;
        }
      }
    }
    
    last_screen = gsm.active_screen;
    return entering_battle || new_battle_starting;
  }

  void for_each_with(afterhours::Entity &, CombatQueue &cq, float) override {
    // Update last_battle_path after detecting new battle
    // CRITICAL: Only update if this is actually a new battle (path changed or was empty)
    auto battleRequest = afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    if (battleRequest.get().has<BattleLoadRequest>()) {
      const BattleLoadRequest &request = battleRequest.get().get<BattleLoadRequest>();
      if (request.loaded && !request.playerJsonPath.empty()) {
        // Update last_battle_path if it changed or was empty
        // Also mark as initialized for this battle
        if (last_battle_path != request.playerJsonPath) {
          last_battle_path = request.playerJsonPath;
        }
        last_initialized = true;
      }
    }
    
    // CRITICAL: Clean up leftover animation events from previous battle
    // These can block ResolveCombatTickSystem if not cleared
    // Do this FIRST, before any other operations
    // ALWAYS clean up, even if this is the "same" battle (paths match)
    // because animation events from Battle 1 can persist and block Battle 2
    int animation_events_cleaned = 0;
    for (afterhours::Entity &e :
         afterhours::EntityQuery()
             .whereHasComponent<IsBlockingAnimationEvent>()
             .gen()) {
      e.cleanup = true;
      animation_events_cleaned++;
    }
    // Cleanup will happen automatically when system loop runs
    if (animation_events_cleaned > 0) {
      log_info("COMBAT: Marked {} animation events for cleanup", animation_events_cleaned);
    }
    
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
      
      // CRITICAL: Clear PendingCombatMods from previous battle
      // These can trigger StatBoost animations that block StartCourseSystem
      if (e.has<PendingCombatMods>()) {
        e.removeComponent<PendingCombatMods>();
      }
      
      dish_count++;
    }

    log_info("COMBAT: Initialized combat state - {} courses, {} dishes in queue",
             cq.total_courses, dish_count);

    // CRITICAL: Clear OnServeState so SimplifiedOnServeSystem can fire OnServe for new battle
    // OnServeState persists between battles and has allFired=true, which prevents OnServe from firing
    for (afterhours::Entity &e :
         afterhours::EntityQuery()
             .whereHasComponent<OnServeState>()
             .gen()) {
      e.removeComponent<OnServeState>();
    }

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
