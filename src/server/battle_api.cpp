#include "battle_api.h"
#include "../log.h"
#include "../seeded_rng.h"
#include "../utils/battle_fingerprint.h"
#include "async/components/team_pool.h"
#include "battle_serializer.h"
#include "file_storage.h"
#include "team_types.h"
#include <afterhours/ah.h>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>

namespace server {
BattleAPI::BattleAPI(const ServerConfig &cfg) : config(cfg) {}

static nlohmann::json make_error_json(const std::string &error) {
  return nlohmann::json{{"error", error}};
}

static void set_error_response(httplib::Response &res, int status_code,
                               const std::string &error_message) {
  res.status = status_code;
  res.set_content(make_error_json(error_message).dump(), "application/json");
}

#define return_if(condition, status_code, error_message)                       \
  do {                                                                         \
    if (condition) {                                                           \
      set_error_response(res, status_code, error_message);                     \
      return;                                                                  \
    }                                                                          \
  } while (0)

std::string
BattleAPI::get_error_message(const std::string &detailed_error) const {
  if (config.error_detail_level == "trace" ||
      config.error_detail_level == "info") {
    return detailed_error;
  }
  return "Internal server error";
}

void BattleAPI::setup_routes() {
  if (config.enable_cors) {
    server.set_default_headers(
        {{"Access-Control-Allow-Origin", config.cors_origin},
         {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
         {"Access-Control-Allow-Headers", "Content-Type"}});
  }

  server.Get("/health",
             [this](const httplib::Request &req, httplib::Response &res) {
               handle_health_request(req, res);
             });

  server.Post("/battle",
              [this](const httplib::Request &req, httplib::Response &res) {
                handle_battle_request(req, res);
              });

  server.Post("/save-game-state",
              [this](const httplib::Request &req, httplib::Response &res) {
                handle_save_game_state(req, res);
              });

  server.Get("/game-state",
             [this](const httplib::Request &req, httplib::Response &res) {
               handle_get_game_state(req, res);
             });

  server.Options("/health", [](const httplib::Request &,
                               httplib::Response &res) { res.status = 200; });

  server.Options("/battle", [](const httplib::Request &,
                               httplib::Response &res) { res.status = 200; });

  server.Options("/save-game-state",
                 [](const httplib::Request &, httplib::Response &res) {
                   res.status = 200;
                 });

  server.Options("/game-state",
                 [](const httplib::Request &, httplib::Response &res) {
                   res.status = 200;
                 });
}

void BattleAPI::handle_health_request(const httplib::Request &,
                                      httplib::Response &res) {
  std::string status = "healthy";
  std::vector<std::string> issues;

  std::filesystem::path opponents_path = config.get_opponents_path();
  size_t opponent_count = 0;
  if (!FileStorage::directory_exists(opponents_path.string())) {
    status = "unhealthy";
    issues.push_back("Opponents directory does not exist");
  } else {
    opponent_count =
        FileStorage::count_files_in_directory(opponents_path.string(), ".json");
    if (opponent_count == 0) {
      status = "degraded";
      issues.push_back("No opponent files available");
    }
  }

  std::filesystem::path temp_path = config.get_temp_files_path();
  if (!FileStorage::directory_exists(temp_path.string())) {
    try {
      FileStorage::ensure_directory_exists(temp_path.string());
    } catch (const std::exception &e) {
      status = "unhealthy";
      issues.push_back("Cannot create temp directory");
    }
  } else {
    if (!FileStorage::check_disk_space(temp_path.string(), 1048576)) {
      status = "degraded";
      issues.push_back("Low disk space in temp directory");
    }
  }

  std::filesystem::path results_path = config.get_results_path();
  if (!FileStorage::directory_exists(results_path.string())) {
    try {
      FileStorage::ensure_directory_exists(results_path.string());
    } catch (const std::exception &e) {
      status = "unhealthy";
      issues.push_back("Cannot create results directory");
    }
  } else {
    if (!FileStorage::check_disk_space(results_path.string(), 1048576)) {
      status = "degraded";
      issues.push_back("Low disk space in results directory");
    }
  }

  nlohmann::json response = {{"status", status}};
  if (!issues.empty()) {
    response["issues"] = issues;
  }
  response["opponent_count"] = opponent_count;

  res.set_content(response.dump(), "application/json");
  res.status = 200;
}

void BattleAPI::handle_battle_request(const httplib::Request &req,
                                      httplib::Response &res) {
  std::string request_id = std::to_string(
      std::chrono::steady_clock::now().time_since_epoch().count());
  auto request_start = std::chrono::steady_clock::now();

  log_info("[{}] Battle request received", request_id);

  BattleSimulator simulator;
  bool simulator_initialized = false;
  try {
    std::string content_type = req.get_header_value("Content-Type");
    return_if(content_type.find("application/json") == std::string::npos, 415,
              "Content-Type must be application/json");

    if (req.body.size() > static_cast<size_t>(config.max_request_body_size)) {
      std::string error_msg = "Request body too large. Maximum size: " +
                              std::to_string(config.max_request_body_size) +
                              " bytes";
      set_error_response(res, 413, error_msg);
      return;
    }

    return_if(req.body.empty(), 400, "Request body is empty");

    nlohmann::json request_json = nlohmann::json::parse(req.body);

    return_if(
        !TeamManager::validate_team_json(request_json, config.max_team_size),
        400, "Invalid team JSON format");

    nlohmann::json player_team = request_json["team"];

    std::optional<TeamFilePath> opponent_path =
        TeamManager::select_random_opponent_with_fallback(
            config.get_opponents_path(), config.file_operation_retries);
    return_if(!opponent_path.has_value(), 500, "No opponents available");

    nlohmann::json opponent_team = FileStorage::load_json_from_file_with_retry(
        opponent_path.value(), config.file_operation_retries);
    return_if(opponent_team.empty(), 500, "Failed to load opponent team");

    // Explicitly not using SeededRng here because
    // this is the real seed generation for the battle
    std::random_device rd;
    uint64_t seed = static_cast<uint64_t>(rd()) << 32 | rd();

    const float fixed_dt = 1.0f / 60.0f;
    int max_iterations =
        std::min(config.timeout_seconds * 60, config.max_simulation_iterations);
    int iterations = 0;
    auto start_time = std::chrono::steady_clock::now();

    simulator.start_battle(player_team, opponent_team, seed,
                           config.get_temp_files_path());
    simulator_initialized = true;

    int last_logged_iteration = 0;
    while (!simulator.is_complete() && iterations < max_iterations) {
      auto current_time = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                         current_time - start_time)
                         .count();

      if (iterations > 0 && iterations % 3600 == 0 &&
          iterations != last_logged_iteration) {
        log_info("[{}] Battle progress: {} iterations, {:.1f}s elapsed",
                 request_id, iterations, elapsed);
        last_logged_iteration = iterations;
      }

      if (elapsed >= config.timeout_seconds) {
        std::string battle_id = std::to_string(seed);
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        std::string timestamp = ss.str();

        nlohmann::json timeout_state = {
            {"seed", seed},
            {"simulation_time", simulator.get_simulation_time()},
            {"iterations", iterations},
            {"player_team", player_team},
            {"opponent_team", opponent_team}};

        std::filesystem::path debug_path = config.get_debug_path();
        FileStorage::ensure_directory_exists(debug_path.string());
        std::string timeout_filename =
            (debug_path / (timestamp + "_" + battle_id + ".json")).string();
        FileStorage::save_json_to_file(timeout_filename, timeout_state);

        simulator.cleanup_temp_files();
        set_error_response(res, 408, "Battle simulation timeout");
        return;
      }

      simulator.update(fixed_dt);
      iterations++;
    }

    if (iterations >= max_iterations) {
      simulator.cleanup_temp_files();
      res.status = 408;
      res.set_content(make_error_json("Battle simulation timeout").dump(),
                      "application/json");
      return;
    }

    bool debug_mode = config.debug;
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

    std::filesystem::path results_path = config.get_results_path();
    FileStorage::ensure_directory_exists(results_path.string());

    if (!FileStorage::check_disk_space(results_path.string(), 1048576)) {
      simulator.cleanup_temp_files();
      set_error_response(res, 507, "Insufficient storage space");
      return;
    }

    std::string result_filename =
        (results_path / (timestamp + "_" + battle_id + ".json")).string();
    if (!FileStorage::save_json_to_file(result_filename, result_to_save)) {
      simulator.cleanup_temp_files();
      set_error_response(res, 507, "Failed to save battle result");
      return;
    }

    FileStorage::cleanup_old_files(results_path.string(), 10, ".json");

    simulator.cleanup_temp_files();

    std::filesystem::path temp_path = config.get_temp_files_path();
    FileStorage::cleanup_old_files(temp_path.string(),
                                   config.temp_file_retention_count, ".json");

    auto request_end = std::chrono::steady_clock::now();
    auto request_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(request_end -
                                                              request_start);
    log_info("[{}] Battle request completed in {}ms", request_id,
             request_duration.count());

    if (request_duration.count() > 1000) {
      log_warn("[{}] Slow request detected: {}ms", request_id,
               request_duration.count());
    }

    res.status = 200;
    res.set_content(response.dump(), "application/json");

  } catch (const nlohmann::json::exception &e) {
    if (simulator_initialized) {
      simulator.cleanup_temp_files();
    }
    std::string error_msg =
        get_error_message("Invalid JSON: " + std::string(e.what()));
    nlohmann::json error = {{"error", error_msg}};
    if (config.error_detail_level == "trace" ||
        config.error_detail_level == "info") {
      error["details"] = e.what();
    }
    res.status = 400;
    res.set_content(error.dump(), "application/json");
    log_error("[{}] Battle API JSON error: {}", request_id, e.what());
  } catch (const std::exception &e) {
    if (simulator_initialized) {
      simulator.cleanup_temp_files();
    }
    std::string error_msg =
        get_error_message("Server error: " + std::string(e.what()));
    nlohmann::json error = {{"error", error_msg}};
    if (config.error_detail_level == "trace" ||
        config.error_detail_level == "info") {
      error["details"] = e.what();
    }
    res.status = 500;
    res.set_content(error.dump(), "application/json");
    log_error("[{}] Battle API error: {}", request_id, e.what());
  }
}

constexpr const char *SERVER_VERSION = "0.1.0";

std::string
BattleAPI::compute_game_state_checksum(const nlohmann::json &state) const {
  return ::compute_game_state_checksum(state);
}

void BattleAPI::handle_save_game_state(const httplib::Request &req,
                                       httplib::Response &res) {
  try {
    std::string content_type = req.get_header_value("Content-Type");
    return_if(content_type.find("application/json") == std::string::npos, 415,
              "Content-Type must be application/json");

    return_if(req.body.empty(), 400, "Request body is empty");

    nlohmann::json request_json = nlohmann::json::parse(req.body);

    return_if(!request_json.contains("userId") ||
                  !request_json.contains("gameState") ||
                  !request_json.contains("checksum"),
              400, "Missing required fields: userId, gameState, checksum");

    std::string userId = request_json["userId"].get<std::string>();
    nlohmann::json gameState = request_json["gameState"];
    std::string client_checksum = request_json["checksum"].get<std::string>();
    uint64_t timestamp = request_json.value(
        "timestamp",
        static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count()));

    std::string server_checksum = compute_game_state_checksum(gameState);

    auto entry_opt = afterhours::EntityQuery({.force_merge = true})
                         .whereHasComponent<server::async::TeamPoolEntry>()
                         .whereLambda([&userId](const afterhours::Entity &e) {
                           const server::async::TeamPoolEntry &entry =
                               e.get<server::async::TeamPoolEntry>();
                           return entry.userId == userId;
                         })
                         .gen_first();

    bool match = (client_checksum == server_checksum);

    if (entry_opt) {
      auto &entry_entity = entry_opt.asE();
      auto &entry = entry_entity.get<server::async::TeamPoolEntry>();

      if (!entry.gameStateChecksum.empty()) {
        match = (client_checksum == entry.gameStateChecksum);
      }

      entry.gameState = gameState;
      entry.gameStateChecksum = server_checksum;
      entry.lastSaved = timestamp;
    } else {
      auto &new_entity = afterhours::EntityHelper::createEntity();
      auto &entry = new_entity.addComponent<server::async::TeamPoolEntry>();
      entry.userId = userId;
      entry.gameState = gameState;
      entry.gameStateChecksum = server_checksum;
      entry.lastSaved = timestamp;
    }

    nlohmann::json response;
    response["status"] = "ok";
    response["match"] = match;
    response["serverVersion"] = SERVER_VERSION;

    if (!match) {
      response["gameState"] = gameState;
    }

    res.set_content(response.dump(), "application/json");
    res.status = 200;
  } catch (const nlohmann::json::exception &e) {
    set_error_response(res, 400, "Invalid JSON: " + std::string(e.what()));
  } catch (const std::exception &e) {
    set_error_response(res, 500, "Server error: " + std::string(e.what()));
  }
}

