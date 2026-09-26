#pragma once

#include "../components/battle_result.h"
#include "../components/is_drink_shop_item.h"
#include "../components/replay_state.h"
#include "../components/is_draggable.h"
#include "../components/render_order.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../render_constants.h"
#include "../utils/http_helpers.h"
#include <httplib.h>
#include "../shop.h"
#include "GameStateSaveSystem.h"
#include <afterhours/src/plugins/texture_manager.h>
#include <afterhours/ah.h>
#include <optional>
#include <vector>

using namespace afterhours;

// Local type alias to avoid conflicts
using ProcessBattleOptEntity =
    std::optional<std::reference_wrapper<afterhours::Entity>>;

struct ProcessBattleRewards : System<> {
  bool processed = false;
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();

    // Re-arm when a new battle starts.
    if (last_screen != GameStateManager::Screen::Battle &&
        gsm.active_screen == GameStateManager::Screen::Battle) {
      processed = false;
    }

    last_screen = gsm.active_screen;

    if (processed || gsm.active_screen == GameStateManager::Screen::Battle) {
      return false;
    }

    // Rewards belong to battle completion, not to the Results screen: the
    // screen can be flipped away (Skip / Back to Shop / harness waits)
    // before any tick observes it, which would lose the round increment.
    // Fire once a BattleResult exists outside the Battle screen.
    if (afterhours::EntityHelper::has_singleton<ReplayState>()) {
      auto replay_entity =
          afterhours::EntityHelper::get_singleton<ReplayState>();
      if (replay_entity.get().has<ReplayState>()) {
        const auto &replay = replay_entity.get().get<ReplayState>();
        if (replay.active && replay.from_history) {
          return false; // Replays never award rewards.
        }
      }
    }

    if (!afterhours::EntityHelper::has_singleton<BattleResult>()) {
      return false;
    }
    auto result_entity = EntityHelper::get_singleton<BattleResult>();
    return result_entity.get().has<BattleResult>();
  }

  void once(float) override {
    // Handle battle rewards
    ProcessBattleOptEntity resultEntity =
        EntityHelper::get_singleton<BattleResult>();
    if (!resultEntity.has_value() || !resultEntity->get().has<BattleResult>()) {
      return;
    }

    BattleResult &result = resultEntity->get().get<BattleResult>();

    // Award rewards based on battle outcome
    award_battle_rewards(result);

    // Increment round number
    ProcessBattleOptEntity round_entity = EntityHelper::get_singleton<Round>();
    if (round_entity.has_value() && round_entity->get().has<Round>()) {
      round_entity->get().get<Round>().current++;
      log_info("Round incremented to {}",
               round_entity->get().get<Round>().current);
    }

    // Refill store for next round
    refill_store();

    // Issue 50: persist the completed round transaction (rewards+round+refill)
    {
      GameStateSaveSystem saver;
      auto save_result = saver.save_game_state();
      if (!save_result.success) {
        log_warn("PROCESS_REWARDS: post-battle save failed");
      } else {
        upload_game_state(save_result);
      }
    }
    processed = true;
  }

