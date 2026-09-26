#pragma once

#include "../components/battle_load_request.h"
#include "../components/dish_level.h"
#include "../components/drink_pairing.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../utils/http_helpers.h"
#include <afterhours/ah.h>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>    // For std::ofstream
#include <functional> // For std::reference_wrapper
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

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
    for (auto &ref : afterhours::EntityQuery({.force_merge = true})
                         .template whereHasComponent<IsInventoryItem>()
                         .template whereHasComponent<IsDish>()
                         .gen()) {
      inventory_dishes.push_back(ref);
    }

    if (inventory_dishes.empty()) {
      return "";
    }

    std::sort(inventory_dishes.begin(), inventory_dishes.end(),
              [](const std::reference_wrapper<afterhours::Entity> &a,
                 const std::reference_wrapper<afterhours::Entity> &b) {
                return a.get().get<IsInventoryItem>().slot <
                       b.get().get<IsInventoryItem>().slot;
              });

    nlohmann::json team = nlohmann::json::array();
    int slot_index = 0;
    for (const auto &entity_ref : inventory_dishes) {
      auto &entity = entity_ref.get();
      auto &dish = entity.get<IsDish>();

      nlohmann::json dish_entry;
      dish_entry["slot"] = slot_index++;
      dish_entry["dishType"] = magic_enum::enum_name(dish.type);
      dish_entry["level"] = entity.has<DishLevel>() ? entity.get<DishLevel>().level : 1; // issue 7
      if (entity.has<DrinkPairing>() && entity.get<DrinkPairing>().drink)
        dish_entry["drink"] = magic_enum::enum_name(*entity.get<DrinkPairing>().drink); // issue 8
      team.push_back(dish_entry);
    }

    // Build complete JSON - seed will come from server response
    nlohmann::json snapshot;
    snapshot["team"] = team;

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

    // Write file (no seed in filename since it comes from server).
    // Full epoch millis + monotonic counter: back-to-back battles in the
    // same millisecond must not reuse (and overwrite) a snapshot path.
    static std::atomic<long long> snapshot_counter{0};
    long long epoch_millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch())
            .count();
    std::string filename =
        fmt::format("output/battles/pending/{}_{}_{}.json", timestamp,
                    epoch_millis, snapshot_counter.fetch_add(1));
    std::ofstream file(filename);
    if (file.is_open()) {
      file << snapshot.dump(2);
      file.close();

      // Server-only battle flow (settled): snapshot file is kept for debug,
      // but the battle is created by ServerBattleRequestSystem - it uploads
      // the team, the server matches an opponent, and it sets the paths.
      BattleLoadRequest request;
      request.serverUrl = http_helpers::get_server_url();
      request.playerJsonPath = "";
      request.opponentJsonPath = "";

      if (afterhours::EntityHelper::has_singleton<BattleLoadRequest>()) {
        auto existingRequest =
            afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
        if (existingRequest.get().has<BattleLoadRequest>()) {
          auto &existingBattleRequest =
              existingRequest.get().get<BattleLoadRequest>();
          existingBattleRequest.serverUrl = request.serverUrl;
          existingBattleRequest.playerJsonPath = "";
          existingBattleRequest.opponentJsonPath = "";
          existingBattleRequest.loaded = false;
          existingBattleRequest.serverRequestPending = false;
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
