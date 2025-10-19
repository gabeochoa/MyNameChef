#pragma once

#include "../components/battle_load_request.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include <afterhours/ah.h>
#include <chrono>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>    // For std::ofstream
#include <functional> // For std::reference_wrapper
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <random>

/**
 * ExportMenuSnapshotSystem - Exports player inventory to battle format
 *
 * This system:
 * - Queries all inventory dishes
 * - Exports them to JSON format for battle loading
 * - Creates or updates BattleLoadRequest singleton for battle system
 *
 * Handles singleton reuse to prevent crashes on multiple battles.
 */
class ExportMenuSnapshotSystem {
public:
  std::string export_menu_snapshot() {
    // Query entities with both IsInventoryItem and IsDish
    std::vector<std::reference_wrapper<afterhours::Entity>> inventory_dishes;
    for (auto &ref : afterhours::EntityQuery()
                         .template whereHasComponent<IsInventoryItem>()
                         .template whereHasComponent<IsDish>()
                         .gen()) {
      inventory_dishes.push_back(ref);
    }

    if (inventory_dishes.empty()) {
      return "";
    }

    // Sort by slot and normalize slots (100-106 -> 0-6)
    std::sort(inventory_dishes.begin(), inventory_dishes.end(),
              [](const std::reference_wrapper<afterhours::Entity> &a,
                 const std::reference_wrapper<afterhours::Entity> &b) {
                return a.get().get<IsInventoryItem>().slot <
                       b.get().get<IsInventoryItem>().slot;
              });

    // Build team array
    nlohmann::json team = nlohmann::json::array();
    for (const auto &entity_ref : inventory_dishes) {
      auto &entity = entity_ref.get();
      auto &inventory_item = entity.get<IsInventoryItem>();
      auto &dish = entity.get<IsDish>();

      nlohmann::json dish_entry;
      dish_entry["slot"] = inventory_item.slot - 100; // Normalize to 0-6
      dish_entry["dishType"] = magic_enum::enum_name(dish.type);
      team.push_back(dish_entry);
    }

    // Generate seed
    std::random_device rd;
    std::mt19937_64 gen(rd());
    uint64_t seed = gen();

    // Build complete JSON
    nlohmann::json snapshot;
    snapshot["team"] = team;
    snapshot["seed"] = seed;

    // Add metadata
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    char timestamp[32];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S",
                  std::localtime(&time_t));

    snapshot["meta"]["timestampIso"] = timestamp;
    snapshot["meta"]["gameVersion"] = "0.1.0";

    // Ensure output directory exists
    std::filesystem::create_directories("output/battles/pending");

    // Write file
    std::string filename =
        fmt::format("output/battles/pending/{}_{:016x}.json", timestamp, seed);
    std::ofstream file(filename);
    if (file.is_open()) {
      file << snapshot.dump(2);
      file.close();

      // Create BattleLoadRequest singleton for battle loading
      BattleLoadRequest request;
      request.playerJsonPath = filename;
      request.opponentJsonPath = "resources/battles/opponent_sample.json";

      // Check if singleton already exists and update it, or create new one
      const auto componentId =
          afterhours::components::get_type_id<BattleLoadRequest>();
      bool singletonExists =
          afterhours::EntityHelper::get().singletonMap.contains(componentId);

      if (singletonExists) {
        // Update existing singleton
        auto existingRequest =
            afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
        if (existingRequest.get().has<BattleLoadRequest>()) {
          auto &existingBattleRequest =
              existingRequest.get().get<BattleLoadRequest>();
          existingBattleRequest.playerJsonPath = filename;
          existingBattleRequest.opponentJsonPath =
              "resources/battles/opponent_sample.json";
        }
      } else {
        // Create new singleton
        auto &requestEntity = afterhours::EntityHelper::createEntity();
        requestEntity.addComponent<BattleLoadRequest>(std::move(request));
        afterhours::EntityHelper::registerSingleton<BattleLoadRequest>(
            requestEntity);
      }

      return filename;
    } else {
      log_error("Failed to write snapshot file: {}", filename.c_str());
      return "";
    }
  }
};
