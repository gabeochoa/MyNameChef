#pragma once

#include "../../../components/battle_team_data.h"
#include "../../../components/combat_queue.h"
#include "../../../dish_types.h"
#include "../../../log.h"
#include "../../../seeded_rng.h"
#include "../components/battle_info.h"
#include "../components/command_queue_entry.h"
#include "../components/team_pool.h"
#include <afterhours/ah.h>
#include <chrono>
#include <cstdint>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <random>

namespace server::async {
struct ProcessCommandQueueSystem : afterhours::System<CommandQueueEntry> {
  void for_each_with(afterhours::Entity &cmd_entity, CommandQueueEntry &cmd,
                     float) override {
    switch (cmd.type) {
    case CommandType::AddTeam:
      process_add_team(cmd);
      break;
    case CommandType::MatchRequest:
      process_match_request(cmd_entity, cmd);
      break;
    case CommandType::BattleSessionRequest:
      process_battle_session_request(cmd);
      break;
    }

    cmd_entity.cleanup = true;
  }

private:
  void process_add_team(const CommandQueueEntry &cmd) {
    uint64_t added_at = std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();

    auto &team_entity = afterhours::EntityHelper::createEntity();
    team_entity.addComponent<TeamPoolEntry>(cmd.teamId, cmd.userId, cmd.round,
                                            cmd.shopTier, cmd.team, added_at);

    log_info("SERVER_MATCHMAKING: Added team {} for user {} to pool "
             "(round={}, tier={})",
             cmd.teamId, cmd.userId, cmd.round, cmd.shopTier);
  }

  void process_match_request(afterhours::Entity &cmd_entity,
                             const CommandQueueEntry &cmd) {
    // Define reusable filters as lambdas
    auto user_filter = [&cmd](const afterhours::Entity &e) {
      const TeamPoolEntry &entry = e.get<TeamPoolEntry>();
      return entry.userId != cmd.userId;
    };

    auto match_filter = [&cmd](const afterhours::Entity &e) {
      const TeamPoolEntry &entry = e.get<TeamPoolEntry>();
      int round_diff = std::abs(entry.round - cmd.round);
      int tier_diff = std::abs(entry.shopTier - cmd.shopTier);
      return round_diff <= 2 && tier_diff <= 1;
    };

    // Find random opponent using progressive filtering
    SeededRng &rng = SeededRng::get();
    auto opponent_opt =
        afterhours::EntityQuery({.force_merge = true})
            .whereHasComponent<TeamPoolEntry>()
            .whereLambda(user_filter)
            .whereLambda(match_filter)
            .gen_random([&rng](size_t count) { return rng.gen_index(count); });

    if (!opponent_opt) {
      log_warn("SERVER_MATCHMAKING: No matching opponent found for battle {} "
               "(user={}, round={}, tier={})",
               cmd.battleId, cmd.userId, cmd.round, cmd.shopTier);
      cmd_entity.cleanup = true;
      return;
    }

    afterhours::Entity &opponent_entity = opponent_opt.asE();
    const TeamPoolEntry &opponent = opponent_entity.get<TeamPoolEntry>();

    // Get player team data using progressive filtering
    nlohmann::json player_team = cmd.team;
    if (player_team.empty() && !cmd.teamId.empty()) {
      auto player_opt =
          afterhours::EntityQuery({.force_merge = true})
              .whereHasComponent<TeamPoolEntry>()
              .whereLambda([&cmd](const afterhours::Entity &e) {
                const TeamPoolEntry &entry = e.get<TeamPoolEntry>();
                return entry.teamId == cmd.teamId && entry.userId == cmd.userId;
              })
              .gen_first();

      if (player_opt) {
        player_team = player_opt.asE().get<TeamPoolEntry>().team;
      }
    }

    if (player_team.empty()) {
      log_warn("SERVER_MATCHMAKING: Could not find player team data for "
               "battle {}",
               cmd.battleId);
      cmd_entity.cleanup = true;
      return;
    }

    log_info("SERVER_MATCHMAKING: Matched battle {} - player {} vs opponent {} "
             "(team {})",
             cmd.battleId, cmd.userId, opponent.userId, opponent.teamId);

    // Start battle directly
    start_battle(cmd.battleId, player_team, opponent.team, opponent.teamId,
                 cmd.teamId, cmd.userId, cmd.round, cmd.shopTier);
  }

