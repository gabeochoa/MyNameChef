#pragma once

#include "../components/battle_result.h"
#include "../game_state_manager.h"
#include "../shop.h"
#include <afterhours/ah.h>

using namespace afterhours;

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
    auto resultEntity = EntityHelper::get_singleton<BattleResult>();
    if (!resultEntity.get().has<BattleResult>()) {
      return;
    }

    auto &result = resultEntity.get().get<BattleResult>();

    // Award rewards based on battle outcome
    award_battle_rewards(result);

    // Refill store for next round
    refill_store();

    processed = true;
  }

private:
  void award_battle_rewards(const BattleResult &result) {
    auto walletEntity = EntityHelper::get_singleton<Wallet>();
    auto healthEntity = EntityHelper::get_singleton<Health>();

    if (!walletEntity.get().has<Wallet>() ||
        !healthEntity.get().has<Health>()) {
      return;
    }

    auto &wallet = walletEntity.get().get<Wallet>();
    auto &health = healthEntity.get().get<Health>();

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
    auto free_slots = get_free_slots(SHOP_SLOTS);
    for (int slot : free_slots) {
      make_shop_item(slot, get_random_dish());
    }
    EntityHelper::merge_entity_arrays();
    log_info("Store refilled with {} new items", free_slots.size());
  }
};
