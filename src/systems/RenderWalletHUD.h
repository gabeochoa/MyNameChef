#pragma once

#include "../font_info.h"
#include "../game_state_manager.h"
#include "../shop.h"
#include <afterhours/ah.h>
using namespace afterhours;

struct RenderWalletHUD : System<> {

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  virtual void once(float) const override {
    auto wallet_entity = EntityHelper::get_singleton<Wallet>();
    if (!wallet_entity.get().has<Wallet>())
      return;

    const auto &wallet = wallet_entity.get().get<Wallet>();
    std::string wallet_text = std::to_string(wallet.gold) + " gold";

    // TODO: Convert wallet display to UI element with label for better
    // testability Expected: Wallet should be a UI element with label "Gold:
    // 100" or similar This would allow
    // UITestHelpers::visible_ui_exists("Gold:") to work
    render_backend::DrawTextWithActiveFont(wallet_text.c_str(), 20, 80,
                                           font_sizes::Medium, raylib::GOLD);

    auto health_entity = EntityHelper::get_singleton<Health>();
    if (!health_entity.get().has<Health>())
      return;

    const auto &health = health_entity.get().get<Health>();
    std::string health_text = std::to_string(health.current) + "/" +
                              std::to_string(health.max) + " health";

    render_backend::DrawTextWithActiveFont(health_text.c_str(), 20, 110,
                                           font_sizes::Medium, raylib::RED);

    auto round_entity = EntityHelper::get_singleton<Round>();
    if (!round_entity.get().has<Round>())
      return;

    const auto &round = round_entity.get().get<Round>();
    std::string round_text = "Round " + std::to_string(round.current);

    render_backend::DrawTextWithActiveFont(round_text.c_str(), 20, 140,
                                           font_sizes::Medium, raylib::WHITE);
  }
};