void BattleAPI::handle_get_game_state(const httplib::Request &req,
                                      httplib::Response &res) {
  try {
    std::string userId = req.get_param_value("userId");
    std::string checksum = req.get_param_value("checksum");

    return_if(userId.empty(), 400, "Missing userId parameter");
    return_if(checksum.empty(), 400, "Missing checksum parameter");

    auto entry_opt = afterhours::EntityQuery({.force_merge = true})
                         .whereHasComponent<server::async::TeamPoolEntry>()
                         .whereLambda([&userId](const afterhours::Entity &e) {
                           const server::async::TeamPoolEntry &entry =
                               e.get<server::async::TeamPoolEntry>();
                           return entry.userId == userId;
                         })
                         .gen_first();

    return_if(!entry_opt, 404, "Game state not found");

    const auto &entry = entry_opt.asE().get<server::async::TeamPoolEntry>();

    bool match = (checksum == entry.gameStateChecksum);

    nlohmann::json response;
    response["status"] = "ok";
    response["match"] = match;
    response["serverVersion"] = SERVER_VERSION;

    if (!match) {
      response["gameState"] = entry.gameState;
    }

    res.set_content(response.dump(), "application/json");
    res.status = 200;
  } catch (const std::exception &e) {
    set_error_response(res, 500, "Server error: " + std::string(e.what()));
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
