#pragma once

#include <deque>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace server {
struct AddTeamCmd {
  std::string teamId;
  std::string userId;
  int round;
  int shopTier;
  std::string band;
  nlohmann::json team;
};

struct MatchRequestCmd {
  std::string battleId;
  std::string teamId;
  std::string userId;
  int round;
  int shopTier;
  std::string band;
  nlohmann::json team;
};

struct BattleSessionRequestCmd {
  std::string battleId;
  nlohmann::json playerTeam;
  nlohmann::json opponentTeam;
  std::string opponentId;
};

class CommandQueue {
public:
  static CommandQueue &get();

  void enqueue_add_team(const AddTeamCmd &cmd);
  bool try_pop_add_team(AddTeamCmd &out);

  void enqueue_match_request(const MatchRequestCmd &cmd);
  bool try_pop_match_request(MatchRequestCmd &out);

  void enqueue_session_request(const BattleSessionRequestCmd &cmd);
  bool try_pop_session_request(BattleSessionRequestCmd &out);

private:
  std::mutex mtx;
  std::deque<AddTeamCmd> add_team_q;
  std::deque<MatchRequestCmd> match_req_q;
  std::deque<BattleSessionRequestCmd> session_req_q;
};
} // namespace server
