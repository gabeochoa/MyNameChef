#pragma once

#include "../seeded_rng.h"
#include "async/battle_event.h"
#include "server_context.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace server {
struct BattleSimulator {
  ServerContext ctx;
  uint64_t seed;
  bool battle_active;
  float simulation_time;
  std::vector<async::DebugBattleEvent> accumulated_events;

  BattleSimulator();

  void start_battle(const nlohmann::json &player_team_json,
                    const nlohmann::json &opponent_team_json,
                    uint64_t battle_seed,
                    const std::filesystem::path &temp_files_path);

  void update(float dt);

  bool is_complete() const;

  float get_simulation_time() const { return simulation_time; }

  nlohmann::json get_battle_state() const;

  std::vector<async::DebugBattleEvent> get_accumulated_events() const {
    return accumulated_events;
  }

  // Static cleanup function for tests - cleans up all battle entities
  static void cleanup_test_entities();

private:
  void track_events(float timestamp, int course_index);
  void create_battle_result();
  void ensure_battle_result();
};
} // namespace server
