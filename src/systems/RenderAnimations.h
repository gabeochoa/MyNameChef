#pragma once

#include "../components/animation_event.h"
#include "../components/battle_anim_keys.h"
#include "../components/dish_battle_state.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../rl.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/animation.h>

// Unified system to render all animation effects
struct RenderAnimations : afterhours::System<AnimationEvent> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  virtual void for_each_with(const afterhours::Entity &animEntity,
                             const AnimationEvent &animEvent,
                             float) const override {

    switch (animEvent.type) {
    case AnimationEventType::StatBoost:
      render_stat_boost_animation(animEntity, animEvent);
      break;
    case AnimationEventType::FreshnessChain:
      render_freshness_chain_animation(animEntity, animEvent);
      break;
    case AnimationEventType::SlideIn:
      // SlideIn animations don't need visual rendering
      break;
    }
  }

private:
  void render_stat_boost_animation(const afterhours::Entity &animEntity,
                                   const AnimationEvent &) const {
    if (!animEntity.has<AnimationEvent>()) {
      return;
    }

    const auto &animEvent = animEntity.get<AnimationEvent>();

    // Get StatBoost data from variant
    const StatBoostData *statBoostData =
        std::get_if<StatBoostData>(&animEvent.data);
    if (!statBoostData) {
      return;
    }

    // Find the target dish entity
    auto targetOpt = afterhours::EntityQuery()
                         .whereID(statBoostData->targetEntityId)
                         .gen_first();
    if (!targetOpt) {
      return;
    }
    const auto &targetEntity = *targetOpt;

    // Target must be a battle dish with transform
    if (!targetEntity->has<DishBattleState>() ||
        !targetEntity->has<Transform>()) {
      return;
    }

    // Check if animation exists - if not, skip rendering
    auto progress = afterhours::animation::get_value(BattleAnimKey::StatBoost,
                                                     (size_t)animEntity.id);
    if (!progress.has_value()) {
      return;
    }

    float animProgress = std::clamp(progress.value(), 0.0f, 1.0f);
    if (animProgress >= 1.0f) {
      return;
    }

    const auto &transform = targetEntity->get<Transform>();
    const auto &dbs = targetEntity->get<DishBattleState>();

    // Calculate position with battle animation offsets
    auto [centerX, centerY] =
        calculate_animation_position(transform, dbs, targetEntity->id);

    // Animation effects
    float scale = 0.5f + (animProgress * 0.5f);
    float alpha = 255.0f * (1.0f - animProgress);
    float bounceY = centerY - (animProgress * 20.0f);

    // Draw +1 overlay
    const float overlaySize = 40.0f * scale;

    // Background circle
    raylib::Color bgColor = raylib::Color{255, 215, 0, (unsigned char)alpha};
    raylib::DrawCircle(static_cast<int>(centerX), static_cast<int>(bounceY),
                       static_cast<int>(overlaySize / 2.0f), bgColor);

    // Border
    raylib::Color borderColor =
        raylib::Color{255, 165, 0, (unsigned char)alpha};
    raylib::DrawCircleLines(static_cast<int>(centerX),
                            static_cast<int>(bounceY),
                            static_cast<int>(overlaySize / 2.0f), borderColor);

    // +1 text
    std::string boostText = "+1";
    int fontSize = static_cast<int>(overlaySize * 0.4f);
    int textWidth = raylib::MeasureText(boostText.c_str(), fontSize);

    raylib::Color textColor = raylib::Color{0, 0, 0, (unsigned char)alpha};
    raylib::DrawText(
        boostText.c_str(), static_cast<int>(centerX - textWidth / 2.0f),
        static_cast<int>(bounceY - fontSize / 2.0f), fontSize, textColor);

    // Draw stat type indicator
    std::string statType = "";
    if (statBoostData->zingDelta > 0) {
      statType = "ZING";
    } else if (statBoostData->bodyDelta > 0) {
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

  void render_freshness_chain_animation(const afterhours::Entity &animEntity,
                                        const AnimationEvent &) const {
    if (!animEntity.has<AnimationEvent>()) {
      return;
    }

    const auto &animEvent = animEntity.get<AnimationEvent>();

    // Get FreshnessChain data from variant
    const FreshnessChainData *freshnessData =
        std::get_if<FreshnessChainData>(&animEvent.data);
    if (!freshnessData) {
      return;
    }

    // Check if animation exists - if not, skip rendering
    auto progress = afterhours::animation::get_value(
        BattleAnimKey::FreshnessChain, (size_t)animEntity.id);
    if (!progress.has_value()) {
      return;
    }

    float animProgress = std::clamp(progress.value(), 0.0f, 1.0f);
    if (animProgress >= 1.0f) {
      return;
    }

    // Render freshness chain effect for each affected dish
    render_freshness_effect(freshnessData->sourceEntityId, animProgress, true);

    if (freshnessData->previousEntityId != -1) {
      render_freshness_effect(freshnessData->previousEntityId, animProgress,
                              false);
    }

    if (freshnessData->nextEntityId != -1) {
      render_freshness_effect(freshnessData->nextEntityId, animProgress, false);
    }
  }

  void render_freshness_effect(int entityId, float animProgress,
                               bool isSource) const {
    // Find the target dish entity
    auto targetOpt = afterhours::EntityQuery().whereID(entityId).gen_first();
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

    // Calculate position with battle animation offsets
    auto [centerX, centerY] =
        calculate_animation_position(transform, dbs, targetEntity->id);

    // Different effects for source vs affected dishes
    if (isSource) {
      // Source dish gets a pulsing blue glow
      float pulseScale = 1.0f + (sin(animProgress * 3.14159f * 4.0f) * 0.2f);
      float alpha = 255.0f * (1.0f - animProgress) * 0.6f;

      raylib::Color glowColor =
          raylib::Color{100, 150, 255, (unsigned char)alpha};
      float glowSize = 60.0f * pulseScale;

      raylib::DrawCircle(static_cast<int>(centerX), static_cast<int>(centerY),
                         static_cast<int>(glowSize), glowColor);
    } else {
      // Affected dishes get a green freshness boost indicator
      float scale = 0.3f + (animProgress * 0.7f);
      float alpha = 255.0f * (1.0f - animProgress);
      float bounceY = centerY - (animProgress * 15.0f);

      raylib::Color freshnessColor =
          raylib::Color{50, 200, 100, (unsigned char)alpha};
      float freshnessSize = 35.0f * scale;

      // Draw freshness symbol
      raylib::DrawCircle(static_cast<int>(centerX), static_cast<int>(bounceY),
                         static_cast<int>(freshnessSize), freshnessColor);

      // Draw "+1 Freshness" text
      raylib::Color textColor =
          raylib::Color{255, 255, 255, (unsigned char)alpha};
      raylib::DrawText("+1", static_cast<int>(centerX - 10),
                       static_cast<int>(bounceY - 5), 12, textColor);
    }
  }

  // Helper function to calculate animation position with battle offsets
  std::pair<float, float>
  calculate_animation_position(const Transform &transform,
                               const DishBattleState &dbs, int entityId) const {
    float offset_x = 0.0f;
    float offset_y = 0.0f;

    bool isPlayer = dbs.team_side == DishBattleState::TeamSide::Player;

    // Slide-in animation offset
    float slide_v = 1.0f;
    if (auto v = afterhours::animation::get_value(BattleAnimKey::SlideIn,
                                                  (size_t)entityId);
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

    // Calculate center position
    float centerX = transform.position.x + offset_x + transform.size.x / 2.0f;
    float centerY = transform.position.y + offset_y + transform.size.y / 2.0f;

    return {centerX, centerY};
  }
};
