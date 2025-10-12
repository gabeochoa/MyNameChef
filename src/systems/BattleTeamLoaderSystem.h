#pragma once

#include "../components/battle_load_request.h"
#include "../components/battle_team_tags.h"
#include "../components/is_dish.h"
#include "../components/transform.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../rl.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

struct BattleTeamLoaderSystem : afterhours::System<> {
  bool loaded = false;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle && !loaded;
  }

  void once(float) override {
    auto battleRequest = afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    if (!battleRequest.get().has<BattleLoadRequest>()) {
      log_error("No BattleLoadRequest found");
      return;
    }

    auto &request = battleRequest.get().get<BattleLoadRequest>();
    
    // Load player team
    if (!request.playerJsonPath.empty()) {
      load_team_from_json(request.playerJsonPath, true);
    }
    
    // Load opponent team
    if (!request.opponentJsonPath.empty()) {
      load_team_from_json(request.opponentJsonPath, false);
    }
    
    request.loaded = true;
    loaded = true;
    log_info("Battle teams loaded successfully");
  }

private:
  void load_team_from_json(const std::string& jsonPath, bool isPlayer) {
    if (!std::filesystem::exists(jsonPath)) {
      log_warn("JSON file does not exist: {}", jsonPath);
      if (isPlayer) {
        log_error("Player JSON file missing, cannot proceed");
        return;
      } else {
        // Create fallback opponent team
        create_fallback_opponent_team();
        return;
      }
    }

    std::ifstream file(jsonPath);
    if (!file.is_open()) {
      log_error("Failed to open JSON file: {}", jsonPath);
      return;
    }

    nlohmann::json json;
    try {
      file >> json;
    } catch (const std::exception& e) {
      log_error("Failed to parse JSON file {}: {}", jsonPath, e.what());
      return;
    }

    if (!json.contains("team") || !json["team"].is_array()) {
      log_error("Invalid JSON format: missing or invalid 'team' array");
      return;
    }

    const auto& team = json["team"];
    for (size_t i = 0; i < team.size(); ++i) {
      const auto& dishEntry = team[i];
      
      if (!dishEntry.contains("slot") || !dishEntry.contains("dishType")) {
        log_warn("Skipping invalid dish entry at index {}", i);
        continue;
      }

      int slot = dishEntry["slot"];
      std::string dishTypeStr = dishEntry["dishType"];
      
      // Convert string to DishType
      auto dishTypeOpt = magic_enum::enum_cast<DishType>(dishTypeStr);
      if (!dishTypeOpt.has_value()) {
        log_warn("Unknown dish type: {}", dishTypeStr);
        continue;
      }

      create_battle_dish_entity(dishTypeOpt.value(), slot, isPlayer);
    }
  }

  void create_fallback_opponent_team() {
    log_info("Creating fallback opponent team");
    std::vector<DishType> fallbackDishes = {
      DishType::GarlicBread,
      DishType::TomatoSoup,
      DishType::GrilledCheese
    };

    for (size_t i = 0; i < fallbackDishes.size(); ++i) {
      create_battle_dish_entity(fallbackDishes[i], (int)i, false);
    }
  }

  void create_battle_dish_entity(DishType dishType, int slot, bool isPlayer) {
    auto &entity = afterhours::EntityHelper::createEntity();
    
    // Calculate position based on team and slot
    float x, y;
    if (isPlayer) {
      x = 120.0f + slot * 100.0f;  // Left side, spaced 100px apart
      y = 420.0f;  // Bottom row
    } else {
      x = 960.0f + slot * 100.0f;  // Right side, spaced 100px apart  
      y = 220.0f;  // Top row
    }

    // Add components
    entity.addComponent<Transform>(afterhours::vec2{x, y}, afterhours::vec2{80.0f, 80.0f});
    entity.addComponent<IsDish>(dishType);
    
    if (isPlayer) {
      entity.addComponent<IsPlayerTeamItem>();
    } else {
      entity.addComponent<IsOpponentTeamItem>();
    }

    log_info("Created {} battle dish: {} at slot {}", 
             isPlayer ? "player" : "opponent", 
             magic_enum::enum_name(dishType), slot);
  }
};
