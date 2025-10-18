#pragma once

#include "../components/battle_load_request.h"
#include "../components/battle_processor.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

struct BattleProcessorSystem : afterhours::System<BattleProcessor> {
  virtual bool should_run(float) override {
    auto battleProcessor =
        afterhours::EntityHelper::get_singleton<BattleProcessor>();
    if (!battleProcessor.get().has<BattleProcessor>()) {
      return false;
    }

    auto &processor = battleProcessor.get().get<BattleProcessor>();
    return processor.isBattleActive();
  }

  void for_each_with(afterhours::Entity &, BattleProcessor &processor,
                     float dt) override {
    if (!processor.simulationStarted) {
      // Check for BattleLoadRequest to start simulation
      auto battleRequest =
          afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
      if (battleRequest.get().has<BattleLoadRequest>()) {
        auto &request = battleRequest.get().get<BattleLoadRequest>();

        if (request.loaded && !processor.isBattleActive()) {
          // Start new battle simulation
          BattleProcessor::BattleInput input;
          input.playerJsonPath = request.playerJsonPath;
          input.opponentJsonPath = request.opponentJsonPath;
          input.seed = 1234567890; // TODO: Load from JSON

          processor.startBattle(input);
          processor.simulationStarted = true;
        }
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
