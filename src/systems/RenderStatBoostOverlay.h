#pragma once

#include "../components/animation_event.h"
#include "../components/battle_anim_keys.h"
#include "../components/dish_battle_state.h"
#include "../components/render_order.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../rl.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/animation.h>

// Draw +1 stat boost overlay during StatBoost animations
struct RenderStatBoostOverlay
    : afterhours::System<AnimationEvent, StatBoostAnimation> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  virtual void for_each_with(const afterhours::Entity &animEntity,
                             const AnimationEvent &animEvent,
                             const StatBoostAnimation &statBoost,
                             float) const override {

    // Only render StatBoost animations
    if (animEvent.type != AnimationEventType::StatBoost) {
      return;
    }

    // Find the target dish entity
    auto targetOpt =
        afterhours::EntityQuery().whereID(statBoost.targetEntityId).gen_first();
    if (!targetOpt) {
      return;
    }
    const auto &targetEntity = *targetOpt;

    // Target must be a battle dish with transform
    if (!targetEntity->has<DishBattleState>() ||
        !targetEntity->has<Transform>()) {
      return;
    }

    const auto &transform = targetEntity->get<Transform>();
    const auto &dbs = targetEntity->get<DishBattleState>();

    // Check if animation exists - if not, skip rendering
    auto progress = afterhours::animation::get_value(BattleAnimKey::StatBoost,
                                                     (size_t)animEntity.id);
    if (!progress.has_value()) {
      // Animation hasn't been scheduled yet, skip rendering
      return;
    }

    float animProgress = std::clamp(progress.value(), 0.0f, 1.0f);

    // Skip if animation is complete
    if (animProgress >= 1.0f) {
      return;
    }

    // Compute position including battle animation offsets
    float offset_x = 0.0f;
    float offset_y = 0.0f;

    bool isPlayer = dbs.team_side == DishBattleState::TeamSide::Player;

    // Slide-in animation offset
    float slide_v = 1.0f;
    if (auto v = afterhours::animation::get_value(BattleAnimKey::SlideIn,
                                                  (size_t)targetEntity->id);
        v.has_value()) {
      slide_v = std::clamp(v.value(), 0.0f, 1.0f);
    }

    if (isPlayer) {
      float off = -(transform.position.y + transform.size.y + 20.0f);
      offset_y = (1.0f - slide_v) * off;
    } else {
      float screen_h = raylib::GetScreenHeight();
      float off = (screen_h - transform.position.y) + 20.0f;
      offset_y = (1.0f - slide_v) * off;
    }

    // Entering animation offset
    if (dbs.phase == DishBattleState::Phase::Entering &&
        dbs.enter_progress >= 0.0f) {
      float present_v = std::clamp(dbs.enter_progress, 0.0f, 1.0f);
      float judge_center_y = 360.0f;
      offset_y += (judge_center_y - transform.position.y) * present_v;
    }

    // Calculate overlay position (center of the dish)
    float centerX = transform.position.x + offset_x + transform.size.x / 2.0f;
    float centerY = transform.position.y + offset_y + transform.size.y / 2.0f;

    // Animation effects
    float scale = 0.5f + (animProgress * 0.5f);       // Scale from 0.5 to 1.0
    float alpha = 255.0f * (1.0f - animProgress);     // Fade out
    float bounceY = centerY - (animProgress * 20.0f); // Bounce up

    // Draw +1 overlay
    const float overlaySize = 40.0f * scale;

    // Background circle
    raylib::Color bgColor =
        raylib::Color{255, 215, 0, (unsigned char)alpha}; // Gold
    raylib::DrawCircle(static_cast<int>(centerX), static_cast<int>(bounceY),
                       static_cast<int>(overlaySize / 2.0f), bgColor);

    // Border
    raylib::Color borderColor =
        raylib::Color{255, 165, 0, (unsigned char)alpha}; // Orange
    raylib::DrawCircleLines(static_cast<int>(centerX),
                            static_cast<int>(bounceY),
                            static_cast<int>(overlaySize / 2.0f), borderColor);

    // +1 text
    std::string boostText = "+1";
    int fontSize = static_cast<int>(overlaySize * 0.4f);
    int textWidth = raylib::MeasureText(boostText.c_str(), fontSize);
    int textHeight = fontSize;

    raylib::Color textColor =
        raylib::Color{0, 0, 0, (unsigned char)alpha}; // Black
    raylib::DrawText(
        boostText.c_str(), static_cast<int>(centerX - textWidth / 2.0f),
        static_cast<int>(bounceY - textHeight / 2.0f), fontSize, textColor);

    // Draw stat type indicator (small text below)
    std::string statType = "";
    if (statBoost.zingDelta > 0) {
      statType = "ZING";
    } else if (statBoost.bodyDelta > 0) {
      statType = "BODY";
    }

    if (!statType.empty()) {
      int smallFontSize = static_cast<int>(overlaySize * 0.2f);
      int smallTextWidth = raylib::MeasureText(statType.c_str(), smallFontSize);
      raylib::DrawText(statType.c_str(),
                       static_cast<int>(centerX - smallTextWidth / 2.0f),
                       static_cast<int>(bounceY + overlaySize / 2.0f + 5.0f),
                       smallFontSize, textColor);
    }
  }
};
