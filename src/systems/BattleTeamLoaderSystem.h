#pragma once

#include "../components/battle_load_request.h"
#include "../components/battle_team_tags.h"
#include "../components/dish_battle_state.h"
#include "../components/has_tooltip.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../components/render_order.h"
#include "../components/transform.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../tooltip.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <fstream>
#include <magic_enum/magic_enum.hpp> // For enum string conversion
#include <nlohmann/json.hpp>
#include <random>

struct BattleTeamLoaderSystem : afterhours::System<> {
  bool loaded = false;
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();

    // Reset loaded flag when leaving battle screen
    if (last_screen == GameStateManager::Screen::Battle &&
        gsm.active_screen != GameStateManager::Screen::Battle) {
      loaded = false;
    }

    last_screen = gsm.active_screen;
    return gsm.active_screen == GameStateManager::Screen::Battle && !loaded;
  }

  void once(float) override {
    auto battleRequest =
        afterhours::EntityHelper::get_singleton<BattleLoadRequest>();

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

    // Merge entities so they can be found by rendering systems
    afterhours::EntityHelper::merge_entity_arrays();
  }

private:
  void load_team_from_json(const std::string &jsonPath, bool isPlayer) {
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
    } catch (const std::exception &e) {
      log_error("Failed to parse JSON file {}: {}", jsonPath, e.what());
      return;
    }

    if (!json.contains("team") || !json["team"].is_array()) {
      log_error("Invalid JSON format: missing or invalid 'team' array");
      return;
    }

    const auto &team = json["team"];
    for (size_t i = 0; i < team.size(); ++i) {
      const auto &dishEntry = team[i];

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
        DishType::GarlicBread, DishType::TomatoSoup, DishType::GrilledCheese};

    for (size_t i = 0; i < fallbackDishes.size(); ++i) {
      create_battle_dish_entity(fallbackDishes[i], (int)i, false);
    }
  }

  void create_battle_dish_entity(DishType dishType, int slot, bool isPlayer) {
    auto &entity = afterhours::EntityHelper::createEntity();

    // Calculate position based on team and slot
    float x, y;
    if (isPlayer) {
      x = 120.0f + slot * 100.0f; // Left side, spaced 100px apart
      y = 150.0f;
    } else {
      x = 120.0f + slot * 100.0f; // Left side, spaced 100px apart
      y = 500.0f;
    }

    // Add components
    entity.addComponent<Transform>(afterhours::vec2{x, y},
                                   afterhours::vec2{80.0f, 80.0f});
    entity.addComponent<IsDish>(dishType);
    auto &dbs = entity.addComponent<DishBattleState>();
    dbs.queue_index = slot;
    dbs.team_side = isPlayer ? DishBattleState::TeamSide::Player
                             : DishBattleState::TeamSide::Opponent;
    dbs.phase = DishBattleState::Phase::InQueue;
    dbs.phase_progress = 0.0f;
    entity.addComponent<HasRenderOrder>(RenderOrder::BattleTeams);

    if (isPlayer) {
      entity.addComponent<IsPlayerTeamItem>();
    } else {
      entity.addComponent<IsOpponentTeamItem>();
    }

    entity.addComponent<HasTooltip>(generate_dish_tooltip(dishType));
  }

  void add_random_dish_to_inventory() {
    // Array of all available dish types
    constexpr DishType dish_pool[] = {
        DishType::GarlicBread,   DishType::TomatoSoup,
        DishType::GrilledCheese, DishType::ChickenSkewer,
        DishType::CucumberSalad, DishType::VanillaSoftServe,
        DishType::CapreseSalad,  DishType::Minestrone,
        DishType::SearedSalmon,  DishType::SteakFlorentine,
    };

    // Pick a random dish
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(
        0, sizeof(dish_pool) / sizeof(dish_pool[0]) - 1);
    DishType randomDish = dish_pool[dis(gen)];

    // Create the inventory item entity
    auto &entity = afterhours::EntityHelper::createEntity();

    // Position it in the first inventory slot (slot 100)
    float x = 100.0f; // First inventory slot X position
    float y = 500.0f; // Inventory row Y position

    entity.addComponent<Transform>(afterhours::vec2{x, y},
                                   afterhours::vec2{80.0f, 80.0f});
    entity.addComponent<IsDish>(randomDish);
    entity.addComponent<IsInventoryItem>();
    entity.get<IsInventoryItem>().slot = 100; // First inventory slot

    log_info("Added random dish to inventory: {} at slot 100",
             magic_enum::enum_name(randomDish));
  }
};
