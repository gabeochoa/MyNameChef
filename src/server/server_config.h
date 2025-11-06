#pragma once

#include <filesystem>
#include <string>

namespace server {
struct ServerConfig {
  int port = 8080;
  std::string base_path = ".";
  int timeout_seconds = 30;
  std::string error_detail_level = "warn";
  bool debug = false;
  int max_request_body_size = 1048576;
  int max_team_size = 7;
  int max_simulation_iterations = 1000000;
  int temp_file_retention_count = 10;
  bool enable_cors = true;
  std::string cors_origin = "*";
  int file_operation_retries = 3;

  static ServerConfig load_from_json(const std::string &config_path);
  static ServerConfig defaults();

  std::filesystem::path get_temp_files_path() const;
  std::filesystem::path get_results_path() const;
  std::filesystem::path get_opponents_path() const;
  std::filesystem::path get_debug_path() const;
};
} // namespace server