private:
  // GameStateLoad prefers the server's copy (issue 43), so the post-battle
  // state must be uploaded too - otherwise Continue rolls back a battle.
  void upload_game_state(const GameStateSaveSystem::SaveResult &save_result) {
    auto url_parts =
        http_helpers::parse_server_url(http_helpers::get_server_url());
    if (!url_parts.success || url_parts.is_https) {
      return;
    }
    nlohmann::json save_request;
    save_request["userId"] = save_result.gameState["userId"];
    save_request["checksum"] = save_result.checksum;
    save_request["gameState"] = save_result.gameState;
    save_request["timestamp"] = save_result.gameState["timestamp"];
    httplib::Client client(url_parts.host, url_parts.port);
    client.set_read_timeout(5, 0);
    client.set_connection_timeout(5, 0);
    auto res = client.Post("/save-game-state", save_request.dump(),
                           "application/json");
    log_info("PROCESS_REWARDS: post-battle upload status {}",
             res ? res->status : -1);
    if (!res || res->status != 200) {
      log_warn("PROCESS_REWARDS: post-battle server upload failed");
    }
  }

  void award_battle_rewards(const BattleResult &result) {
    ProcessBattleOptEntity walletEntity = EntityHelper::get_singleton<Wallet>();
    ProcessBattleOptEntity healthEntity = EntityHelper::get_singleton<Health>();

    if (!walletEntity.has_value() || !walletEntity->get().has<Wallet>() ||
        !healthEntity.has_value() || !healthEntity->get().has<Health>()) {
      return;
    }

    Wallet &wallet = walletEntity->get().get<Wallet>();
    Health &health = healthEntity->get().get<Health>();

    // Award coins and health based on outcome
    switch (result.outcome) {
    case BattleResult::Outcome::PlayerWin:
      wallet.gold += 5;                                          // Win bonus
      health.current = std::min(health.max, health.current + 1); // Heal 1 HP
      log_info("Battle won! Awarded 5 gold and 1 health");
      break;
    case BattleResult::Outcome::OpponentWin:
      wallet.gold += 1;                                 // Participation reward
      health.current = std::max(1, health.current - 1); // Lose 1 HP
      log_info("Battle lost! Awarded 1 gold, lost 1 health");
      break;
    case BattleResult::Outcome::Tie:
      wallet.gold += 3; // Tie reward
      log_info("Battle tied! Awarded 3 gold");
      break;
    }
  }

  void refill_store() {
    std::vector<int> free_slots = get_free_slots(SHOP_SLOTS);

    // Get current shop tier
    ProcessBattleOptEntity shop_tier_entity =
        EntityHelper::get_singleton<ShopTier>();
    int current_tier = 1; // Default to tier 1
    if (shop_tier_entity.has_value() &&
        shop_tier_entity->get().has<ShopTier>()) {
      current_tier = shop_tier_entity->get().get<ShopTier>().current_tier;
    }

    for (int slot : free_slots) {
      make_shop_item(slot, get_random_dish_for_tier(current_tier));
    }
    // Issue 74: replenish each empty drink slot on round advancement
    { std::vector<bool> occ(DRINK_SHOP_SLOTS, false);
      for (auto &ref : EntityQuery({.force_merge = true}).whereHasComponent<IsDrinkShopItem>().gen()) { int s = ref.get().get<IsDrinkShopItem>().slot; if (s >= 0 && s < DRINK_SHOP_SLOTS) occ[s] = true; }
      float sx = static_cast<float>(raylib::GetScreenWidth()) - (2 * (SLOT_SIZE + SLOT_GAP)) - 50.0f;
      for (int s = 0; s < DRINK_SHOP_SLOTS; ++s) if (!occ[s]) {
        auto pos = calculate_slot_position(s, static_cast<int>(sx), DRINK_SHOP_START_Y, 2); auto dt = get_random_drink_for_tier(current_tier);
        auto &e = EntityHelper::createEntity(); e.addComponent<Transform>(pos, vec2{SLOT_SIZE, SLOT_SIZE}); e.addComponent<IsDrinkShopItem>(s, dt); e.addComponent<IsDraggable>(true); e.addComponent<HasRenderOrder>(RenderOrder::ShopItems, RenderScreen::Shop);
        auto info = get_drink_info(dt); const auto fr = afterhours::texture_manager::idx_to_sprite_frame(info.sprite.i, info.sprite.j); e.addComponent<afterhours::texture_manager::HasSprite>(pos, vec2{SLOT_SIZE, SLOT_SIZE}, 0.f, fr, render_constants::kDishSpriteScale, raylib::WHITE);
      } }
    log_info("Store refilled with {} new items at tier {}", free_slots.size(),
             current_tier);
  }
};
