#include "command_queue.h"
#include <afterhours/ah.h>

namespace server::async {
CommandQueue &CommandQueue::get() {
  static CommandQueue q;
  return q;
}

void CommandQueue::enqueue_add_team(const TeamId &team_id,
                                    const UserId &user_id, int round,
                                    int shop_tier, const nlohmann::json &team) {
  std::lock_guard<std::mutex> lock(mtx);
  auto &entity = afterhours::EntityHelper::createEntity();
  auto &cmd = entity.addComponent<CommandQueueEntry>();
  cmd.type = CommandType::AddTeam;
  cmd.teamId = team_id;
  cmd.userId = user_id;
  cmd.round = round;
  cmd.shopTier = shop_tier;
  cmd.team = team;
}

void CommandQueue::enqueue_match_request(const BattleId &battle_id,
                                         const TeamId &team_id,
                                         const UserId &user_id, int round,
                                         int shop_tier,
                                         const nlohmann::json &team) {
  std::lock_guard<std::mutex> lock(mtx);
  auto &entity = afterhours::EntityHelper::createEntity();
  auto &cmd = entity.addComponent<CommandQueueEntry>();
  cmd.type = CommandType::MatchRequest;
  cmd.battleId = battle_id;
  cmd.teamId = team_id;
  cmd.userId = user_id;
  cmd.round = round;
  cmd.shopTier = shop_tier;
  cmd.team = team;
}

void CommandQueue::enqueue_session_request(const BattleId &battle_id,
                                           const nlohmann::json &player_team,
                                           const nlohmann::json &opponent_team,
                                           const TeamId &opponent_id,
                                           const TeamId &player_team_id,
                                           const UserId &player_user_id,
                                           int round, int shop_tier) {
  std::lock_guard<std::mutex> lock(mtx);
  auto &entity = afterhours::EntityHelper::createEntity();
  auto &cmd = entity.addComponent<CommandQueueEntry>();
  cmd.type = CommandType::BattleSessionRequest;
  cmd.battleId = battle_id;
  cmd.opponentId = opponent_id;
  cmd.playerTeamId = player_team_id;
  cmd.playerUserId = player_user_id;
  cmd.round = round;
  cmd.shopTier = shop_tier;
  cmd.playerTeam = player_team;
  cmd.opponentTeam = opponent_team;
}
} // namespace server::async
