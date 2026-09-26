#include "server_config.h"
#include "../log.h"
#include "file_storage.h"
#include <nlohmann/json.hpp>

namespace server {
// Issue 21: bounded integer config; invalid values keep defaults.
static bool bounded_int(const nlohmann::json &j, const char *k, int lo, int hi, int &out) {
  if (!j.contains(k) || (!j[k].is_number_integer() && !j[k].is_number_unsigned())) return false;
  long v = j[k].get<long>(); if (v < lo || v > hi) { log_warn("Config {}={} out of [{},{}], using default", k, v, lo, hi); return false; }
  out = static_cast<int>(v); return true;
}
ServerConfig ServerConfig::load_from_json(const std::string &config_path) {
  ServerConfig config = defaults();

  if (!FileStorage::file_exists(config_path)) {
    log_warn("Config file not found: {}, using defaults", config_path);
    return config;
  }

  nlohmann::json json_config = FileStorage::load_json_from_file(config_path);

  if (json_config.empty()) {
    log_warn("Config file is empty: {}, using defaults", config_path);
    return config;
  }

  bounded_int(json_config, "port", 1, 65535, config.port);

  if (json_config.contains("base_path") &&
      json_config["base_path"].is_string()) {
    config.base_path = json_config["base_path"];
  }

  bounded_int(json_config, "timeout_seconds", 1, 600, config.timeout_seconds);

  if (json_config.contains("error_detail_level") &&
      json_config["error_detail_level"].is_string()) {
    std::string level = json_config["error_detail_level"];
    if (level == "trace" || level == "info" || level == "warn" ||
        level == "error") {
      config.error_detail_level = level;
    } else {
      log_warn("Invalid error_detail_level: {}, must be one of: trace, info, "
               "warn, error. Using default 'warn'",
               level);
    }
  }

  if (json_config.contains("debug") && json_config["debug"].is_boolean()) {
    config.debug = json_config["debug"];
  }

  bounded_int(json_config, "max_request_body_size", 1024, 67108864, config.max_request_body_size);

  bounded_int(json_config, "max_team_size", 1, 7, config.max_team_size);

  bounded_int(json_config, "max_simulation_iterations", 1, 10000000, config.max_simulation_iterations);

  bounded_int(json_config, "temp_file_retention_count", 0, 10000, config.temp_file_retention_count);

  if (json_config.contains("enable_cors") &&
      json_config["enable_cors"].is_boolean()) {
    config.enable_cors = json_config["enable_cors"];
  }

  if (json_config.contains("cors_origin") &&
      json_config["cors_origin"].is_string()) {
    config.cors_origin = json_config["cors_origin"];
  }

  bounded_int(json_config, "file_operation_retries", 0, 20, config.file_operation_retries);

  return config;
}

ServerConfig ServerConfig::defaults() {
  ServerConfig config;
  return config;
}

std::filesystem::path ServerConfig::get_temp_files_path() const {
  // Server-private scratch dir: the simulator writes temp_player_<seed> /
  // temp_opponent_<seed> files here and deletes them after re-simulation.
  // Sharing output/battles with a same-directory client would delete the
  // client's own battle files (same names, same seed).
  return std::filesystem::path(base_path) / "output" / "battles" /
         "server_temp";
}

std::filesystem::path ServerConfig::get_results_path() const {
  return std::filesystem::path(base_path) / "output" / "battles" / "results";
}

std::filesystem::path ServerConfig::get_opponents_path() const {
  return std::filesystem::path(base_path) / "resources" / "battles" /
         "opponents";
}

std::filesystem::path ServerConfig::get_debug_path() const {
  return std::filesystem::path(base_path) / "output" / "battles" / "debug";
}

} // namespace server
