#pragma once

#include "../types.h"
#include <afterhours/ah.h>
#include <nlohmann/json.hpp>

namespace server::async {
enum class CommandType { AddTeam, MatchRequest, BattleSessionRequest };

struct CommandQueueEntry : afterhours::BaseComponent {
  CommandType type;
  BattleId battleId; // For MatchRequest and BattleSessionRequest
  TeamId teamId;
  TeamId opponentId; // For BattleSessionRequest
  TeamId playerTeamId; // For BattleSessionRequest
  UserId userId; // For AddTeam and MatchRequest
  UserId playerUserId; // For BattleSessionRequest
  int round;
  int shopTier;
  nlohmann::json team; // For AddTeam and MatchRequest
  nlohmann::json playerTeam; // For BattleSessionRequest
  nlohmann::json opponentTeam; // For BattleSessionRequest

  CommandQueueEntry()
      : type(CommandType::AddTeam), round(0), shopTier(0) {}
};
} // namespace server::async

