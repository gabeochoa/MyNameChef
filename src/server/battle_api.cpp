#include "battle_api.h"
#include "battle_serializer.h"
#include "file_storage.h"
#include "../log.h"
#include "../seeded_rng.h"
#include "team_types.h"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace server {
static nlohmann::json make_error_json(const std::string &error) {
  return nlohmann::json{{"error", error}};
}

void BattleAPI::setup_routes() {
  server.set_default_headers(
      {{"Access-Control-Allow-Origin", "*"},
       {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
       {"Access-Control-Allow-Headers", "Content-Type"}});

  server.Get("/health",
             [this](const httplib::Request &req, httplib::Response &res) {
               handle_health_request(req, res);
             });

  server.Post("/battle",
              [this](const httplib::Request &req, httplib::Response &res) {
                handle_battle_request(req, res);
              });

  server.Options("/health", [](const httplib::Request &,
                               httplib::Response &res) { res.status = 200; });

  server.Options("/battle", [](const httplib::Request &,
                               httplib::Response &res) { res.status = 200; });
}

void BattleAPI::handle_health_request(const httplib::Request &,
                                      httplib::Response &res) {
  nlohmann::json response = {{"status", "ok"}};
  res.set_content(response.dump(), "application/json");
  res.status = 200;
}

void BattleAPI::handle_battle_request(const httplib::Request &req,
                                      httplib::Response &res) {
  try {
    nlohmann::json request_json = nlohmann::json::parse(req.body);

    if (!TeamManager::validate_team_json(request_json)) {
      res.status = 400;
      res.set_content(make_error_json("Invalid team JSON format").dump(),
                      "application/json");
      return;
    }

    nlohmann::json player_team = request_json["team"];

    std::optional<TeamFilePath> opponent_path =
        TeamManager::select_random_opponent();
    if (!opponent_path.has_value()) {
      res.status = 500;
      res.set_content(make_error_json("No opponents available").dump(),
                      "application/json");
      return;
    }

    nlohmann::json opponent_team =
        TeamManager::load_team_from_file(opponent_path.value());
    if (opponent_team.empty()) {
      res.status = 500;
      res.set_content(make_error_json("Failed to load opponent team").dump(),
                      "application/json");
      return;
    }

    // Explicitly not using SeededRng here because
    // this is the real seed generation for the battle
    std::random_device rd;
    uint64_t seed = static_cast<uint64_t>(rd()) << 32 | rd();

    const float fixed_dt = 1.0f / 60.0f;
    int max_iterations = 100000;
    int iterations = 0;

    BattleSimulator simulator;
    simulator.start_battle(player_team, opponent_team, seed);

    while (!simulator.is_complete() && iterations < max_iterations) {
      simulator.update(fixed_dt);
      iterations++;
    }

    if (iterations >= max_iterations) {
      res.status = 500;
      res.set_content(make_error_json("Battle simulation timeout").dump(),
                      "application/json");
      return;
    }

    bool debug_mode = request_json.value("debug", false);
    nlohmann::json outcomes = BattleSerializer::collect_battle_outcomes();
    nlohmann::json events = BattleSerializer::collect_battle_events(simulator);

    std::filesystem::path opp_path(opponent_path.value());
    TeamId opponent_id = extract_team_id_from_path(opponent_path.value());

    nlohmann::json response = BattleSerializer::serialize_battle_result(
        seed, opponent_id, outcomes, events, debug_mode);

    std::string player_team_id = request_json.value("playerTeamId", "");
    std::string player_username = request_json.value("playerUsername", "");
    std::string opponent_username = request_json.value("opponentUsername", "");

    std::string battle_id = std::to_string(seed);

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    std::string timestamp = ss.str();

    nlohmann::json result_to_save = response;
    result_to_save["playerTeamId"] = player_team_id;
    result_to_save["opponentTeamId"] = opponent_id;
    result_to_save["playerUsername"] = player_username;
    result_to_save["opponentUsername"] = opponent_username;

    std::string results_dir = "output/battles/results";
    FileStorage::ensure_directory_exists(results_dir);

    std::string result_filename =
        results_dir + "/" + timestamp + "_" + battle_id + ".json";
    FileStorage::save_json_to_file(result_filename, result_to_save);

    FileStorage::cleanup_old_files(results_dir, 10, ".json");

    res.status = 200;
    res.set_content(response.dump(), "application/json");

  } catch (const nlohmann::json::exception &e) {
    res.status = 400;
    nlohmann::json error = {{"error", "Invalid JSON"}, {"details", e.what()}};
    res.set_content(error.dump(), "application/json");
  } catch (const std::exception &e) {
    res.status = 500;
    nlohmann::json error = {{"error", "Server error"}, {"details", e.what()}};
    res.set_content(error.dump(), "application/json");
    log_error("Battle API error: {}", e.what());
  }
}

void BattleAPI::start(int port) {
  log_info("Starting battle server on port {}", port);
  setup_routes();

  if (!server.listen("0.0.0.0", port)) {
    log_error("Failed to start server on port {}", port);
  }
}

void BattleAPI::stop() { server.stop(); }
} // namespace server
