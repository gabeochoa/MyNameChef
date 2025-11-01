
#include "../components/battle_processor.h"
#include "../components/battle_result.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../dish_types.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <fstream>
#include <magic_enum/magic_enum.hpp>

// Implementation of BattleProcessor methods
void BattleProcessor::startBattle(const BattleInput &input) {
  activeBattle = input;
  playerDishes.clear();
  opponentDishes.clear();
  outcomes.clear();

  currentCourse = 0;
  totalCourses = 7;
  simulationComplete = false;
  playerWins = 0;
  opponentWins = 0;
  ties = 0;
  simulationTime = 0.0f;

  // Load teams from JSON files
  loadTeamFromJson(input.playerJsonPath, true);
  loadTeamFromJson(input.opponentJsonPath, false);

  initializeDishes();
}

void BattleProcessor::updateSimulation(float dt) {
  if (simulationComplete)
    return;

  simulationTime += dt;

  // Process current course
  processCourse(currentCourse, dt);

  // Check if current course is finished
  auto playerDish = findDishForSlot(currentCourse, true);
  auto opponentDish = findDishForSlot(currentCourse, false);

  if (playerDish && opponentDish &&
      playerDish->phase == DishSimData::Phase::Finished &&
      opponentDish->phase == DishSimData::Phase::Finished) {
    advanceCourse();
  }

  // Check if all courses complete
  if (currentCourse >= totalCourses) {
    simulationComplete = true;
  }
}

void BattleProcessor::finishBattle() {
  // Create BattleResult singleton
  BattleResult result;

  // Determine overall outcome
  if (playerWins > opponentWins) {
    result.outcome = BattleResult::Outcome::PlayerWin;
  } else if (opponentWins > playerWins) {
    result.outcome = BattleResult::Outcome::OpponentWin;
  } else {
    result.outcome = BattleResult::Outcome::Tie;
  }

  result.playerWins = playerWins;
  result.opponentWins = opponentWins;
  result.ties = ties;

  // Convert outcomes
  for (const auto &outcome : outcomes) {
    BattleResult::CourseOutcome courseOutcome;
    courseOutcome.slotIndex = outcome.slotIndex;
    courseOutcome.ticks = outcome.ticks;

    switch (outcome.winner) {
    case CourseOutcome::Winner::Player:
      courseOutcome.winner = BattleResult::CourseOutcome::Winner::Player;
      break;
    case CourseOutcome::Winner::Opponent:
      courseOutcome.winner = BattleResult::CourseOutcome::Winner::Opponent;
      break;
    case CourseOutcome::Winner::Tie:
      courseOutcome.winner = BattleResult::CourseOutcome::Winner::Tie;
      break;
    }

    result.outcomes.push_back(courseOutcome);
  }

  // Create or update BattleResult singleton
  auto resultEntity = afterhours::EntityHelper::get_singleton<BattleResult>();
  if (resultEntity.get().has<BattleResult>()) {
    auto &existingResult = resultEntity.get().get<BattleResult>();
    existingResult.outcome = result.outcome;
    existingResult.playerWins = result.playerWins;
    existingResult.opponentWins = result.opponentWins;
    existingResult.ties = result.ties;
    existingResult.outcomes = std::move(result.outcomes);
  } else {
    auto &entity = afterhours::EntityHelper::createEntity();
    entity.addComponent<BattleResult>(std::move(result));
    afterhours::EntityHelper::registerSingleton<BattleResult>(entity);
  }

  // Clear active battle
  activeBattle.reset();
}

void BattleProcessor::loadTeamFromJson(const std::string &jsonPath,
                                       bool isPlayer) {
  if (!std::filesystem::exists(jsonPath)) {
    return;
  }

  std::ifstream file(jsonPath);
  if (!file.is_open()) {
    return;
  }

  nlohmann::json json;
  try {
    file >> json;
  } catch (const std::exception &e) {
    return;
  }

  if (!json.contains("team") || !json["team"].is_array()) {
    return;
  }

  const auto &team = json["team"];
  for (size_t i = 0; i < team.size(); ++i) {
    const auto &dishEntry = team[i];

    if (!dishEntry.contains("slot") || !dishEntry.contains("dishType")) {
      continue;
    }

    int slot = dishEntry["slot"];
    std::string dishTypeStr = dishEntry["dishType"];

    auto dishTypeOpt = magic_enum::enum_cast<DishType>(dishTypeStr);
    if (!dishTypeOpt.has_value()) {
      continue;
    }

    DishSimData dish;
    dish.dishType = static_cast<int>(dishTypeOpt.value());
    dish.level = 1; // Default level
    dish.slot = slot;
    dish.isPlayer = isPlayer;

    if (isPlayer) {
      playerDishes.push_back(dish);
    } else {
      opponentDishes.push_back(dish);
    }
  }
}

