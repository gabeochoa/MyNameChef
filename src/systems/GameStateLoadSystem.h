#pragma once

#include "../components/battle_load_request.h"
#include "../components/continue_button_disabled.h"
#include "../components/continue_game_request.h"
#include "../components/dish_level.h"
#include "../components/game_state_loaded.h"
#include "../components/has_tooltip.h"
#include "../components/is_dish.h"
#include "../components/is_draggable.h"
#include "../components/is_drop_slot.h"
#include "../components/is_inventory_item.h"
#include "../components/network_info.h"
#include "../components/render_order.h"
#include "../components/transform.h"
#include "../components/user_id.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../log.h"
#include "../render_constants.h"
#include "../seeded_rng.h"
#include "../server/file_storage.h"
#include "../shop.h"
#include "../tooltip.h"
#include "../utils/battle_fingerprint.h"
#include "../utils/http_helpers.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/texture_manager.h>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <httplib.h>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <raylib/raylib.h>

struct GameStateLoadSystem : afterhours::System<ContinueGameRequest> {
  bool loaded = false;

  virtual bool should_run(float) override {
    if (loaded) {
      return false;
    }

    auto continue_opt =
        afterhours::EntityHelper::get_singleton<ContinueGameRequest>();
    if (!continue_opt.get().has<ContinueGameRequest>()) {
      return false;
    }

    auto &continue_req = continue_opt.get().get<ContinueGameRequest>();
    return continue_req.requested;
  }

  void once(float) override {
    loaded = true;

    auto userId_opt = afterhours::EntityHelper::get_singleton<UserId>();
    if (!userId_opt.get().has<UserId>()) {
      log_error("GAME_STATE_LOAD: UserId singleton not found");
      show_error_and_return();
      return;
    }

    std::string userId = userId_opt.get().get<UserId>().userId;
    std::string save_file =
        server::FileStorage::get_game_state_save_path(userId);

    if (!std::filesystem::exists(save_file)) {
      log_error("GAME_STATE_LOAD: Save file not found: {}", save_file);
      show_error_and_return();
      return;
    }

    std::ifstream file(save_file);
    if (!file.is_open()) {
      log_error("GAME_STATE_LOAD: Failed to open save file: {}", save_file);
      show_error_and_return();
      return;
    }

    nlohmann::json gameState;
    try {
      file >> gameState;
      file.close();
    } catch (const nlohmann::json::exception &e) {
      log_error("GAME_STATE_LOAD: Failed to parse save file: {}", e.what());
      show_error_and_return();
      return;
    }

    if (!gameState.contains("checksum")) {
      log_error("GAME_STATE_LOAD: Save file missing checksum");
      show_error_and_return();
      return;
    }

    std::string local_checksum = gameState["checksum"].get<std::string>();
    gameState.erase("checksum");

    std::string computed_checksum = compute_game_state_checksum(gameState);
    if (computed_checksum != local_checksum) {
      log_warn("GAME_STATE_LOAD: Local checksum mismatch, recomputing");
      local_checksum = computed_checksum;
    }

    std::string server_url = get_server_url();
    nlohmann::json server_state;

    if (!server_url.empty()) {
      server_state = fetch_server_state(userId, local_checksum, server_url);
    }

    if (!server_state.empty() && server_state.contains("gameState")) {
      log_info("GAME_STATE_LOAD: Server returned state, using server state");
      gameState = server_state["gameState"];

      std::ofstream out_file(save_file);
      if (out_file.is_open()) {
        out_file << gameState.dump();
        out_file.close();
      }
    } else if (!server_state.empty() && server_state.value("match", false)) {
      log_info("GAME_STATE_LOAD: Server checksum match, using local state");
    } else if (server_url.empty() || server_state.empty()) {
      log_warn(
          "GAME_STATE_LOAD: Server unavailable, using local state (warning)");
    }

    if (!restore_game_state(gameState)) {
      show_error_and_return();
      return;
    }

    auto shop_manager_opt =
        afterhours::EntityHelper::get_singleton<ShopState>();
    if (shop_manager_opt.get().has<ShopState>()) {
      shop_manager_opt.get().addComponent<GameStateLoaded>();
    }

    log_info("GAME_STATE_LOAD: Game state restored successfully");
  }

private:
  std::string get_server_url() {
    // Try to get server URL from NetworkInfo first
    auto server_addr_opt = NetworkInfo::get_server_address();
    if (server_addr_opt.has_value()) {
      const ServerAddress &addr = server_addr_opt.value();
      return fmt::format("http://{}:{}", addr.ip, addr.port);
    }

    // Fall back to environment variable or default
    return http_helpers::get_server_url();
  }

  nlohmann::json fetch_server_state(const std::string &userId,
                                    const std::string &checksum,
                                    const std::string &server_url) {
    try {
      http_helpers::ServerUrlParts url_parts =
          http_helpers::parse_server_url(server_url);
      if (!url_parts.success) {
        return nlohmann::json();
      }

      httplib::Client client(url_parts.host, url_parts.port);
      client.set_read_timeout(10, 0);
      client.set_connection_timeout(5, 0);

      std::string path =
          "/game-state?userId=" + userId + "&checksum=" + checksum;
      auto res = client.Get(path.c_str());

      if (res && res->status == 200) {
        return nlohmann::json::parse(res->body);
      }
    } catch (...) {
    }

    return nlohmann::json();
  }

