#include "server_config.h"
#include "../log.h"
#include "file_storage.h"
#include <nlohmann/json.hpp>

namespace server {
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

  if (json_config.contains("port") && json_config["port"].is_number()) {
    config.port = json_config["port"];
  }

  if (json_config.contains("base_path") &&
      json_config["base_path"].is_string()) {
    config.base_path = json_config["base_path"];
  }

  if (json_config.contains("timeout_seconds") &&
      json_config["timeout_seconds"].is_number()) {
    config.timeout_seconds = json_config["timeout_seconds"];
  }

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

  if (json_config.contains("max_request_body_size") &&
      json_config["max_request_body_size"].is_number()) {
    config.max_request_body_size = json_config["max_request_body_size"];
  }

  if (json_config.contains("max_team_size") &&
      json_config["max_team_size"].is_number()) {
    config.max_team_size = json_config["max_team_size"];
  }

  if (json_config.contains("max_simulation_iterations") &&
      json_config["max_simulation_iterations"].is_number()) {
    config.max_simulation_iterations = json_config["max_simulation_iterations"];
  }

  if (json_config.contains("temp_file_retention_count") &&
      json_config["temp_file_retention_count"].is_number()) {
    config.temp_file_retention_count = json_config["temp_file_retention_count"];
  }

  if (json_config.contains("enable_cors") &&
      json_config["enable_cors"].is_boolean()) {
    config.enable_cors = json_config["enable_cors"];
  }

  if (json_config.contains("cors_origin") &&
      json_config["cors_origin"].is_string()) {
    config.cors_origin = json_config["cors_origin"];
  }

  if (json_config.contains("file_operation_retries") &&
      json_config["file_operation_retries"].is_number()) {
    config.file_operation_retries = json_config["file_operation_retries"];
  }

  return config;
}

ServerConfig ServerConfig::defaults() {
  ServerConfig config;
  return config;
}

std::filesystem::path ServerConfig::get_temp_files_path() const {
  return std::filesystem::path(base_path) / "output" / "battles";
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
