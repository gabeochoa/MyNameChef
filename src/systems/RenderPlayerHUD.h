#pragma once

#include "../components.h"
#include "../query.h"
#include "../shop.h"
#include <afterhours/ah.h>

struct RenderPlayerHUD : System<Transform, Wallet> {

  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             const Wallet &wallet, float) const override {

    std::string wallet_text = std::to_string(wallet.gold) + " gold";
    float text_size = 14.f;

    raylib::DrawText(
        wallet_text.c_str(), static_cast<int>(transform.pos().x - 40.f),
        static_cast<int>(transform.pos().y - transform.size.y - 45.f),
        static_cast<int>(text_size), raylib::GOLD);
  }
};
