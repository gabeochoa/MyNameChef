#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

namespace server {
struct BattleSerializer {
  static nlohmann::json serialize_battle_result(uint64_t seed,
                                                const std::string &opponent_id,
                                                const nlohmann::json &outcomes,
                                                const nlohmann::json &events,
                                                bool debug_mode = false);

  static std::string compute_checksum(const nlohmann::json &state);

  static nlohmann::json collect_battle_events();
  static nlohmann::json collect_battle_outcomes();
  static nlohmann::json collect_state_snapshot(bool debug_mode);
};
} // namespace server
