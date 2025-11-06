#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace server {
struct FileStorage {
  static std::vector<std::string>
  list_files_in_directory(const std::string &directory_path,
                          const std::string &extension = ".json");
  static nlohmann::json load_json_from_file(const std::string &file_path);
  static nlohmann::json
  load_json_from_file_with_retry(const std::string &file_path,
                                 int max_retries = 3);
  static bool save_json_to_file(const std::string &file_path,
                                const nlohmann::json &data);
  static bool save_json_to_file_with_retry(const std::string &file_path,
                                           const nlohmann::json &data,
                                           int max_retries = 3);
  static bool file_exists(const std::string &file_path);
  static bool directory_exists(const std::string &directory_path);
  static void ensure_directory_exists(const std::string &directory_path);
  static size_t
  count_files_in_directory(const std::string &directory_path,
                           const std::string &extension = ".json");
  static void cleanup_old_files(const std::string &directory, size_t keep_count,
                                const std::string &extension = ".json");
  static bool check_disk_space(const std::string &path,
                               size_t required_bytes = 1048576);
};
} // namespace server
