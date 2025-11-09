#pragma once

#include "../components/battle_load_request.h"
#include "../components/battle_team_data.h"
#include "../components/combat_queue.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../shop.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <fstream>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

struct BattleTeamFileLoaderSystem : afterhours::System<BattleLoadRequest> {
  bool loaded = false;
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();

    if (last_screen == GameStateManager::Screen::Battle &&
        gsm.active_screen != GameStateManager::Screen::Battle) {
      loaded = false;
    }

    last_screen = gsm.active_screen;
    return gsm.active_screen == GameStateManager::Screen::Battle && !loaded;
  }

  void for_each_with(afterhours::Entity &, BattleLoadRequest &request,
                     float) override {
    if (request.loaded) {
      log_info("BATTLE_LOADER: BattleLoadRequest already loaded, skipping");
      loaded = true;
      return;
    }
    
    log_info("BATTLE_LOADER: Loading battle teams - playerPath={}, opponentPath={}", 
             request.playerJsonPath, request.opponentJsonPath);

    auto manager_entity =
        afterhours::EntityHelper::get_singleton<CombatQueue>();
    if (!manager_entity.get().has<CombatQueue>()) {
      return;
    }

    if (!request.playerJsonPath.empty()) {
      load_team_from_json(request.playerJsonPath, true, manager_entity.get());
    }

    if (!request.opponentJsonPath.empty()) {
      load_team_from_json(request.opponentJsonPath, false,
                          manager_entity.get());
    }

    request.loaded = true;
    loaded = true;
    log_info("BATTLE_LOADER: Battle teams loaded successfully, loaded=true");
  }

private:
  void load_team_from_json(const std::string &jsonPath, bool isPlayer,
                           afterhours::Entity &manager_entity) {
    if (!std::filesystem::exists(jsonPath)) {
      if (isPlayer) {
        log_error("Player JSON file missing, cannot proceed: {}", jsonPath);
        return;
      } else {
        log_warn("Opponent JSON file missing, creating fallback team: {}",
                 jsonPath);
        create_fallback_opponent_team(manager_entity);
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
    } catch (const std::exception &e) {
      log_error("Failed to parse JSON file {}: {}", jsonPath, e.what());
      return;
    }

    if (!json.contains("team") || !json["team"].is_array()) {
      log_error("Invalid JSON format: missing or invalid 'team' array");
      return;
    }

    std::vector<TeamDishSpec> team_specs;

    const auto &team = json["team"];
    for (size_t i = 0; i < team.size(); ++i) {
      const auto &dishEntry = team[i];

      if (!dishEntry.contains("slot") || !dishEntry.contains("dishType")) {
        log_warn("Skipping invalid dish entry at index {}", i);
        continue;
      }

      int slot = dishEntry["slot"];
      std::string dishTypeStr = dishEntry["dishType"];

      auto dishTypeOpt = magic_enum::enum_cast<DishType>(dishTypeStr);
      if (!dishTypeOpt.has_value()) {
        log_warn("Unknown dish type: {}", dishTypeStr);
        continue;
      }

      TeamDishSpec spec;
      spec.dishType = dishTypeOpt.value();
      spec.slot = slot;

      if (dishEntry.contains("level") && dishEntry["level"].is_number()) {
        spec.level = dishEntry["level"].get<int>();
      } else {
        spec.level = 1;
      }

      if (dishEntry.contains("powerups") && dishEntry["powerups"].is_array()) {
        for (const auto &powerup : dishEntry["powerups"]) {
          if (powerup.is_number()) {
            spec.powerups.push_back(powerup.get<int>());
          }
        }
      }

      team_specs.push_back(spec);
    }

    if (isPlayer) {
      if (!manager_entity.has<BattleTeamDataPlayer>()) {
        manager_entity.addComponent<BattleTeamDataPlayer>();
      }
      auto &data = manager_entity.get<BattleTeamDataPlayer>();
      data.team = team_specs;
      data.instantiated = false;
    } else {
      if (!manager_entity.has<BattleTeamDataOpponent>()) {
        manager_entity.addComponent<BattleTeamDataOpponent>();
      }
      auto &data = manager_entity.get<BattleTeamDataOpponent>();
      data.team = team_specs;
      data.instantiated = false;
    }
  }

  void create_fallback_opponent_team(afterhours::Entity &manager_entity) {
    log_info("Creating fallback opponent team");
    std::vector<TeamDishSpec> fallback_team;
    std::vector<DishType> fallbackDishes = {DishType::Potato, DishType::Potato,
                                            DishType::Potato};

    for (size_t i = 0; i < fallbackDishes.size(); ++i) {
      TeamDishSpec spec;
      spec.dishType = fallbackDishes[i];
      spec.slot = (int)i;
      fallback_team.push_back(spec);
    }

    if (!manager_entity.has<BattleTeamDataOpponent>()) {
      manager_entity.addComponent<BattleTeamDataOpponent>();
    }
    auto &data = manager_entity.get<BattleTeamDataOpponent>();
    data.team = fallback_team;
    data.instantiated = false;
  }
};
