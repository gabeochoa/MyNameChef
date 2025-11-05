#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <filesystem>

namespace server {
  struct FileStorage {
    static std::vector<std::string> list_files_in_directory(const std::string &directory_path, const std::string &extension = ".json");
    static nlohmann::json load_json_from_file(const std::string &file_path);
    static bool save_json_to_file(const std::string &file_path, const nlohmann::json &data);
    static bool file_exists(const std::string &file_path);
    static bool directory_exists(const std::string &directory_path);
    static void ensure_directory_exists(const std::string &directory_path);
    static size_t count_files_in_directory(const std::string &directory_path, const std::string &extension = ".json");
  };
}

