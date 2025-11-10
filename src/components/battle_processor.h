#pragma once

#include "../battle_timing.h"
#include <afterhours/ah.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

struct BattleProcessor : afterhours::BaseComponent {
  // Input data
  struct BattleInput {
    nlohmann::json playerTeamJson;
    nlohmann::json opponentTeamJson;
    uint64_t seed = 0;
    std::string playerJsonPath;
    std::string opponentJsonPath;
  };

  // Simulation state
  struct DishSimData {
    int entityId = 0;
    int dishType = 0; // DishType as int
    int level = 1;
    int slot = 0;
    bool isPlayer = true;

    // Combat stats
    int baseZing = 0;
    int baseBody = 0;
    int currentZing = 0;
    int currentBody = 0;

    // Battle state
    enum Phase { InQueue, Entering, InCombat, Finished } phase = InQueue;
    float biteTimer = 0.0f;
    bool playersTurn = true;
    bool firstBiteDecided = false;
  };

  struct CourseOutcome {
    int slotIndex = 0;
    enum Winner { Player, Opponent, Tie } winner = Tie;
    int ticks = 0;
  };

  // Battle state
  std::optional<BattleInput> activeBattle;
  std::vector<DishSimData> playerDishes;
  std::vector<DishSimData> opponentDishes;
  std::vector<CourseOutcome> outcomes;

  int currentCourse = 0;
  int totalCourses = 7;
  bool simulationComplete = false;

  // Results
  int playerWins = 0;
  int opponentWins = 0;
  int ties = 0;

  // Simulation control
  bool simulationStarted = false;
  float simulationTime = 0.0f;
  bool finished = false; // Guard against multiple finishBattle() calls


  // Methods
  void startBattle(const BattleInput &input);
  void updateSimulation(float dt);
  void finishBattle();
  bool isBattleActive() const { return activeBattle.has_value(); }

private:
  void loadTeamFromJson(const std::string &jsonPath, bool isPlayer);
  void initializeDishes();
  void processCourse(int courseIndex, float dt);
  void resolveCombatTick(DishSimData &player, DishSimData &opponent, float dt);
  void finishCourse(DishSimData &player, DishSimData &opponent);
  void advanceCourse();
  DishSimData *findDishForSlot(int slot, bool isPlayer);
  std::pair<int, int> calculateDishStats(int dishType, int level);
  bool determineFirstAttacker(const DishSimData &player,
                              const DishSimData &opponent);
};
