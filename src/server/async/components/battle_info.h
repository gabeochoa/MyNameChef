#pragma once

#include "../types.h"
#include <afterhours/ah.h>
#include <nlohmann/json.hpp>

namespace server::async {
enum class BattleStatus { Queued, Running, Complete, Error };

struct BattleInfo : afterhours::BaseComponent {
  BattleId battleId;
  TeamId opponentId;
  TeamId playerTeamId;
  UserId playerUserId;
  BattleStatus status;
  int round;
  int shopTier;
  uint64_t seed;
  nlohmann::json result;

  BattleInfo() : status(BattleStatus::Queued), round(0), shopTier(0), seed(0) {}
};
} // namespace server::async
