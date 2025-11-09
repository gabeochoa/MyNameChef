#pragma once

#include "../types.h"
#include <afterhours/ah.h>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace server::async {
struct TeamPoolEntry : afterhours::BaseComponent {
  TeamId teamId;
  UserId userId;
  int round;
  int shopTier;
  nlohmann::json team;
  uint64_t addedAt;
  nlohmann::json gameState;
  std::string gameStateChecksum;
  uint64_t lastSaved;

  TeamPoolEntry() : round(0), shopTier(0), addedAt(0), lastSaved(0) {}

  TeamPoolEntry(const TeamId &team_id, const UserId &user_id, int r, int tier,
                const nlohmann::json &team_data, uint64_t added_at)
      : teamId(team_id), userId(user_id), round(r), shopTier(tier),
        team(team_data), addedAt(added_at), lastSaved(0) {}
};
} // namespace server::async
