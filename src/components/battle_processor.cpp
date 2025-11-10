
#include "../components/battle_processor.h"
#include "../components/battle_result.h"
#include "../components/trigger_event.h"
#include "../components/trigger_queue.h"
#include "../dish_types.h"
#include "../seeded_rng.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <fstream>
#include <magic_enum/magic_enum.hpp>

// Implementation of BattleProcessor methods
void BattleProcessor::startBattle(const BattleInput &input) {
  log_info("BATTLE_PROCESSOR_START: Starting battle - playerPath={}, opponentPath={}", input.playerJsonPath, input.opponentJsonPath);
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
  finished = false; // Reset finished flag when starting new battle
  log_info("BATTLE_PROCESSOR_START: Battle initialized - simulationComplete=%d, currentCourse=%d, playerDishes=%zu, opponentDishes=%zu, finished=%d", 
           (int)simulationComplete, currentCourse, playerDishes.size(), opponentDishes.size(), (int)finished);

  // Load teams from JSON files
  loadTeamFromJson(input.playerJsonPath, true);
  loadTeamFromJson(input.opponentJsonPath, false);

  initializeDishes();
}

void BattleProcessor::updateSimulation(float dt) {
  // Log every call to verify it's being called
  static int call_count = 0;
  call_count++;
  if (call_count % 60 == 0 || call_count <= 10) { // Log first 10 calls and every 60 after
    log_info("BATTLE_SIM: updateSimulation CALLED - call=%d, complete=%d, time=%.2f, course=%d, dishes=%zu/%zu", 
             call_count, (int)simulationComplete, simulationTime, currentCourse, playerDishes.size(), opponentDishes.size());
  }
  
  if (simulationComplete) {
    if (call_count % 60 == 0 || call_count <= 10) {
      log_info("BATTLE_SIM: updateSimulation EARLY RETURN - simulationComplete=true, call=%d", call_count);
    }
    return;
  }

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
  // Step 1: Defensive guards - check if already finished or battle not active
  // CRITICAL: Check simulationComplete FIRST, before any other operations
  // This prevents crashes if the battle already finished and BattleResult was created
  if (simulationComplete) {
    log_info("BATTLE_PROCESSOR_FINISH: Battle already completed naturally, just clearing activeBattle - finished=%d, isBattleActive=%d", 
             (int)finished, (int)isBattleActive());
    activeBattle.reset();
    finished = true;
    return;
  }
  
  log_info("BATTLE_PROCESSOR_FINISH: Called - finished=%d, isBattleActive=%d, simulationComplete=%d, activeBattle.has_value=%d", 
           (int)finished, (int)isBattleActive(), (int)simulationComplete, activeBattle.has_value() ? 1 : 0);
  
  if (finished) {
    log_info("BATTLE_PROCESSOR_FINISH: Already finished, returning early");
    return;
  }
  
  if (!activeBattle.has_value()) {
    log_info("BATTLE_PROCESSOR_FINISH: No active battle, returning early");
    finished = true; // Mark as finished to prevent future calls
    return;
  }
  
  // Wrap entire function in try/catch for safety
  try {
    log_info("BATTLE_PROCESSOR_FINISH: Starting battle finish - playerWins=%d, opponentWins=%d, ties=%d, outcomes.size=%zu", 
             playerWins, opponentWins, ties, outcomes.size());
    
    // Step 2: Handle incomplete battles gracefully
    // If battle hasn't completed naturally, create a partial result
    bool isComplete = simulationComplete || (currentCourse >= totalCourses);
    if (!isComplete) {
      log_info("BATTLE_PROCESSOR_FINISH: Battle incomplete (currentCourse=%d, totalCourses=%d), creating partial result", 
               currentCourse, totalCourses);
    }
    
    // Create BattleResult singleton
    BattleResult result;

    // Determine overall outcome - handle case where no courses completed
    if (outcomes.empty() && playerWins == 0 && opponentWins == 0 && ties == 0) {
      // No courses completed - default to tie
      log_info("BATTLE_PROCESSOR_FINISH: No courses completed, defaulting to tie");
      result.outcome = BattleResult::Outcome::Tie;
    } else if (playerWins > opponentWins) {
      result.outcome = BattleResult::Outcome::PlayerWin;
    } else if (opponentWins > playerWins) {
      result.outcome = BattleResult::Outcome::OpponentWin;
    } else {
      result.outcome = BattleResult::Outcome::Tie;
    }

    result.playerWins = playerWins;
    result.opponentWins = opponentWins;
    result.ties = ties;

    // Convert outcomes - safely iterate even if empty
    log_info("BATTLE_PROCESSOR_FINISH: Converting {} outcomes", outcomes.size());
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
    log_info("BATTLE_PROCESSOR_FINISH: Converted {} outcomes to BattleResult", result.outcomes.size());

    // Create or update BattleResult singleton
    log_info("BATTLE_PROCESSOR_FINISH: Accessing BattleResult singleton");
    const auto componentId = afterhours::components::get_type_id<BattleResult>();
    bool singletonExists =
        afterhours::EntityHelper::get().singletonMap.contains(componentId);
    log_info("BATTLE_PROCESSOR_FINISH: Singleton exists=%d", singletonExists ? 1 : 0);

  // CRITICAL: Always create a new BattleResult singleton instead of trying to update existing one
  // This avoids crashes if the previous singleton was cleaned up
  // The Results screen will use the most recent BattleResult
  // First, remove old singleton from map if it exists (to avoid registerSingleton error)
  // CRITICAL: Just remove from map - don't try to access the entity as it might be cleaned up
  if (singletonExists) {
    // Remove from singleton map to allow registering new one
    // Don't try to access the old entity - it might have been cleaned up and accessing it would crash
    afterhours::EntityHelper::get().singletonMap.erase(componentId);
    log_info("BATTLE_PROCESSOR_FINISH: Removed old singleton from map (entity may have been cleaned up)");
  }
  
  log_info("BATTLE_PROCESSOR_FINISH: Creating new BattleResult singleton");
  try {
    auto &entity = afterhours::EntityHelper::createEntity();
    entity.addComponent<BattleResult>(std::move(result));
    afterhours::EntityHelper::registerSingleton<BattleResult>(entity);
    log_info("BATTLE_PROCESSOR_FINISH: Successfully created and registered BattleResult singleton");
  } catch (const std::exception &e) {
    log_info("BATTLE_PROCESSOR_FINISH: Exception creating BattleResult singleton: {}", e.what());
    // If creating singleton fails, we can't continue - but at least we've cleared activeBattle above
    // The battle will be marked as finished, so it won't be called again
  } catch (...) {
    log_info("BATTLE_PROCESSOR_FINISH: Unknown exception creating BattleResult singleton");
    // If creating singleton fails, we can't continue - but at least we've cleared activeBattle above
    // The battle will be marked as finished, so it won't be called again
  }

    // Clear active battle
    log_info("BATTLE_PROCESSOR_FINISH: Clearing activeBattle - was_active=%d", activeBattle.has_value() ? 1 : 0);
    activeBattle.reset();
    finished = true; // Mark as finished to prevent multiple calls
    log_info("BATTLE_PROCESSOR_FINISH: activeBattle cleared - isBattleActive=%d, finished=%d", 
             activeBattle.has_value() ? 1 : 0, (int)finished);
  } catch (const std::exception &e) {
    log_info("BATTLE_PROCESSOR_FINISH: Exception caught in finishBattle: {}", e.what());
    // Still mark as finished and clear active battle to prevent retry
    finished = true;
    activeBattle.reset();
  } catch (...) {
    log_info("BATTLE_PROCESSOR_FINISH: Unknown exception caught in finishBattle");
    // Still mark as finished and clear active battle to prevent retry
    finished = true;
    activeBattle.reset();
  }
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

  static int process_count = 0;
  process_count++;
  if (process_count % 60 == 0 || process_count <= 10) {
    int player_phase = playerDish ? (int)playerDish->phase : -1;
    int opponent_phase = opponentDish ? (int)opponentDish->phase : -1;
    log_info("BATTLE_SIM: processCourse - course={}, playerDish exists={}, opponentDish exists={}, playerPhase={}, opponentPhase={}, call={}", 
             courseIndex, playerDish ? 1 : 0, opponentDish ? 1 : 0, player_phase, opponent_phase, process_count);
  }

  if (!playerDish || !opponentDish) {
    if (process_count % 60 == 0 || process_count <= 10) {
      log_info("BATTLE_SIM: processCourse - No dishes found for course %d (player=%p, opponent=%p)", 
               courseIndex, (void*)playerDish, (void*)opponentDish);
    }
    return;
  }

  // Start entering if both are in queue
  if (playerDish->phase == DishSimData::Phase::InQueue &&
      opponentDish->phase == DishSimData::Phase::InQueue) {
    log_info("BATTLE_SIM: Course {} - Dishes entering (player slot={}, opponent slot={})", 
             courseIndex, playerDish->slot, opponentDish->slot);
    playerDish->phase = DishSimData::Phase::Entering;
    opponentDish->phase = DishSimData::Phase::Entering;
    return;
  }

  // Handle entering phase
  if (playerDish->phase == DishSimData::Phase::Entering) {
    // Simple enter simulation - just move to combat after duration
    log_info("BATTLE_SIM: Course {} - Dishes entering combat (player slot={}, opponent slot={})", 
             courseIndex, playerDish->slot, opponentDish->slot);
    playerDish->phase = DishSimData::Phase::InCombat;
    opponentDish->phase = DishSimData::Phase::InCombat;
    return;
  }

  // Handle combat phase
  if (playerDish->phase == DishSimData::Phase::InCombat &&
      opponentDish->phase == DishSimData::Phase::InCombat) {
    if (process_count % 60 == 0 || process_count <= 10) {
      log_info("BATTLE_SIM: Course {} - Calling resolveCombatTick (player slot={}, opponent slot={}, dt={})", 
               courseIndex, playerDish->slot, opponentDish->slot, dt);
    }
    resolveCombatTick(*playerDish, *opponentDish, dt);
  } else {
    if (process_count % 60 == 0 || process_count <= 10) {
      log_info("BATTLE_SIM: Course {} - NOT calling resolveCombatTick - playerPhase={}, opponentPhase={}", 
               courseIndex, (int)playerDish->phase, (int)opponentDish->phase);
    }
  }
}

