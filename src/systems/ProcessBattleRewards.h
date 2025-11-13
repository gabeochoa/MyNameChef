#pragma once

#include "../components/battle_result.h"
#include "../game_state_manager.h"
#include "../shop.h"
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

    // Reset processed flag when leaving results screen
    if (last_screen == GameStateManager::Screen::Results &&
        gsm.active_screen != GameStateManager::Screen::Results) {
      processed = false;
    }

    last_screen = gsm.active_screen;

    // Only run once when on results screen and not yet processed
    return gsm.active_screen == GameStateManager::Screen::Results && !processed;
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

    processed = true;
  }

private:
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
    log_info("Store refilled with {} new items at tier {}", free_slots.size(),
             current_tier);
  }
};
