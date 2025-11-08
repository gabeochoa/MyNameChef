#pragma once

#include "battle_serializer.h"
#include "battle_simulator.h"
#include "server_config.h"
#include "team_manager.h"
#include <httplib.h>
#include <string>

namespace server {
struct BattleAPI {
  httplib::Server server;
  ServerConfig config;

  BattleAPI(const ServerConfig &cfg);

  void setup_routes();
  void start(int port);
  void stop();

private:
  void handle_battle_request(const httplib::Request &req,
                             httplib::Response &res);
  void handle_health_request(const httplib::Request &req,
                             httplib::Response &res);
  void handle_save_game_state(const httplib::Request &req,
                              httplib::Response &res);
  void handle_get_game_state(const httplib::Request &req,
                             httplib::Response &res);
  std::string get_error_message(const std::string &detailed_error) const;
  std::string compute_game_state_checksum(const nlohmann::json &state) const;
};
} // namespace server
