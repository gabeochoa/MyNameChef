#pragma once

#include "../shop.h"
#include <afterhours/ah.h>

struct RenderWalletHUD : System<> {
  virtual void once(float) const override {
    auto wallet_entity = EntityHelper::get_singleton<Wallet>();
    if (!wallet_entity.get().has<Wallet>())
      return;

    const auto &wallet = wallet_entity.get().get<Wallet>();
    std::string wallet_text = std::to_string(wallet.gold) + " gold";
    float text_size = 20.f;

    raylib::DrawText(wallet_text.c_str(), 20, 20, static_cast<int>(text_size),
                     raylib::GOLD);
  }
};