  void process_battle_session_request(const CommandQueueEntry &cmd) {
    start_battle(cmd.battleId, cmd.playerTeam, cmd.opponentTeam, cmd.opponentId,
                 cmd.playerTeamId, cmd.playerUserId, cmd.round, cmd.shopTier);
  }

  void start_battle(const BattleId &battle_id,
                    const nlohmann::json &player_team_json,
                    const nlohmann::json &opponent_team_json,
                    const TeamId &opponent_id, const TeamId &player_team_id,
                    const UserId &player_user_id, int round, int shop_tier) {
    auto manager_ref = afterhours::EntityHelper::get_singleton<CombatQueue>();
    auto &manager_entity = manager_ref.get();

    // Setup battle team data
    if (!manager_entity.has<BattleTeamDataPlayer>()) {
      manager_entity.addComponent<BattleTeamDataPlayer>();
    }
    if (!manager_entity.has<BattleTeamDataOpponent>()) {
      manager_entity.addComponent<BattleTeamDataOpponent>();
    }

    auto &player_data = manager_entity.get<BattleTeamDataPlayer>();
    auto &opponent_data = manager_entity.get<BattleTeamDataOpponent>();

    player_data.team.clear();
    opponent_data.team.clear();
    player_data.instantiated = false;
    opponent_data.instantiated = false;

    // Parse player team
    if (player_team_json.contains("team") &&
        player_team_json["team"].is_array()) {
      for (const auto &dish : player_team_json["team"]) {
        TeamDishSpec spec;
        if (dish.contains("slot")) {
          spec.slot = dish["slot"];
        }
        if (dish.contains("dishType")) {
          std::string dishTypeStr = dish["dishType"];
          auto dishTypeOpt = magic_enum::enum_cast<DishType>(dishTypeStr);
          if (dishTypeOpt.has_value()) {
            spec.dishType = dishTypeOpt.value();
          }
        }
        if (dish.contains("level")) {
          spec.level = dish["level"];
        } else {
          spec.level = 1;
        }
        if (dish.contains("powerups") && dish["powerups"].is_array()) {
          for (const auto &p : dish["powerups"]) {
            if (p.is_number()) {
              spec.powerups.push_back(p.get<int>());
            }
          }
        }
        player_data.team.push_back(spec);
      }
    }

    // Parse opponent team
    if (opponent_team_json.contains("team") &&
        opponent_team_json["team"].is_array()) {
      for (const auto &dish : opponent_team_json["team"]) {
        TeamDishSpec spec;
        if (dish.contains("slot")) {
          spec.slot = dish["slot"];
        }
        if (dish.contains("dishType")) {
          std::string dishTypeStr = dish["dishType"];
          auto dishTypeOpt = magic_enum::enum_cast<DishType>(dishTypeStr);
          if (dishTypeOpt.has_value()) {
            spec.dishType = dishTypeOpt.value();
          }
        }
        if (dish.contains("level")) {
          spec.level = dish["level"];
        } else {
          spec.level = 1;
        }
        if (dish.contains("powerups") && dish["powerups"].is_array()) {
          for (const auto &p : dish["powerups"]) {
            if (p.is_number()) {
              spec.powerups.push_back(p.get<int>());
            }
          }
        }
        opponent_data.team.push_back(spec);
      }
    }

    // Generate unique seed for this battle (non-deterministic, one-time)
    std::random_device rd;
    uint64_t battle_seed = static_cast<uint64_t>(rd()) << 32 | rd();

    // Set seed for deterministic battle simulation
    SeededRng::get().set_seed(battle_seed);

    // Create battle info entity
    auto &battle_entity = afterhours::EntityHelper::createEntity();
    auto &battle_info = battle_entity.addComponent<BattleInfo>();
    battle_info.battleId = battle_id;
    battle_info.opponentId = opponent_id;
    battle_info.playerTeamId = player_team_id;
    battle_info.playerUserId = player_user_id;
    battle_info.round = round;
    battle_info.shopTier = shop_tier;
    battle_info.seed = battle_seed;
    battle_info.status = BattleStatus::Running;
    battle_info.result = nlohmann::json::object();

    log_info(
        "SERVER_BATTLE: Starting battle {} (opponent={}, round={}, tier={})",
        battle_id, opponent_id, round, shop_tier);
  }
};
} // namespace server::async
