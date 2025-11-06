#pragma once

#include "components/command_queue_entry.h"
#include "types.h"
#include <mutex>
#include <nlohmann/json.hpp>

namespace server::async {
class CommandQueue {
public:
  static CommandQueue &get();

  // Enqueue commands by creating entities
  void enqueue_add_team(const TeamId &team_id, const UserId &user_id, int round,
                        int shop_tier, const nlohmann::json &team);
  void enqueue_match_request(const BattleId &battle_id, const TeamId &team_id,
                             const UserId &user_id, int round, int shop_tier,
                             const nlohmann::json &team);
  void enqueue_session_request(const BattleId &battle_id,
                               const nlohmann::json &player_team,
                               const nlohmann::json &opponent_team,
                               const TeamId &opponent_id,
                               const TeamId &player_team_id,
                               const UserId &player_user_id, int round,
                               int shop_tier);

private:
  std::mutex mtx; // Protect entity creation
};
} // namespace server::async
