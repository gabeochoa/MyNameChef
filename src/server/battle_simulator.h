#pragma once

#include "../seeded_rng.h"
#include "server_context.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace server {
struct BattleSimulator {
  ServerContext ctx;
  uint64_t seed;
  bool battle_active;
  float simulation_time;

  BattleSimulator();

  void start_battle(const nlohmann::json &player_team_json,
                    const nlohmann::json &opponent_team_json,
                    uint64_t battle_seed);

  void update(float dt);

  bool is_complete() const;

  nlohmann::json get_battle_state() const;
};
} // namespace server
