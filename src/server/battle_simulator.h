#pragma once

#include "../seeded_rng.h"
#include "server_context.h"
#include "../components/trigger_event.h"
#include "../components/dish_battle_state.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace server {
struct BattleEvent {
  TriggerHook hook;
  int sourceEntityId;
  int slotIndex;
  DishBattleState::TeamSide teamSide;
  float timestamp;
  int courseIndex;
  int payloadInt;
  float payloadFloat;
};

struct BattleSimulator {
  ServerContext ctx;
  uint64_t seed;
  bool battle_active;
  float simulation_time;
  std::vector<BattleEvent> accumulated_events;

  BattleSimulator();

  void start_battle(const nlohmann::json &player_team_json,
                    const nlohmann::json &opponent_team_json,
                    uint64_t battle_seed);

  void update(float dt);

  bool is_complete() const;

  nlohmann::json get_battle_state() const;

  std::vector<BattleEvent> get_accumulated_events() const {
    return accumulated_events;
  }

private:
  void track_events(float timestamp, int course_index);
  void create_battle_result();
  void ensure_battle_result();
};
} // namespace server
