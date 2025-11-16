#pragma once

#include "../components/battle_load_request.h"
#include "../components/battle_processor.h"
#include "../game_state_manager.h"
#include "../shop.h"
#include <afterhours/ah.h>

struct BattleProcessorSystem : afterhours::System<BattleProcessor> {
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;
  std::string last_battle_player_path =
      ""; // Track last battle's player path to detect new battles
  std::string current_started_battle_path =
      ""; // Track which battle we've already called startBattle for

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();

    // Always update last_screen to track screen transitions accurately
    // This ensures we can detect entering battle even after being on other
    // screens
    last_screen = gsm.active_screen;

    if (gsm.active_screen != GameStateManager::Screen::Battle) {
      static int skip_count = 0;
      skip_count++;
      if (skip_count % 60 == 0) {
        log_info(
            "BATTLE_PROCESSOR_SHOULD_RUN: Screen is not Battle - screen={}",
            (int)gsm.active_screen);
      }
      return false;
    }

    auto battleProcessor =
        afterhours::EntityHelper::get_singleton<BattleProcessor>();
    if (!battleProcessor.get().has<BattleProcessor>()) {
      static int no_processor_count = 0;
      no_processor_count++;
      if (no_processor_count % 60 == 0) {
        log_info("BATTLE_PROCESSOR_SHOULD_RUN: No BattleProcessor singleton");
      }
      return false;
    }

