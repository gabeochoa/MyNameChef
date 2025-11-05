#include "battle_serializer.h"
#include "../components/battle_result.h"
#include "../components/combat_queue.h"
#include "../utils/battle_fingerprint.h"
#include <afterhours/ah.h>
#include <iomanip>
#include <sstream>

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

nlohmann::json BattleSerializer::collect_battle_events() {
  return nlohmann::json::array();
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
  return nlohmann::json::array();
}
} // namespace server
