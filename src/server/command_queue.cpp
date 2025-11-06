#include "command_queue.h"

namespace server {
CommandQueue &CommandQueue::get() {
  static CommandQueue q;
  return q;
}

void CommandQueue::enqueue_add_team(const AddTeamCmd &cmd) {
  std::lock_guard<std::mutex> lock(mtx);
  add_team_q.push_back(cmd);
}

bool CommandQueue::try_pop_add_team(AddTeamCmd &out) {
  std::lock_guard<std::mutex> lock(mtx);
  if (add_team_q.empty())
    return false;
  out = add_team_q.front();
  add_team_q.pop_front();
  return true;
}

void CommandQueue::enqueue_match_request(const MatchRequestCmd &cmd) {
  std::lock_guard<std::mutex> lock(mtx);
  match_req_q.push_back(cmd);
}

bool CommandQueue::try_pop_match_request(MatchRequestCmd &out) {
  std::lock_guard<std::mutex> lock(mtx);
  if (match_req_q.empty())
    return false;
  out = match_req_q.front();
  match_req_q.pop_front();
  return true;
}

void CommandQueue::enqueue_session_request(const BattleSessionRequestCmd &cmd) {
  std::lock_guard<std::mutex> lock(mtx);
  session_req_q.push_back(cmd);
}

bool CommandQueue::try_pop_session_request(BattleSessionRequestCmd &out) {
  std::lock_guard<std::mutex> lock(mtx);
  if (session_req_q.empty())
    return false;
  out = session_req_q.front();
  session_req_q.pop_front();
  return true;
}
} // namespace server
