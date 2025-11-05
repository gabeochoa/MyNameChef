#include "../test_framework.h"
#include "../team_manager.h"
#include "../file_storage.h"
#include <nlohmann/json.hpp>

inline nlohmann::json load_test_json(const std::string &filename) {
  std::string path = "src/server/tests/test_data/" + filename;
  return server::FileStorage::load_json_from_file(path);
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
  nlohmann::json json = load_test_json("invalid_too_many_dishes.json");
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
  nlohmann::json json = load_test_json("valid_full_team.json");
  ASSERT_TRUE(server::TeamManager::validate_team_json(json));
}
