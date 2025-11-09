#pragma once

#include "../components/battle_load_request.h"
#include "../components/battle_processor.h"
#include "../game_state_manager.h"
#include "../shop.h"
#include <afterhours/ah.h>

struct BattleProcessorSystem : afterhours::System<BattleProcessor> {
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Battle)
      return false;

    auto battleProcessor =
        afterhours::EntityHelper::get_singleton<BattleProcessor>();
    if (!battleProcessor.get().has<BattleProcessor>())
      return false;
    return !hasActiveAnimation();
  }

  void for_each_with(afterhours::Entity &, BattleProcessor &processor,
                     float dt) override {
    auto &gsm = GameStateManager::get();
    
    bool entering_battle = last_screen != GameStateManager::Screen::Battle &&
                           gsm.active_screen == GameStateManager::Screen::Battle;
    
    if (entering_battle && !processor.isBattleActive()) {
      log_info("BATTLE_PROCESSOR: Entering battle screen, resetting simulationStarted");
      processor.simulationStarted = false;
    }
    
    last_screen = gsm.active_screen;

    if (!processor.simulationStarted) {
      // Check for BattleLoadRequest to start simulation
      auto battleRequest =
          afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
      if (battleRequest.get().has<BattleLoadRequest>()) {
        auto &request = battleRequest.get().get<BattleLoadRequest>();

        log_info("BATTLE_PROCESSOR: Checking battle start - loaded={}, isBattleActive={}", 
                 request.loaded, processor.isBattleActive());

        if (request.loaded && !processor.isBattleActive()) {
          // Start new battle simulation
          log_info("BATTLE_PROCESSOR: Starting new battle simulation");
          BattleProcessor::BattleInput input;
          input.playerJsonPath = request.playerJsonPath;
          input.opponentJsonPath = request.opponentJsonPath;
          input.seed = 1234567890; // TODO: Load from JSON

          processor.startBattle(input);
          processor.simulationStarted = true;
          log_info("BATTLE_PROCESSOR: Battle simulation started");
        } else {
          log_info("BATTLE_PROCESSOR: Cannot start battle - loaded={}, isBattleActive={}", 
                   request.loaded, processor.isBattleActive());
        }
      } else {
        log_info("BATTLE_PROCESSOR: No BattleLoadRequest found");
      }
    }

    if (processor.isBattleActive()) {
      processor.updateSimulation(dt);

      if (processor.simulationComplete) {
        processor.finishBattle();
        processor.simulationStarted = false;

        // Transition to results screen
        GameStateManager::get().to_results();
      }
    }
  }
};
