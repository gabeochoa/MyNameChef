#pragma once

#include "../components/dish_level.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../components/user_id.h"
#include "../game_state_manager.h"
#include "../log.h"
#include "../seeded_rng.h"
#include "../server/file_storage.h"
#include "../shop.h"
#include "../utils/battle_fingerprint.h"
#include <afterhours/ah.h>
#include <chrono>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

// Local type aliases to avoid conflicts
using GameStateOptEntity = std::optional<std::reference_wrapper<afterhours::Entity>>;
using GameStateRefEntity = std::reference_wrapper<afterhours::Entity>;

struct GameStateSaveSystem : afterhours::System<> {
  struct SaveResult {
    nlohmann::json gameState;
    std::string checksum;
    bool success;
  };

  SaveResult save_game_state() {
    SaveResult result;
    result.success = false;

    GameStateOptEntity userId_opt = afterhours::EntityHelper::get_singleton<UserId>();
    if (!userId_opt.has_value() || !userId_opt->get().has<UserId>()) {
      log_error("GAME_STATE_SAVE: UserId singleton not found");
      return result;
    }
    std::string userId = userId_opt->get().get<UserId>().userId;

    std::vector<GameStateRefEntity> inventory_dishes;
    for (afterhours::Entity &entity : afterhours::EntityQuery()
                                          .whereHasComponent<IsInventoryItem>()
                                          .whereHasComponent<IsDish>()
                                          .gen()) {
      inventory_dishes.push_back(entity);
    }

    std::sort(inventory_dishes.begin(), inventory_dishes.end(),
              [](const GameStateRefEntity &a, const GameStateRefEntity &b) {
                return a.get().get<IsInventoryItem>().slot <
                       b.get().get<IsInventoryItem>().slot;
              });

    nlohmann::json inventory = nlohmann::json::array();
    for (const GameStateRefEntity &entity_ref : inventory_dishes) {
      afterhours::Entity &entity = entity_ref.get();
      IsInventoryItem &inventory_item = entity.get<IsInventoryItem>();
      IsDish &dish = entity.get<IsDish>();

      nlohmann::json dish_entry;
      dish_entry["slot"] = inventory_item.slot - 100;
      dish_entry["dishType"] = std::string(magic_enum::enum_name(dish.type));

      int level = 1;
      if (entity.has<DishLevel>()) {
        level = entity.get<DishLevel>().level;
      }
      dish_entry["level"] = level;

      inventory.push_back(dish_entry);
    }

    GameStateOptEntity wallet_opt = afterhours::EntityHelper::get_singleton<Wallet>();
    GameStateOptEntity health_opt = afterhours::EntityHelper::get_singleton<Health>();
    GameStateOptEntity round_opt = afterhours::EntityHelper::get_singleton<Round>();
    GameStateOptEntity shop_tier_opt =
        afterhours::EntityHelper::get_singleton<ShopTier>();
    GameStateOptEntity reroll_cost_opt =
        afterhours::EntityHelper::get_singleton<RerollCost>();

    if (!wallet_opt.has_value() || !wallet_opt->get().has<Wallet>() ||
        !health_opt.has_value() || !health_opt->get().has<Health>() ||
        !shop_tier_opt.has_value() || !shop_tier_opt->get().has<ShopTier>() ||
        !reroll_cost_opt.has_value() ||
        !reroll_cost_opt->get().has<RerollCost>()) {
      log_error("GAME_STATE_SAVE: Required singletons not found");
      return result;
    }

    Wallet &wallet = wallet_opt->get().get<Wallet>();
    Health &health = health_opt->get().get<Health>();
    if (!round_opt.has_value() || !round_opt->get().has<Round>()) {
      if (!round_opt.has_value()) {
        log_error("GAME_STATE_SAVE: Round singleton entity not found");
        return result;
      }
      round_opt->get().addComponent<Round>();
    }
    Round &round = round_opt->get().get<Round>();
    ShopTier &shop_tier = shop_tier_opt->get().get<ShopTier>();
    RerollCost &reroll_cost = reroll_cost_opt->get().get<RerollCost>();

    uint64_t shop_seed = SeededRng::get().seed;
    uint64_t timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();

    nlohmann::json gameState;
    gameState["shopSeed"] = shop_seed;
    gameState["inventory"] = inventory;
    gameState["gold"] = wallet.gold;
    gameState["health"] =
        nlohmann::json{{"current", health.current}, {"max", health.max}};
    gameState["round"] = round.current;
    gameState["shopTier"] = shop_tier.current_tier;
    gameState["rerollCost"] =
        nlohmann::json{{"base", reroll_cost.base},
                       {"increment", reroll_cost.increment},
                       {"current", reroll_cost.current}};
    gameState["userId"] = userId;
    gameState["timestamp"] = timestamp;
    gameState["clientVersion"] = GAME_STATE_CLIENT_VERSION;

    std::string checksum = compute_game_state_checksum(gameState);
    gameState["checksum"] = checksum;

    std::string save_file =
        server::FileStorage::get_game_state_save_path(userId);
    if (!server::FileStorage::save_json_to_file(save_file, gameState)) {
      log_error("GAME_STATE_SAVE: Failed to save game state to: {}", save_file);
      return result;
    }

    log_info("GAME_STATE_SAVE: Saved game state for user {} to {}", userId,
             save_file);
    log_info("GAME_STATE_SAVE: Checksum: {}", checksum);

    result.gameState = gameState;
    result.checksum = checksum;
    result.success = true;
    return result;
  }
};