    bool has_anim = hasActiveAnimation();
    return !has_anim;
  }

  void for_each_with(afterhours::Entity &, BattleProcessor &processor,
                     float dt) override {
    auto &gsm = GameStateManager::get();

    // CRITICAL: If we have a loaded request, we MUST start a new battle
    // Even if isBattleActive() is true, if we have a new loaded request, the
    // old battle should be finished
    auto battleRequest =
        afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    bool should_start_new_battle = false;

    if (battleRequest.get().has<BattleLoadRequest>()) {
      auto &request = battleRequest.get().get<BattleLoadRequest>();
      if (request.loaded) {
        // Check if this is a NEW battle (different player path)
        bool is_new_battle =
            (last_battle_player_path != request.playerJsonPath);

        log_info(
            "BATTLE_PROCESSOR: Battle check - last_path='{}', new_path='{}', "
            "is_new={}, isBattleActive={}, simulationStarted={}",
            last_battle_player_path, request.playerJsonPath, is_new_battle,
            processor.isBattleActive(), processor.simulationStarted);

        if (!processor.isBattleActive()) {
          // No active battle - check if we've already started this battle
          // Only start if we haven't already started it (check
          // current_started_battle_path) CRITICAL: If
          // current_started_battle_path matches, we've already called
          // startBattle for this battle Even if isBattleActive is false (maybe
          // finishBattle was called), we shouldn't call startBattle again
          bool already_started =
              (current_started_battle_path == request.playerJsonPath);

          if (already_started) {
            log_info("BATTLE_PROCESSOR: Battle already started (no active "
                     "battle but path matches), skipping - started_path='{}', "
                     "current_path='{}', simulationStarted={}",
                     current_started_battle_path, request.playerJsonPath,
                     processor.simulationStarted ? 1 : 0);
            should_start_new_battle = false;
          } else {
            // No active battle and haven't started this one - start it
            bool was_started = processor.simulationStarted;
            processor.simulationStarted = false;
            should_start_new_battle = true;

            if (was_started) {
              log_info("BATTLE_PROCESSOR: Resetting simulationStarted for new "
                       "battle (previous battle finished)");
            } else {
              log_info("BATTLE_PROCESSOR: Ready to start new battle - "
                       "loaded={}, isBattleActive={}, simulationStarted={}",
                       request.loaded, processor.isBattleActive(),
                       processor.simulationStarted);
            }
          }
        } else if (processor.isBattleActive()) {
          // CRITICAL: Battle is still active but we have a loaded request
          // Check if this is actually a NEW battle (different path) or the same
          // battle Only finish and restart if it's a different battle
          bool is_different_battle =
              (last_battle_player_path != request.playerJsonPath &&
               !last_battle_player_path.empty());

          if (is_different_battle) {
            // Different battle - finish old one and start new one
            log_info(
                "BATTLE_PROCESSOR: Battle active but NEW battle requested - "
                "finishing old battle and starting new one. last_path='{}', "
                "new_path='{}', isBattleActive={}, finished={}",
                last_battle_player_path, request.playerJsonPath,
                processor.isBattleActive() ? 1 : 0, processor.finished ? 1 : 0);

            // Only call finishBattle if battle is actually active and not
            // already finished
            // CRITICAL: Also check if simulationComplete - if battle already
            // finished naturally, we should just clear activeBattle instead of
            // calling finishBattle again
            // CRITICAL: If simulationComplete is true, the battle already
            // finished and finishBattle() was already called. Don't call it
            // again.
            if (processor.isBattleActive() && !processor.finished &&
                !processor.simulationComplete) {
              log_info(
                  "BATTLE_PROCESSOR: Calling finishBattle() on active battle");
              processor.finishBattle();
            } else {
              log_info("BATTLE_PROCESSOR: Skipping finishBattle() - "
                       "isBattleActive={}, finished={}, simulationComplete={}",
                       processor.isBattleActive() ? 1 : 0,
                       processor.finished ? 1 : 0,
                       processor.simulationComplete ? 1 : 0);
              // If battle already completed naturally, we need to clear it
              // But we can't directly access activeBattle (it's private)
              // CRITICAL: If simulationComplete is true, finishBattle() was
              // already called when the battle completed. Don't call it again -
              // just reset state. However, if isBattleActive() is still true,
              // it means activeBattle wasn't cleared, so we need to call
              // finishBattle() to clear it (it will check simulationComplete
              // and skip BattleResult creation)
              if (processor.simulationComplete && processor.isBattleActive() &&
                  !processor.finished) {
                log_info("BATTLE_PROCESSOR: Battle already completed naturally "
                         "but activeBattle not cleared, "
                         "calling finishBattle() to clear it");
                // Call finishBattle() - it will check simulationComplete and
                // just clear activeBattle without recreating BattleResult
                processor.finishBattle();
              } else if (processor.simulationComplete &&
                         !processor.isBattleActive()) {
                // Battle completed and activeBattle already cleared - just
                // reset flags
                log_info("BATTLE_PROCESSOR: Battle already completed naturally "
                         "and activeBattle cleared, "
                         "just resetting flags");
                processor.simulationStarted = false;
                processor.finished = true;
              }
            }

            processor.simulationStarted = false;
            last_battle_player_path =
                ""; // Clear it so we can start the new battle
            current_started_battle_path =
                ""; // Clear started path so we can start new battle
            should_start_new_battle = true;
          } else {
            // Same battle - don't finish it, just continue
            log_info(
                "BATTLE_PROCESSOR: Battle active and same battle requested - "
                "continuing current battle. last_path='{}', new_path='{}'",
                last_battle_player_path, request.playerJsonPath);
            should_start_new_battle = false;
          }
        }
      }
    }

    // Start new battle if we should start one
    // CRITICAL: Only start if we haven't already started this battle (check
    // last_path)
    if (should_start_new_battle) {
      // We've already determined we need to start a new battle - do it now
      // Note: We already finished the old battle above if needed, so don't do
      // it again
      if (battleRequest.get().has<BattleLoadRequest>()) {
        auto &request = battleRequest.get().get<BattleLoadRequest>();

        // CRITICAL: Check if we've already started this battle to prevent
        // repeated calls Use current_started_battle_path to track which battle
        // we've called startBattle for Also check if battle is active - if it
        // is, we've already started it
        bool already_started_this_battle =
            (current_started_battle_path == request.playerJsonPath) ||
            (processor.isBattleActive() && processor.simulationStarted &&
             last_battle_player_path == request.playerJsonPath);

        if (already_started_this_battle) {
          log_info("BATTLE_PROCESSOR: Battle already started, skipping "
                   "startBattle call - started_path='{}', current_path='{}', "
                   "last_path='{}', isBattleActive={}, simulationStarted={}",
                   current_started_battle_path, request.playerJsonPath,
                   last_battle_player_path, processor.isBattleActive() ? 1 : 0,
                   processor.simulationStarted ? 1 : 0);
        } else {
          // CRITICAL: Don't return early here - we need to let updateSimulation
          // run! The check above already determined whether to start a new
          // battle or not Even if we don't start a new battle, we still need to
          // call updateSimulation

          // If battle is still active with different path, finish it first
          if (processor.isBattleActive() &&
              last_battle_player_path != request.playerJsonPath &&
              !last_battle_player_path.empty()) {
            log_info("BATTLE_PROCESSOR: Finishing old battle (different path) "
                     "before starting new one - isBattleActive={}, finished={}",
                     processor.isBattleActive() ? 1 : 0,
                     processor.finished ? 1 : 0);
            // Only call finishBattle if not already finished
            if (!processor.finished) {
              processor.finishBattle();
            } else {
              log_info("BATTLE_PROCESSOR: Skipping finishBattle() - already "
                       "finished");
            }
          }

          log_info("BATTLE_PROCESSOR: Starting new battle (should_start=true) "
                   "- playerPath={}, isBattleActive={}, last_path='{}'",
                   request.playerJsonPath, processor.isBattleActive(),
                   last_battle_player_path);

          BattleProcessor::BattleInput input;
          input.playerJsonPath = request.playerJsonPath;
          input.opponentJsonPath = request.opponentJsonPath;
          input.seed = 1234567890; // TODO: Load from JSON

          log_info("BATTLE_PROCESSOR: About to call startBattle - "
                   "playerPath={}, isBattleActive before={}",
                   request.playerJsonPath, processor.isBattleActive() ? 1 : 0);
          processor.startBattle(input);
          log_info("BATTLE_PROCESSOR: After startBattle - isBattleActive={}, "
                   "playerDishes={}, opponentDishes={}",
                   processor.isBattleActive() ? 1 : 0,
                   processor.playerDishes.size(),
                   processor.opponentDishes.size());
          processor.simulationStarted = true;
          last_battle_player_path =
              request.playerJsonPath; // Track this battle's path
          current_started_battle_path =
              request.playerJsonPath; // Track that we've started this battle
          log_info("BATTLE_PROCESSOR: Battle simulation started - "
                   "isBattleActive={}, simulationStarted={}, playerPath={}",
                   processor.isBattleActive(), processor.simulationStarted,
                   request.playerJsonPath);
        }
      }
    } else if (!processor.simulationStarted && !processor.isBattleActive()) {
      // Normal case: no battle active, start a new one
      if (battleRequest.get().has<BattleLoadRequest>()) {
        auto &request = battleRequest.get().get<BattleLoadRequest>();

        log_info("BATTLE_PROCESSOR: Checking battle start - loaded={}, "
                 "isBattleActive={}, simulationStarted={}",
                 request.loaded, processor.isBattleActive(),
                 processor.simulationStarted);

        if (request.loaded && !processor.isBattleActive()) {
          // Start new battle simulation
          log_info("BATTLE_PROCESSOR: Starting new battle simulation - "
                   "playerPath={}, opponentPath={}",
                   request.playerJsonPath, request.opponentJsonPath);
          BattleProcessor::BattleInput input;
          input.playerJsonPath = request.playerJsonPath;
          input.opponentJsonPath = request.opponentJsonPath;
          input.seed = 1234567890; // TODO: Load from JSON

          processor.startBattle(input);
          processor.simulationStarted = true;
          last_battle_player_path =
              request.playerJsonPath; // Track this battle's path
          current_started_battle_path =
              request.playerJsonPath; // Track that we've started this battle
          log_info("BATTLE_PROCESSOR: Battle simulation started - "
                   "isBattleActive={}, simulationStarted={}, playerPath={}",
                   processor.isBattleActive(), processor.simulationStarted,
                   request.playerJsonPath);
        } else {
          log_info("BATTLE_PROCESSOR: Cannot start battle - loaded={}, "
                   "isBattleActive={}, simulationStarted={}",
                   request.loaded, processor.isBattleActive(),
                   processor.simulationStarted);
        }
      } else {
        log_info("BATTLE_PROCESSOR: No BattleLoadRequest found");
      }
    } else {
      log_info("BATTLE_PROCESSOR: Skipping battle start - "
               "simulationStarted={}, isBattleActive={}",
               processor.simulationStarted, processor.isBattleActive());
    }

    // CRITICAL: Always check and call updateSimulation if battle is active
    if (processor.isBattleActive()) {
      processor.updateSimulation(dt);

      if (processor.simulationComplete) {
        log_info("BATTLE_PROCESSOR: Battle completed, finishing battle - "
                 "last_path='{}', finished={}",
                 last_battle_player_path, processor.finished ? 1 : 0);
        // Only call finishBattle if not already finished
        if (!processor.finished) {
          processor.finishBattle();
        } else {
          log_info(
              "BATTLE_PROCESSOR: Skipping finishBattle() - already finished");
        }
        processor.simulationStarted = false;
        // CRITICAL: Clear last_battle_player_path when battle finishes so next
        // battle can start
        last_battle_player_path = "";
        current_started_battle_path =
            ""; // Clear started path when battle finishes
        log_info("BATTLE_PROCESSOR: Battle finished - isBattleActive={}, "
                 "simulationStarted={}, last_path cleared",
                 processor.isBattleActive(), processor.simulationStarted);

        // CRITICAL: Ensure battle is fully cleared - force reset activeBattle
        // if needed This ensures the next battle can start properly
        if (processor.isBattleActive()) {
          log_info("BATTLE_PROCESSOR: WARNING - Battle still active after "
                   "finishBattle(), forcing reset");
          // finishBattle() should have cleared activeBattle, but if it didn't,
          // we need to handle it Since activeBattle is private, we can't
          // directly reset it, but finishBattle() should handle it
        }

        // Transition to results screen
        GameStateManager::get().to_results();
      }
    } else if (processor.simulationStarted && !processor.isBattleActive()) {
      // Battle was started but is no longer active (finished or cleared)
      // This can happen if finishBattle() was called but simulationStarted
      // wasn't reset
      log_info("BATTLE_PROCESSOR: Battle was started but is no longer active, "
               "resetting simulationStarted");
      processor.simulationStarted = false;
    }
  }
};
