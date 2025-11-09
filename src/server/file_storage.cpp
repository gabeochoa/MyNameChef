#include "file_storage.h"
#include "../log.h"
#include <chrono>
#include <fstream>
#include <thread>

namespace server {
std::vector<std::string>
FileStorage::list_files_in_directory(const std::string &directory_path,
                                     const std::string &extension) {
  std::vector<std::string> files;

  if (!std::filesystem::exists(directory_path)) {
    log_warn("Directory does not exist: {}", directory_path);
    return files;
  }

  for (const auto &entry :
       std::filesystem::directory_iterator(directory_path)) {
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

nlohmann::json
FileStorage::load_json_from_file_with_retry(const std::string &file_path,
                                            int max_retries) {
  for (int attempt = 0; attempt < max_retries; ++attempt) {
    nlohmann::json result = load_json_from_file(file_path);
    if (!result.empty()) {
      return result;
    }

    if (attempt < max_retries - 1) {
      log_warn("Retrying file load for {} (attempt {}/{})", file_path,
               attempt + 1, max_retries);
      std::this_thread::sleep_for(
          std::chrono::milliseconds(100 * (attempt + 1)));
    }
  }

  log_error("Failed to load file {} after {} attempts", file_path, max_retries);
  return nlohmann::json{};
}

std::string FileStorage::load_string_from_file(const std::string &file_path) {
  if (!file_exists(file_path)) {
    return "";
  }

  std::ifstream file(file_path);
  if (!file.is_open()) {
    log_error("Failed to open file: {}", file_path);
    return "";
  }

  std::string content;
  std::getline(file, content);
  file.close();
  return content;
}

bool FileStorage::save_json_to_file(const std::string &file_path,
                                    const nlohmann::json &data) {
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

bool FileStorage::save_json_to_file_with_retry(const std::string &file_path,
                                               const nlohmann::json &data,
                                               int max_retries) {
  for (int attempt = 0; attempt < max_retries; ++attempt) {
    if (save_json_to_file(file_path, data)) {
      return true;
    }

    if (attempt < max_retries - 1) {
      log_warn("Retrying file save for {} (attempt {}/{})", file_path,
               attempt + 1, max_retries);
      std::this_thread::sleep_for(
          std::chrono::milliseconds(100 * (attempt + 1)));
    }
  }

  log_error("Failed to save file {} after {} attempts", file_path, max_retries);
  return false;
}

bool FileStorage::save_string_to_file(const std::string &file_path,
                                      const std::string &data) {
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
    file << data;
    file.close();
    return true;
  } catch (const std::exception &e) {
    log_error("Failed to write string to file {}: {}", file_path, e.what());
    return false;
  }
}

bool FileStorage::file_exists(const std::string &file_path) {
  return std::filesystem::exists(file_path) &&
         std::filesystem::is_regular_file(file_path);
}

bool FileStorage::directory_exists(const std::string &directory_path) {
  return std::filesystem::exists(directory_path) &&
         std::filesystem::is_directory(directory_path);
}

void FileStorage::ensure_directory_exists(const std::string &directory_path) {
  if (!directory_exists(directory_path)) {
    std::filesystem::create_directories(directory_path);
  }
}

size_t FileStorage::count_files_in_directory(const std::string &directory_path,
                                             const std::string &extension) {
  return list_files_in_directory(directory_path, extension).size();
}

void FileStorage::cleanup_old_files(const std::string &directory,
                                    size_t keep_count,
                                    const std::string &extension) {
  if (!directory_exists(directory)) {
    return;
  }

  std::vector<std::pair<std::filesystem::path, std::filesystem::file_time_type>>
      files_with_time;

  for (const auto &entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file() && entry.path().extension() == extension) {
      files_with_time.push_back({entry.path(), entry.last_write_time()});
    }
  }

  if (files_with_time.size() <= keep_count) {
    return;
  }

  std::sort(files_with_time.begin(), files_with_time.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });

  size_t files_to_delete = files_with_time.size() - keep_count;
  for (size_t i = keep_count; i < files_with_time.size(); ++i) {
    try {
      std::filesystem::remove(files_with_time[i].first);
      log_info("Cleaned up old file: {}", files_with_time[i].first.string());
    } catch (const std::exception &e) {
      log_error("Failed to delete file {}: {}",
                files_with_time[i].first.string(), e.what());
    }
  }

  log_info("File cleanup: kept {} files, deleted {} files", keep_count,
           files_to_delete);
}

std::string FileStorage::get_game_state_save_path(const std::string &userId) {
  return "output/saves/game_state_" + userId + ".json";
}

bool FileStorage::check_disk_space(const std::string &path,
                                   size_t required_bytes) {
  try {
    std::filesystem::path fs_path(path);
    if (!std::filesystem::exists(fs_path)) {
      fs_path = fs_path.parent_path();
    }
    if (!std::filesystem::exists(fs_path)) {
      log_warn("Cannot check disk space for non-existent path: {}", path);
      return true;
    }

    std::filesystem::space_info space = std::filesystem::space(fs_path);
    if (space.available < required_bytes) {
      log_error(
          "Insufficient disk space: {} bytes available, {} bytes required",
          space.available, required_bytes);
      return false;
    }
    return true;
  } catch (const std::exception &e) {
    log_error("Failed to check disk space for {}: {}", path, e.what());
    return true;
  }
}
} // namespace server
