#pragma once

#include <string>
#include <filesystem>

namespace server {
  using TeamFilePath = std::string;
  using TeamId = std::string;
  
  inline TeamId extract_team_id_from_path(const TeamFilePath &file_path) {
    std::filesystem::path path(file_path);
    return path.stem().string();
  }
}

