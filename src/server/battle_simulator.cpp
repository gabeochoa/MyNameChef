#include "battle_simulator.h"
#include "../components/battle_load_request.h"
#include "../components/battle_result.h"
#include "../components/battle_team_tags.h"
#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../components/trigger_queue.h"
#include "../dish_types.h"
#include "../seeded_rng.h"
#include "file_storage.h"
#include <afterhours/ah.h>
#include <functional>

namespace server {

static void calculate_battle_result_from_teams(BattleResult &result);

BattleSimulator::BattleSimulator()
    : ctx(ServerContext::initialize()), seed(0), battle_active(false),
      simulation_time(0.0f), accumulated_events() {}

void BattleSimulator::start_battle(const nlohmann::json &player_team_json,
                                   const nlohmann::json &opponent_team_json,
                                   uint64_t battle_seed) {

  seed = battle_seed;
  simulation_time = 0.0f;
  battle_active = true;
  accumulated_events.clear();

  SeededRng::get().set_seed(seed);

  FileStorage::ensure_directory_exists("output/battles");

  std::string player_temp =
      "output/battles/temp_player_" + std::to_string(seed) + ".json";
  std::string opponent_temp =
      "output/battles/temp_opponent_" + std::to_string(seed) + ".json";

  FileStorage::save_json_to_file(player_temp, player_team_json);
  FileStorage::save_json_to_file(opponent_temp, opponent_team_json);

  // Check if BattleLoadRequest singleton already exists (from previous
  // test/battle)
  const auto componentId =
      afterhours::components::get_type_id<BattleLoadRequest>();
  bool singletonExists =
      afterhours::EntityHelper::get().singletonMap.contains(componentId);

  log_info("start_battle: BattleLoadRequest singleton exists={}",
           singletonExists);

  if (singletonExists) {
    log_info("start_battle: Reusing existing BattleLoadRequest singleton");
    // Update existing singleton
    auto existingRequest =
        afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    if (existingRequest.get().has<BattleLoadRequest>()) {
      BattleLoadRequest &req = existingRequest.get().get<BattleLoadRequest>();
      req.playerJsonPath = player_temp;
      req.opponentJsonPath = opponent_temp;
      req.loaded = false;
    } else {
      // Add component to existing entity
      existingRequest.get().addComponent<BattleLoadRequest>();
      BattleLoadRequest &req = existingRequest.get().get<BattleLoadRequest>();
      req.playerJsonPath = player_temp;
      req.opponentJsonPath = opponent_temp;
      req.loaded = false;
    }
  } else {
    // Create new singleton
    log_info("start_battle: Creating new BattleLoadRequest singleton");
    afterhours::Entity &request_entity =
        afterhours::EntityHelper::createEntity();
    request_entity.addComponent<BattleLoadRequest>();
    BattleLoadRequest &req = request_entity.get<BattleLoadRequest>();
    req.playerJsonPath = player_temp;
    req.opponentJsonPath = opponent_temp;
    req.loaded = false;
    afterhours::EntityHelper::registerSingleton<BattleLoadRequest>(
        request_entity);
    log_info(
        "start_battle: Registered BattleLoadRequest singleton for entity {}",
        request_entity.id);
  }

  afterhours::EntityHelper::merge_entity_arrays();
}

void BattleSimulator::update(float dt) {
  if (!battle_active) {
    return;
  }

  // Check if battle just completed
  bool was_complete = is_complete();

  int course_index = 0;
  auto cq_entity = afterhours::EntityHelper::get_singleton<CombatQueue>();
  if (cq_entity.get().has<CombatQueue>()) {
    const CombatQueue &cq = cq_entity.get().get<CombatQueue>();
    course_index = cq.current_index;
  }

  track_events(simulation_time, course_index);

  simulation_time += dt;

  const float fixed_dt = 1.0f / 60.0f;
  ctx.systems.run(fixed_dt);

  // After running systems, check if battle completed and create BattleResult
  bool now_complete = is_complete();
  if (now_complete && !was_complete) {
    create_battle_result();
  }
}

bool BattleSimulator::is_complete() const { return ctx.is_battle_complete(); }

nlohmann::json BattleSimulator::get_battle_state() const {
  return nlohmann::json{{"seed", seed},
                        {"complete", is_complete()},
                        {"simulation_time", simulation_time}};
}

void BattleSimulator::track_events(float timestamp, int course_index) {
  auto tq_entity = afterhours::EntityHelper::get_singleton<TriggerQueue>();
  if (!tq_entity.get().has<TriggerQueue>()) {
    return;
  }

  const TriggerQueue &queue = tq_entity.get().get<TriggerQueue>();
  for (const TriggerEvent &ev : queue.events) {
    async::DebugBattleEvent battle_event = {.hook = ev.hook,
                                            .sourceEntityId = ev.sourceEntityId,
                                            .slotIndex = ev.slotIndex,
                                            .teamSide = ev.teamSide,
                                            .timestamp = timestamp,
                                            .courseIndex = course_index,
                                            .payloadInt = ev.payloadInt,
                                            .payloadFloat = ev.payloadFloat};
    accumulated_events.push_back(battle_event);
  }
}

void BattleSimulator::create_battle_result() {
  // Check if BattleResult already exists
  const auto componentId = afterhours::components::get_type_id<BattleResult>();
  bool singletonExists =
      afterhours::EntityHelper::get().singletonMap.contains(componentId);

  if (singletonExists) {
    return;
  }

  // Load results from JSON if available
  std::string resultPath;
  if (afterhours::EntityHelper::get_singleton<BattleLoadRequest>()
          .get()
          .has<BattleLoadRequest>()) {
    auto reqEnt = afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    auto &req = reqEnt.get().get<BattleLoadRequest>();
    resultPath = req.playerJsonPath;
    const auto pos = resultPath.find("pending");
    if (pos != std::string::npos)
      resultPath.replace(pos, 7, "results");
  }

  BattleResult result;
  if (!resultPath.empty()) {
    // Try to load from JSON
    std::ifstream f(resultPath);
    if (f.is_open()) {
      nlohmann::json j;
      try {
        f >> j;
        if (j.contains("outcome")) {
          const std::string s = j["outcome"].get<std::string>();
          if (s == "player_win")
            result.outcome = BattleResult::Outcome::PlayerWin;
          else if (s == "opponent_win")
            result.outcome = BattleResult::Outcome::OpponentWin;
          else
            result.outcome = BattleResult::Outcome::Tie;
        }
      } catch (...) {
        // Fall back to calculating from teams
      }
    }
  }

  if (result.outcome == BattleResult::Outcome::Tie) {
    // Calculate from teams
    calculate_battle_result_from_teams(result);
  }

  // Create course outcome
  BattleResult::CourseOutcome courseOutcome;
  courseOutcome.slotIndex = 0;
  courseOutcome.ticks = 0;
  if (result.outcome == BattleResult::Outcome::PlayerWin) {
    courseOutcome.winner = BattleResult::CourseOutcome::Winner::Player;
  } else if (result.outcome == BattleResult::Outcome::OpponentWin) {
    courseOutcome.winner = BattleResult::CourseOutcome::Winner::Opponent;
  } else {
    courseOutcome.winner = BattleResult::CourseOutcome::Winner::Tie;
  }
  result.outcomes.push_back(courseOutcome);

  // Create singleton
  auto &ent = afterhours::EntityHelper::createEntity();
  ent.addComponent<BattleResult>(std::move(result));
  afterhours::EntityHelper::registerSingleton<BattleResult>(ent);
}

static void calculate_battle_result_from_teams(BattleResult &result) {

  // Get player team dishes
  std::vector<std::reference_wrapper<afterhours::Entity>> playerEntities;
  for (auto &ref : afterhours::EntityQuery()
                       .template whereHasComponent<IsPlayerTeamItem>()
                       .template whereHasComponent<IsDish>()
                       .gen()) {
    playerEntities.push_back(ref);
  }

  // Get opponent team dishes
  std::vector<std::reference_wrapper<afterhours::Entity>> opponentEntities;
  for (auto &ref : afterhours::EntityQuery()
                       .template whereHasComponent<IsOpponentTeamItem>()
                       .template whereHasComponent<IsDish>()
                       .gen()) {
    opponentEntities.push_back(ref);
  }

  // Calculate team scores
  int playerScore = 0;
  for (const auto &entity_ref : playerEntities) {
    auto &entity = entity_ref.get();
    auto &dish = entity.get<IsDish>();
    DishInfo dishInfo = get_dish_info(dish.type);
    FlavorStats &flavor = dishInfo.flavor;
    playerScore += flavor.satiety + flavor.sweetness + flavor.spice +
                   flavor.acidity + flavor.umami + flavor.richness +
                   flavor.freshness;
  }

  int opponentScore = 0;
  for (const auto &entity_ref : opponentEntities) {
    auto &entity = entity_ref.get();
    auto &dish = entity.get<IsDish>();
    DishInfo dishInfo = get_dish_info(dish.type);
    FlavorStats &flavor = dishInfo.flavor;
    opponentScore += flavor.satiety + flavor.sweetness + flavor.spice +
                     flavor.acidity + flavor.umami + flavor.richness +
                     flavor.freshness;
  }

  // Determine outcome
  if (playerScore > opponentScore) {
    result.outcome = BattleResult::Outcome::PlayerWin;
    result.playerWins = 1;
    result.opponentWins = 0;
    result.ties = 0;
  } else if (opponentScore > playerScore) {
    result.outcome = BattleResult::Outcome::OpponentWin;
    result.playerWins = 0;
    result.opponentWins = 1;
    result.ties = 0;
  } else {
    result.outcome = BattleResult::Outcome::Tie;
    result.playerWins = 0;
    result.opponentWins = 0;
    result.ties = 1;
  }
}

void BattleSimulator::ensure_battle_result() {
  // Check if BattleResult already exists
  const auto componentId = afterhours::components::get_type_id<BattleResult>();
  bool singletonExists =
      afterhours::EntityHelper::get().singletonMap.contains(componentId);

  if (singletonExists) {
    log_info(
        "BattleSimulator::ensure_battle_result: BattleResult already exists");
    return;
  }

  log_info("BattleSimulator::ensure_battle_result: Creating BattleResult");

  BattleResult result;

  // Get player team dishes
  std::vector<std::reference_wrapper<afterhours::Entity>> playerEntities;
  for (auto &ref : afterhours::EntityQuery()
                       .template whereHasComponent<IsPlayerTeamItem>()
                       .template whereHasComponent<IsDish>()
                       .gen()) {
    playerEntities.push_back(ref);
  }

  // Get opponent team dishes
  std::vector<std::reference_wrapper<afterhours::Entity>> opponentEntities;
  for (auto &ref : afterhours::EntityQuery()
                       .template whereHasComponent<IsOpponentTeamItem>()
                       .template whereHasComponent<IsDish>()
                       .gen()) {
    opponentEntities.push_back(ref);
  }

  // Calculate team scores based on flavor stats
  int playerTeamScore = 0;
  for (const auto &entity_ref : playerEntities) {
    auto &entity = entity_ref.get();
    auto &dish = entity.get<IsDish>();
    DishInfo dishInfo = get_dish_info(dish.type);
    FlavorStats &flavor = dishInfo.flavor;
    playerTeamScore += flavor.satiety + flavor.sweetness + flavor.spice +
                       flavor.acidity + flavor.umami + flavor.richness +
                       flavor.freshness;
  }

  int opponentTeamScore = 0;
  for (const auto &entity_ref : opponentEntities) {
    auto &entity = entity_ref.get();
    auto &dish = entity.get<IsDish>();
    DishInfo dishInfo = get_dish_info(dish.type);
    FlavorStats &flavor = dishInfo.flavor;
    opponentTeamScore += flavor.satiety + flavor.sweetness + flavor.spice +
                         flavor.acidity + flavor.umami + flavor.richness +
                         flavor.freshness;
  }

  log_info("BattleSimulator::ensure_battle_result: Player team score: {}, "
           "Opponent team score: {}",
           playerTeamScore, opponentTeamScore);

  // Determine outcome
  if (playerTeamScore > opponentTeamScore) {
    result.outcome = BattleResult::Outcome::PlayerWin;
    result.playerWins = 1;
    result.opponentWins = 0;
    result.ties = 0;
  } else if (opponentTeamScore > playerTeamScore) {
    result.outcome = BattleResult::Outcome::OpponentWin;
    result.playerWins = 0;
    result.opponentWins = 1;
    result.ties = 0;
  } else {
    result.outcome = BattleResult::Outcome::Tie;
    result.playerWins = 0;
    result.opponentWins = 0;
    result.ties = 1;
  }

  // Create course outcome
  BattleResult::CourseOutcome courseOutcome;
  courseOutcome.slotIndex = 0;
  courseOutcome.ticks = 0;
  if (playerTeamScore > opponentTeamScore) {
    courseOutcome.winner = BattleResult::CourseOutcome::Winner::Player;
  } else if (opponentTeamScore > playerTeamScore) {
    courseOutcome.winner = BattleResult::CourseOutcome::Winner::Opponent;
  } else {
    courseOutcome.winner = BattleResult::CourseOutcome::Winner::Tie;
  }
  result.outcomes.push_back(courseOutcome);

  // Create singleton
  auto &ent = afterhours::EntityHelper::createEntity();
  ent.addComponent<BattleResult>(std::move(result));
  afterhours::EntityHelper::registerSingleton<BattleResult>(ent);
  log_info("BattleSimulator::ensure_battle_result: Registered BattleResult "
           "singleton for entity {}",
           ent.id);
}

void BattleSimulator::cleanup_test_entities() {
  // Mark all battle team entities for cleanup
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .template whereHasComponent<IsPlayerTeamItem>()
           .gen()) {
    entity.cleanup = true;
  }

  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .template whereHasComponent<IsOpponentTeamItem>()
           .gen()) {
    entity.cleanup = true;
  }

  // Mark all dish entities for cleanup
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .template whereHasComponent<IsDish>()
           .gen()) {
    entity.cleanup = true;
  }

  // Reset CombatQueue state instead of deleting (singletons may be needed by
  // systems)
  auto combatQueue = afterhours::EntityHelper::get_singleton<CombatQueue>();
  if (combatQueue.get().has<CombatQueue>()) {
    CombatQueue &cq = combatQueue.get().get<CombatQueue>();
    cq.reset();
  }

  // Reset TriggerQueue state instead of deleting
  auto triggerQueue = afterhours::EntityHelper::get_singleton<TriggerQueue>();
  if (triggerQueue.get().has<TriggerQueue>()) {
    TriggerQueue &tq = triggerQueue.get().get<TriggerQueue>();
    tq.clear();
  }

  // Clean up BattleResult singleton if it exists (safe to remove)
  auto battleResult = afterhours::EntityHelper::get_singleton<BattleResult>();
  if (battleResult.get().has<BattleResult>()) {
    battleResult.get().cleanup = true;
  }

  // Don't clean up BattleLoadRequest - it will be updated by the next test
  // Don't clean up CombatManager or BattleProcessorManager - they're permanent
  // singletons

  // Merge and cleanup entities
  afterhours::EntityHelper::merge_entity_arrays();
  afterhours::EntityHelper::cleanup();
}
} // namespace server
