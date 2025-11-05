#include "battle_serializer.h"
#include "battle_simulator.h"
#include "../components/battle_result.h"
#include "../components/combat_queue.h"
#include "../components/combat_stats.h"
#include "../components/is_dish.h"
#include "../components/trigger_event.h"
#include "../components/dish_battle_state.h"
#include "../utils/battle_fingerprint.h"
#include <afterhours/ah.h>
#include <iomanip>
#include <sstream>
#include <magic_enum/magic_enum.hpp>
#include <algorithm>

namespace server {
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

nlohmann::json BattleSerializer::collect_battle_events(const BattleSimulator &simulator) {
  std::vector<BattleEvent> events = simulator.get_accumulated_events();

  std::sort(events.begin(), events.end(),
            [](const BattleEvent &a, const BattleEvent &b) {
              return a.timestamp < b.timestamp;
            });

  nlohmann::json events_array = nlohmann::json::array();

  for (const BattleEvent &ev : events) {
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
  afterhours::RefEntity result_entity =
      afterhours::EntityHelper::get_singleton<BattleResult>();
  if (!result_entity.get().has<BattleResult>()) {
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

  for (afterhours::Entity &entity :
       afterhours::EntityQuery()
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
