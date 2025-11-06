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

  static ServerConfig load_from_json(const std::string &config_path);
  static ServerConfig defaults();

  std::filesystem::path get_temp_files_path() const;
  std::filesystem::path get_results_path() const;
  std::filesystem::path get_opponents_path() const;
  std::filesystem::path get_debug_path() const;
};
} // namespace server
