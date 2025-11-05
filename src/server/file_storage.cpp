#include "file_storage.h"
#include "../log.h"
#include <fstream>

namespace server {
  std::vector<std::string> FileStorage::list_files_in_directory(const std::string &directory_path, const std::string &extension) {
    std::vector<std::string> files;
    
    if (!std::filesystem::exists(directory_path)) {
      log_warn("Directory does not exist: {}", directory_path);
      return files;
    }
    
    for (const auto &entry : std::filesystem::directory_iterator(directory_path)) {
      if (entry.is_regular_file() && entry.path().extension() == extension) {
        files.push_back(entry.path().string());
      }
    }
    
    return files;
  }
  
  nlohmann::json FileStorage::load_json_from_file(const std::string &file_path) {
    if (!file_exists(file_path)) {
      log_error("File does not exist: {}", file_path);
      return nlohmann::json{};
    }
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
      log_error("Failed to open file: {}", file_path);
      return nlohmann::json{};
    }
    
    nlohmann::json json_data;
    try {
      file >> json_data;
    } catch (const std::exception &e) {
      log_error("Failed to parse JSON from file {}: {}", file_path, e.what());
      return nlohmann::json{};
    }
    
    return json_data;
  }
  
  bool FileStorage::save_json_to_file(const std::string &file_path, const nlohmann::json &data) {
    std::filesystem::path path(file_path);
    if (path.has_parent_path()) {
      ensure_directory_exists(path.parent_path().string());
    }
    
    std::ofstream file(file_path);
    if (!file.is_open()) {
      log_error("Failed to open file for writing: {}", file_path);
      return false;
    }
    
    try {
      file << data.dump(2);
      file.close();
      return true;
    } catch (const std::exception &e) {
      log_error("Failed to write JSON to file {}: {}", file_path, e.what());
      return false;
    }
  }
  
  bool FileStorage::file_exists(const std::string &file_path) {
    return std::filesystem::exists(file_path) && std::filesystem::is_regular_file(file_path);
  }
  
  bool FileStorage::directory_exists(const std::string &directory_path) {
    return std::filesystem::exists(directory_path) && std::filesystem::is_directory(directory_path);
  }
  
  void FileStorage::ensure_directory_exists(const std::string &directory_path) {
    if (!directory_exists(directory_path)) {
      std::filesystem::create_directories(directory_path);
    }
  }
  
  size_t FileStorage::count_files_in_directory(const std::string &directory_path, const std::string &extension) {
    return list_files_in_directory(directory_path, extension).size();
  }

  void FileStorage::cleanup_old_files(const std::string &directory, size_t keep_count, const std::string &extension) {
    if (!directory_exists(directory)) {
      return;
    }

    std::vector<std::pair<std::filesystem::path, std::filesystem::file_time_type>> files_with_time;

    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
      if (entry.is_regular_file() && entry.path().extension() == extension) {
        files_with_time.push_back({entry.path(), entry.last_write_time()});
      }
    }

    if (files_with_time.size() <= keep_count) {
      return;
    }

    std::sort(files_with_time.begin(), files_with_time.end(),
              [](const auto &a, const auto &b) {
                return a.second > b.second;
              });

    size_t files_to_delete = files_with_time.size() - keep_count;
    for (size_t i = keep_count; i < files_with_time.size(); ++i) {
      try {
        std::filesystem::remove(files_with_time[i].first);
        log_info("Cleaned up old file: {}", files_with_time[i].first.string());
      } catch (const std::exception &e) {
        log_error("Failed to delete file {}: {}", files_with_time[i].first.string(), e.what());
      }
    }

    log_info("File cleanup: kept {} files, deleted {} files", keep_count, files_to_delete);
  }
}

