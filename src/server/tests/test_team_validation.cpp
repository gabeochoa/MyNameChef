#include "../test_framework.h"
#include "../team_manager.h"
#include "../file_storage.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <sstream>

static nlohmann::json load_test_json(const std::string &filename) {
  std::string path = "src/server/tests/test_data/example_teams/" + filename;
  auto result = server::FileStorage::load_json_from_file(path);
  if (!server::FileStorage::file_exists(path)) {
    throw std::runtime_error("Test file does not exist: " + path);
  }
  return result;
}

static void discover_and_test_team_files() {
  std::string test_dir = "src/server/tests/test_data/example_teams";
  
  if (!std::filesystem::exists(test_dir)) {
    throw std::runtime_error("Test directory does not exist: " + test_dir);
  }
  
  std::vector<std::string> test_files;
  for (const auto &entry : std::filesystem::directory_iterator(test_dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      test_files.push_back(entry.path().filename().string());
    }
  }
  
  if (test_files.empty()) {
    throw std::runtime_error("No test files found in " + test_dir);
  }
  
  for (const auto &filename : test_files) {
    bool expected_valid = filename.find("valid_") == 0;
    bool expected_invalid = filename.find("invalid_") == 0;
    
    // Skip files that don't follow the naming convention (e.g., battle_team_*.json)
    if (!expected_valid && !expected_invalid) {
      continue;
    }
    
    std::string filepath = test_dir + "/" + filename;
    nlohmann::json json = server::FileStorage::load_json_from_file(filepath);
    
    bool actual_result = server::TeamManager::validate_team_json(json);
    
    if (expected_valid && !actual_result) {
      std::stringstream ss;
      ss << "File " << filename << " should pass validation but failed";
      throw std::runtime_error(ss.str());
    }
    
    if (expected_invalid && actual_result) {
      std::stringstream ss;
      ss << "File " << filename << " should fail validation but passed";
      throw std::runtime_error(ss.str());
    }
  }
}

SERVER_TEST(team_validation_invalid_empty_json) {
  nlohmann::json json = load_test_json("invalid_empty.json");
  ASSERT_FALSE(server::TeamManager::validate_team_json(json));
}

SERVER_TEST(team_validation_invalid_missing_team) {
  nlohmann::json json = load_test_json("invalid_missing_team.json");
  ASSERT_FALSE(server::TeamManager::validate_team_json(json));
}

SERVER_TEST(team_validation_invalid_team_not_array) {
  nlohmann::json json = load_test_json("invalid_team_not_array.json");
  ASSERT_FALSE(server::TeamManager::validate_team_json(json));
}

SERVER_TEST(team_validation_invalid_empty_team) {
  nlohmann::json json = load_test_json("invalid_empty_team.json");
  ASSERT_FALSE(server::TeamManager::validate_team_json(json));
}

SERVER_TEST(team_validation_invalid_too_many_dishes) {
  nlohmann::json json = load_test_json("invalid_too_many_dishes_8_items.json");
  if (json.empty()) {
    throw std::runtime_error("Failed to load test JSON file");
  }
  ASSERT_FALSE(server::TeamManager::validate_team_json(json));
}

SERVER_TEST(team_validation_invalid_missing_dish_type) {
  nlohmann::json json = load_test_json("invalid_missing_dish_type.json");
  ASSERT_FALSE(server::TeamManager::validate_team_json(json));
}

SERVER_TEST(team_validation_invalid_missing_slot) {
  nlohmann::json json = load_test_json("invalid_missing_slot.json");
  ASSERT_FALSE(server::TeamManager::validate_team_json(json));
}

SERVER_TEST(team_validation_invalid_slot_negative) {
  nlohmann::json json = load_test_json("invalid_slot_negative.json");
  ASSERT_FALSE(server::TeamManager::validate_team_json(json));
}

SERVER_TEST(team_validation_invalid_slot_too_large) {
  nlohmann::json json = load_test_json("invalid_slot_too_large.json");
  ASSERT_FALSE(server::TeamManager::validate_team_json(json));
}

SERVER_TEST(team_validation_invalid_level_zero) {
  nlohmann::json json = load_test_json("invalid_level_zero.json");
  ASSERT_FALSE(server::TeamManager::validate_team_json(json));
}

SERVER_TEST(team_validation_invalid_level_negative) {
  nlohmann::json json = load_test_json("invalid_level_negative.json");
  ASSERT_FALSE(server::TeamManager::validate_team_json(json));
}

SERVER_TEST(team_validation_invalid_powerups_not_array) {
  nlohmann::json json = load_test_json("invalid_powerups_not_array.json");
  ASSERT_FALSE(server::TeamManager::validate_team_json(json));
}

SERVER_TEST(team_validation_valid_single_dish) {
  nlohmann::json json = load_test_json("valid_single_dish.json");
  ASSERT_TRUE(server::TeamManager::validate_team_json(json));
}

SERVER_TEST(team_validation_valid_with_level) {
  nlohmann::json json = load_test_json("valid_with_level.json");
  ASSERT_TRUE(server::TeamManager::validate_team_json(json));
}

SERVER_TEST(team_validation_valid_with_powerups) {
  nlohmann::json json = load_test_json("valid_with_powerups.json");
  ASSERT_TRUE(server::TeamManager::validate_team_json(json));
}

SERVER_TEST(team_validation_valid_full_team) {
  nlohmann::json json = load_test_json("valid_full_team_7_dishes.json");
  ASSERT_TRUE(server::TeamManager::validate_team_json(json));
}

// Auto-discovery test: automatically validates all files in example_teams/
// based on their filename prefix (valid_* should pass, invalid_* should fail)
SERVER_TEST(team_validation_auto_discovery) {
  discover_and_test_team_files();
}
