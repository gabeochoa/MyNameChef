#pragma once

#include "battle_serializer.h"
#include "battle_simulator.h"
#include "team_manager.h"
#include <httplib.h>
#include <string>

namespace server {
struct BattleAPI {
  httplib::Server server;

  void setup_routes();
  void start(int port);
  void stop();

private:
  void handle_battle_request(const httplib::Request &req,
                             httplib::Response &res);
  void handle_health_request(const httplib::Request &req,
                             httplib::Response &res);
};
} // namespace server
