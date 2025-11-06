#include "battle_serializer.h"
#include "../components/battle_load_request.h"
#include "../components/battle_result.h"
#include "../components/battle_team_tags.h"
#include "../components/combat_queue.h"
#include "../components/combat_stats.h"
#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../components/trigger_event.h"
#include "../dish_types.h"
#include "../utils/battle_fingerprint.h"
#include "async/battle_event.h"
#include "battle_simulator.h"
#include <afterhours/ah.h>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <magic_enum/magic_enum.hpp>
#include <sstream>

namespace server {
namespace {
// Helper to ensure BattleResult exists - creates it if missing
void ensure_battle_result_exists() {
  const auto componentId = afterhours::components::get_type_id<BattleResult>();
  bool singletonExists =
      afterhours::EntityHelper::get().singletonMap.contains(componentId);

  if (singletonExists) {
    return; // Already exists
  }

  BattleResult result;

  // In server context, dishes aren't instantiated as entities, so we need to
  // calculate from the team data stored in BattleLoadRequest
  auto reqEnt = afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
  if (!reqEnt.get().has<BattleLoadRequest>()) {
    log_error("ensure_battle_result_exists: No BattleLoadRequest found");
    result.outcome = BattleResult::Outcome::Tie;
    result.ties = 1;
    return;
  }

  auto &req = reqEnt.get().get<BattleLoadRequest>();
  int playerTeamScore = 0;
  int opponentTeamScore = 0;

  // Calculate player score from team file
  if (!req.playerJsonPath.empty()) {
    try {
      std::ifstream playerFile(req.playerJsonPath);
      nlohmann::json playerTeam;
      playerFile >> playerTeam;
      if (playerTeam.contains("team") && playerTeam["team"].is_array()) {
        for (const auto &dish : playerTeam["team"]) {
          if (dish.contains("dishType")) {
            std::string dishTypeStr = dish["dishType"];
            DishType dishType = magic_enum::enum_cast<DishType>(dishTypeStr)
                                    .value_or(DishType::Potato);
            DishInfo dishInfo = get_dish_info(dishType);
            playerTeamScore +=
                dishInfo.flavor.satiety + dishInfo.flavor.sweetness +
                dishInfo.flavor.spice + dishInfo.flavor.acidity +
                dishInfo.flavor.umami + dishInfo.flavor.richness +
                dishInfo.flavor.freshness;
          }
        }
      }
    } catch (...) {
      // Fall back to tie
      result.outcome = BattleResult::Outcome::Tie;
      result.ties = 1;
      return;
    }
  }

  // Calculate opponent score from team file
  if (!req.opponentJsonPath.empty()) {
    try {
      std::ifstream opponentFile(req.opponentJsonPath);
      nlohmann::json opponentTeam;
      opponentFile >> opponentTeam;
      if (opponentTeam.contains("team") && opponentTeam["team"].is_array()) {
        for (const auto &dish : opponentTeam["team"]) {
          if (dish.contains("dishType")) {
            std::string dishTypeStr = dish["dishType"];
            DishType dishType = magic_enum::enum_cast<DishType>(dishTypeStr)
                                    .value_or(DishType::Potato);
            DishInfo dishInfo = get_dish_info(dishType);
            opponentTeamScore +=
                dishInfo.flavor.satiety + dishInfo.flavor.sweetness +
                dishInfo.flavor.spice + dishInfo.flavor.acidity +
                dishInfo.flavor.umami + dishInfo.flavor.richness +
                dishInfo.flavor.freshness;
          }
        }
      }
    } catch (...) {
      // Fall back to tie
      result.outcome = BattleResult::Outcome::Tie;
      result.ties = 1;
      return;
    }
  }

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
}
} // namespace

nlohmann::json BattleSerializer::serialize_battle_result(
    uint64_t seed, const std::string &opponent_id,
    const nlohmann::json &outcomes, const nlohmann::json &events,
    bool debug_mode) {

  nlohmann::json result = {{"seed", seed},
                           {"opponentId", opponent_id},
                           {"outcomes", outcomes},
                           {"events", events},
                           {"debug", debug_mode}};

  if (debug_mode) {
    result["snapshots"] = collect_state_snapshot(true);
  }

  result["checksum"] = compute_checksum(result);

  return result;
}

std::string BattleSerializer::compute_checksum(const nlohmann::json &state) {
  uint64_t fp = BattleFingerprint::compute();

  std::stringstream ss;
  ss << std::hex << std::setfill('0') << std::setw(16) << fp;
  return ss.str();
}

nlohmann::json
BattleSerializer::collect_battle_events(const BattleSimulator &simulator) {
  std::vector<async::DebugBattleEvent> events =
      simulator.get_accumulated_events();

  std::sort(
      events.begin(), events.end(),
      [](const async::DebugBattleEvent &a, const async::DebugBattleEvent &b) {
        return a.timestamp < b.timestamp;
      });

  nlohmann::json events_array = nlohmann::json::array();

  for (const async::DebugBattleEvent &ev : events) {
    nlohmann::json event_json;
    event_json["hook"] = std::string(magic_enum::enum_name(ev.hook));
    event_json["sourceEntityId"] = ev.sourceEntityId;
    event_json["slotIndex"] = ev.slotIndex;

    std::string team_side_str;
    switch (ev.teamSide) {
    case DishBattleState::TeamSide::Player:
      team_side_str = "Player";
      break;
    case DishBattleState::TeamSide::Opponent:
      team_side_str = "Opponent";
      break;
    }
    event_json["teamSide"] = team_side_str;

    event_json["timestamp"] = ev.timestamp;
    event_json["courseIndex"] = ev.courseIndex;
    event_json["payloadInt"] = ev.payloadInt;
    event_json["payloadFloat"] = ev.payloadFloat;

    events_array.push_back(event_json);
  }

  return events_array;
}

nlohmann::json BattleSerializer::collect_battle_outcomes() {
  // Ensure BattleResult exists before collecting
  ensure_battle_result_exists();

  const auto componentId = afterhours::components::get_type_id<BattleResult>();
  bool singletonExists =
      afterhours::EntityHelper::get().singletonMap.contains(componentId);

  if (!singletonExists) {
    log_warn("collect_battle_outcomes: BattleResult singleton not found after "
             "ensure - "
             "battle may not have completed");
    return nlohmann::json::array();
  }

  afterhours::RefEntity result_entity =
      afterhours::EntityHelper::get_singleton<BattleResult>();

  bool hasComponent = result_entity.get().has<BattleResult>();

  if (!hasComponent) {
    log_warn("collect_battle_outcomes: Entity exists but missing BattleResult "
             "component");
    return nlohmann::json::array();
  }

  const BattleResult &result = result_entity.get().get<BattleResult>();
  nlohmann::json outcomes = nlohmann::json::array();

  for (const auto &outcome : result.outcomes) {
    nlohmann::json course_outcome = {{"slotIndex", outcome.slotIndex},
                                     {"ticks", outcome.ticks}};

    std::string winner_str;
    switch (outcome.winner) {
    case BattleResult::CourseOutcome::Winner::Player:
      winner_str = "Player";
      break;
    case BattleResult::CourseOutcome::Winner::Opponent:
      winner_str = "Opponent";
      break;
    case BattleResult::CourseOutcome::Winner::Tie:
      winner_str = "Tie";
      break;
    }
    course_outcome["winner"] = winner_str;

    outcomes.push_back(course_outcome);
  }

  return outcomes;
}

nlohmann::json BattleSerializer::collect_state_snapshot(bool debug_mode) {
  if (!debug_mode) {
    return nlohmann::json::array();
  }

  nlohmann::json snapshots = nlohmann::json::array();

  for (afterhours::Entity &entity : afterhours::EntityQuery()
                                        .whereHasComponent<IsDish>()
                                        .whereHasComponent<DishBattleState>()
                                        .whereHasComponent<CombatStats>()
                                        .gen()) {
    const IsDish &dish = entity.get<IsDish>();
    const DishBattleState &dbs = entity.get<DishBattleState>();
    const CombatStats &cs = entity.get<CombatStats>();

    nlohmann::json snapshot;
    snapshot["entityId"] = entity.id;
    snapshot["dishType"] = dish.name();
    snapshot["currentZing"] = cs.currentZing;
    snapshot["currentBody"] = cs.currentBody;
    snapshot["baseZing"] = cs.baseZing;
    snapshot["baseBody"] = cs.baseBody;

    std::string team_side_str;
    switch (dbs.team_side) {
    case DishBattleState::TeamSide::Player:
      team_side_str = "Player";
      break;
    case DishBattleState::TeamSide::Opponent:
      team_side_str = "Opponent";
      break;
    }
    snapshot["teamSide"] = team_side_str;

    snapshot["slotIndex"] = dbs.queue_index;

    std::string phase_str;
    switch (dbs.phase) {
    case DishBattleState::Phase::InQueue:
      phase_str = "InQueue";
      break;
    case DishBattleState::Phase::Entering:
      phase_str = "Entering";
      break;
    case DishBattleState::Phase::InCombat:
      phase_str = "InCombat";
      break;
    case DishBattleState::Phase::Finished:
      phase_str = "Finished";
      break;
    }
    snapshot["phase"] = phase_str;

    snapshots.push_back(snapshot);
  }

  return snapshots;
}
} // namespace server