void BattleProcessor::initializeDishes() {
  // Initialize player dishes
  for (auto &dish : playerDishes) {
    dish.phase = DishSimData::Phase::InQueue;
    dish.biteTimer = 0.0f;
    dish.playersTurn = true;
    dish.firstBiteDecided = false;

    // Calculate stats
    auto stats = calculateDishStats(dish.dishType, dish.level);
    dish.baseZing = stats.first;
    dish.baseBody = stats.second;
    dish.currentZing = dish.baseZing;
    dish.currentBody = dish.baseBody;
  }

  // Initialize opponent dishes
  for (auto &dish : opponentDishes) {
    dish.phase = DishSimData::Phase::InQueue;
    dish.biteTimer = 0.0f;
    dish.playersTurn = true;
    dish.firstBiteDecided = false;

    // Calculate stats
    auto stats = calculateDishStats(dish.dishType, dish.level);
    dish.baseZing = stats.first;
    dish.baseBody = stats.second;
    dish.currentZing = dish.baseZing;
    dish.currentBody = dish.baseBody;
  }
}

void BattleProcessor::processCourse(int courseIndex, float dt) {
  auto playerDish = findDishForSlot(courseIndex, true);
  auto opponentDish = findDishForSlot(courseIndex, false);

  if (!playerDish || !opponentDish) {
    return;
  }

  // Start entering if both are in queue
  if (playerDish->phase == DishSimData::Phase::InQueue &&
      opponentDish->phase == DishSimData::Phase::InQueue) {
    playerDish->phase = DishSimData::Phase::Entering;
    opponentDish->phase = DishSimData::Phase::Entering;
    return;
  }

  // Handle entering phase
  if (playerDish->phase == DishSimData::Phase::Entering) {
    // Simple enter simulation - just move to combat after duration
    playerDish->phase = DishSimData::Phase::InCombat;
    opponentDish->phase = DishSimData::Phase::InCombat;
    return;
  }

  // Handle combat phase
  if (playerDish->phase == DishSimData::Phase::InCombat &&
      opponentDish->phase == DishSimData::Phase::InCombat) {
    resolveCombatTick(*playerDish, *opponentDish, dt);
  }
}

void BattleProcessor::resolveCombatTick(DishSimData &player,
                                        DishSimData &opponent, float dt) {
  // Only process from player side to avoid double processing
  if (!player.isPlayer)
    return;

  player.biteTimer += dt;
  if (player.biteTimer < kTickMs) {
    return;
  }

  player.biteTimer = 0.0f;

  // Mark first bite as decided (for initialization tracking)
  if (!player.firstBiteDecided) {
    player.firstBiteDecided = true;
  }

  // Both dishes attack simultaneously
  // Player dish attacks opponent
  int player_damage = std::max(1, player.currentZing);
  opponent.currentBody -= player_damage;

  // Opponent dish attacks player
  int opponent_damage = std::max(1, opponent.currentZing);
  player.currentBody -= opponent_damage;

  // Check if either dish is defeated
  if (player.currentBody <= 0 || opponent.currentBody <= 0) {
    finishCourse(player, opponent);
  }
}

void BattleProcessor::finishCourse(DishSimData &player, DishSimData &opponent) {
  CourseOutcome outcome;
  outcome.slotIndex = player.slot;

  if (player.currentBody <= 0 && opponent.currentBody <= 0) {
    outcome.winner = CourseOutcome::Winner::Tie;
    ties++;
  } else if (player.currentBody <= 0) {
    outcome.winner = CourseOutcome::Winner::Opponent;
    opponentWins++;
  } else {
    outcome.winner = CourseOutcome::Winner::Player;
    playerWins++;
  }

  outcome.ticks =
      static_cast<int>(simulationTime * 1000.0f / kTickMs); // Approximate

  outcomes.push_back(outcome);

  player.phase = DishSimData::Phase::Finished;
  opponent.phase = DishSimData::Phase::Finished;
}

void BattleProcessor::advanceCourse() { currentCourse++; }

BattleProcessor::DishSimData *BattleProcessor::findDishForSlot(int slot,
                                                               bool isPlayer) {
  auto &dishes = isPlayer ? playerDishes : opponentDishes;
  for (auto &dish : dishes) {
    if (dish.slot == slot) {
      return &dish;
    }
  }
  return nullptr;
}

std::pair<int, int> BattleProcessor::calculateDishStats(int dishType,
                                                        int level) {
  auto dishInfo = get_dish_info(static_cast<DishType>(dishType));
  const FlavorStats &flavor = dishInfo.flavor;

  int zing = flavor.zing();
  int body = flavor.body();

  // Level scaling: multiply by 2 for each level above 1
  if (level > 1) {
    int levelMultiplier = 1 << (level - 1); // 2^(level-1)
    zing *= levelMultiplier;
    body *= levelMultiplier;
  }

  return {std::max(1, zing), std::max(0, body)};
}

bool BattleProcessor::determineFirstAttacker(const DishSimData &player,
                                             const DishSimData &opponent) {
  // Tiebreaker 1: Highest Zing
  if (player.baseZing > opponent.baseZing) {
    return true; // Player goes first
  } else if (opponent.baseZing > player.baseZing) {
    return false; // Opponent goes first
  }

  // Tiebreaker 2: Highest Body
  if (player.baseBody > opponent.baseBody) {
    return true; // Player goes first
  } else if (opponent.baseBody > player.baseBody) {
    return false; // Opponent goes first
  }

  // Tiebreaker 3: Deterministic fallback
  return true; // Player goes first
}