void BattleProcessor::resolveCombatTick(DishSimData &player,
                                        DishSimData &opponent, float dt) {
  // Only process from player side to avoid double processing
  if (!player.isPlayer)
    return;

  player.biteTimer += dt;
  static int early_return_count = 0;
  early_return_count++;
  float tick_duration = BattleTiming::get_tick_duration();
  if (player.biteTimer < tick_duration) {
    if (early_return_count % 60 == 0 || early_return_count <= 10) {
      log_info("BATTLE_SIM: resolveCombatTick early return - biteTimer={:.3f} < tick_duration={:.3f}, dt={:.3f}, call={}", 
               player.biteTimer, tick_duration, dt, early_return_count);
    }
    return;
  }

  player.biteTimer = 0.0f;

  // Mark first bite as decided (for initialization tracking)
  if (!player.firstBiteDecided) {
    player.firstBiteDecided = true;
    log_info("BATTLE_SIM: Course %d - First combat tick (player slot=%d body=%d, opponent slot=%d body=%d)", 
             player.slot, player.slot, player.currentBody, opponent.slot, opponent.currentBody);
  }

  // Both dishes attack simultaneously
  // Player dish attacks opponent
  int player_damage = std::max(1, player.currentZing);
  int old_opponent_body = opponent.currentBody;
  opponent.currentBody -= player_damage;

  // Opponent dish attacks player
  int opponent_damage = std::max(1, opponent.currentZing);
  int old_player_body = player.currentBody;
  player.currentBody -= opponent_damage;

  static int tick_count = 0;
  tick_count++;
  if (tick_count % 10 == 0) { // Log every 10 ticks to avoid spam
    log_info("BATTLE_SIM: Course %d - Combat tick %d (player slot=%d body: %d->%d, opponent slot=%d body: %d->%d)", 
             player.slot, tick_count, player.slot, old_player_body, player.currentBody, 
             opponent.slot, old_opponent_body, opponent.currentBody);
  }

  // Check if either dish is defeated
  if (player.currentBody <= 0 || opponent.currentBody <= 0) {
    log_info("BATTLE_SIM: Course %d - Dish defeated (player body=%d, opponent body=%d)", 
             player.slot, player.currentBody, opponent.currentBody);
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

  float tick_duration = BattleTiming::get_tick_duration();
  outcome.ticks =
      static_cast<int>(simulationTime / tick_duration); // Approximate

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
    int levelMultiplier = 2;
    for (int i = 2; i < level; ++i) {
      levelMultiplier *= 2;
    }
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

  // Tiebreaker 3: Random using SeededRng (deterministic based on seed)
  auto &rng = SeededRng::get();
  return rng.gen_mod(2) == 0; // 50/50 chance
}
