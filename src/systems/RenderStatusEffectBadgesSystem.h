#pragma once

#include "../components/dish_battle_state.h"
#include "../components/status_effects.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../render_backend.h"
#include <afterhours/ah.h>

using namespace afterhours;

struct RenderStatusEffectBadgesSystem
    : System<Transform, StatusEffects> {
  virtual bool should_run(float) override {
    GameStateManager &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(const Entity &, const Transform &transform,
                     const StatusEffects &status_effects,
                     float) const override {
    if (status_effects.effects.empty()) {
      return;
    }
    float badge_size = 8.0f;
    float badge_gap = 2.0f;
    for (size_t i = 0; i < status_effects.effects.size(); ++i) {
      raylib::Color badge_color = raylib::Color{255, 255, 255, 255};
      switch (status_effects.effects[i].type) {
      case StatusEffectType::Heat:
        badge_color = raylib::Color{255, 120, 0, 255};
        break;
      case StatusEffectType::SweetTooth:
        badge_color = raylib::Color{255, 150, 200, 255};
        break;
      case StatusEffectType::Fatigue:
        badge_color = raylib::Color{150, 150, 150, 255};
        break;
      }
      float x = transform.position.x + transform.size.x - badge_size -
                static_cast<float>(i) * (badge_size + badge_gap);
      float y = transform.position.y + badge_gap;
      raylib::DrawRectangle(static_cast<int>(x), static_cast<int>(y),
                            static_cast<int>(badge_size),
                            static_cast<int>(badge_size), badge_color);
    }
  }
};