  bool restore_game_state(const nlohmann::json &gameState) {
    try {
      if (gameState.contains("shopSeed")) {
        uint64_t seed = gameState["shopSeed"].get<uint64_t>();
        SeededRng::get().set_seed(seed);
        log_info("GAME_STATE_LOAD: Restored shop seed: {}", seed);
      }

      auto wallet_opt = afterhours::EntityHelper::get_singleton<Wallet>();
      auto health_opt = afterhours::EntityHelper::get_singleton<Health>();
      auto shop_tier_opt = afterhours::EntityHelper::get_singleton<ShopTier>();
      auto reroll_cost_opt =
          afterhours::EntityHelper::get_singleton<RerollCost>();

      if (!wallet_opt.get().has<Wallet>() || !health_opt.get().has<Health>() ||
          !shop_tier_opt.get().has<ShopTier>() ||
          !reroll_cost_opt.get().has<RerollCost>()) {
        log_error("GAME_STATE_LOAD: Required singletons not found");
        return false;
      }

      Wallet &wallet = wallet_opt.get().get<Wallet>();
      Health &health = health_opt.get().get<Health>();
      ShopTier &shop_tier = shop_tier_opt.get().get<ShopTier>();
      RerollCost &reroll_cost = reroll_cost_opt.get().get<RerollCost>();

      if (gameState.contains("gold")) {
        int gold_value = gameState["gold"].get<int>();
        wallet.gold = gold_value;
        log_info(
            "GAME_STATE_LOAD: Set wallet.gold to {} (wallet entity id: {})",
            gold_value, wallet_opt.get().id);
      }

      if (gameState.contains("health")) {
        health.current = gameState["health"]["current"].get<int>();
        health.max = gameState["health"]["max"].get<int>();
      }

      auto round_opt = afterhours::EntityHelper::get_singleton<Round>();
      if (gameState.contains("round")) {
        if (!round_opt.get().has<Round>()) {
          round_opt.get().addComponent<Round>();
        }
        round_opt.get().get<Round>().current = gameState["round"].get<int>();
      }

      if (gameState.contains("shopTier")) {
        shop_tier.current_tier = gameState["shopTier"].get<int>();
      }

      if (gameState.contains("rerollCost")) {
        reroll_cost.base = gameState["rerollCost"]["base"].get<int>();
        reroll_cost.increment = gameState["rerollCost"]["increment"].get<int>();
        reroll_cost.current = gameState["rerollCost"]["current"].get<int>();
      }

      if (gameState.contains("inventory") &&
          gameState["inventory"].is_array()) {
        for (const auto &dish_entry : gameState["inventory"]) {
          int slot = dish_entry["slot"].get<int>();
          std::string dish_type_str = dish_entry["dishType"].get<std::string>();
          int level = dish_entry.value("level", 1);

          auto dish_type_opt = magic_enum::enum_cast<DishType>(dish_type_str);
          if (dish_type_opt.has_value()) {
            auto dish_type = dish_type_opt.value();
            auto position = calculate_inventory_position(slot);
            auto &dish_entity = afterhours::EntityHelper::createEntity();

            dish_entity.addComponent<Transform>(position,
                                                vec2{SLOT_SIZE, SLOT_SIZE});
            dish_entity.addComponent<IsDish>(dish_type_opt.value());
            dish_entity.addComponent<DishLevel>(level);
            add_dish_tags(dish_entity, dish_type);
            dish_entity.addComponent<IsInventoryItem>();
            dish_entity.get<IsInventoryItem>().slot = slot;
            dish_entity.addComponent<IsDraggable>(true);
            dish_entity.addComponent<HasRenderOrder>(RenderOrder::ShopItems,
                                                     RenderScreen::Shop);

            auto dish_info = get_dish_info(dish_type_opt.value());
            const auto frame = afterhours::texture_manager::idx_to_sprite_frame(
                dish_info.sprite.i, dish_info.sprite.j);
            dish_entity.addComponent<afterhours::texture_manager::HasSprite>(
                position, vec2{SLOT_SIZE, SLOT_SIZE}, 0.f, frame,
                render_constants::kDishSpriteScale, raylib::WHITE);

            dish_entity.addComponent<HasTooltip>(
                generate_dish_tooltip(dish_type_opt.value()));

            for (auto &ref : afterhours::EntityQuery()
                                 .whereHasComponent<IsDropSlot>()
                                 .gen()) {
              auto &slot_entity = ref.get();
              if (slot_entity.get<IsDropSlot>().slot_id == slot) {
                slot_entity.get<IsDropSlot>().occupied = true;
                break;
              }
            }
          }
        }
      }

      int round_value = round_opt.get().has<Round>()
                            ? round_opt.get().get<Round>().current
                            : 1;
      log_info("GAME_STATE_LOAD: Restored shop state: gold={}, health={}/{}, "
               "tier={}, round={}",
               wallet.gold, health.current, health.max, shop_tier.current_tier,
               round_value);

      return true;
    } catch (const std::exception &e) {
      log_error("GAME_STATE_LOAD: Failed to restore game state: {}", e.what());
      return false;
    }
  }

  void show_error_and_return() {
    make_toast("Failed to load game state", 5.0f);

    auto continue_opt =
        afterhours::EntityHelper::get_singleton<ContinueGameRequest>();
    if (continue_opt.get().has<ContinueGameRequest>()) {
      continue_opt.get().get<ContinueGameRequest>().requested = false;
    }

    auto continue_disabled_opt =
        afterhours::EntityHelper::get_singleton<ContinueButtonDisabled>();
    if (!continue_disabled_opt.get().has<ContinueButtonDisabled>()) {
      continue_disabled_opt.get().addComponent<ContinueButtonDisabled>();
    }

    GameStateManager::get().set_next_screen(GameStateManager::Screen::Main);
  }
};
