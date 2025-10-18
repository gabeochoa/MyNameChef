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

// Draw freshness chain visual effect during FreshnessChain animations
struct RenderFreshnessChainEffect
    : afterhours::System<AnimationEvent, FreshnessChainAnimation> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  virtual void for_each_with(const afterhours::Entity &animEntity,
                             const AnimationEvent &animEvent,
                             const FreshnessChainAnimation &freshnessAnim,
                             float) const override {

    // Only render FreshnessChain animations
    if (animEvent.type != AnimationEventType::FreshnessChain) {
      return;
    }

    // Check if animation exists - if not, skip rendering
    auto progress = afterhours::animation::get_value(
        BattleAnimKey::FreshnessChain, (size_t)animEntity.id);
    if (!progress.has_value()) {
      // Animation hasn't been scheduled yet, skip rendering
      return;
    }

    float animProgress = std::clamp(progress.value(), 0.0f, 1.0f);

    // Skip if animation is complete
    if (animProgress >= 1.0f) {
      return;
    }

    // Render freshness chain effect for each affected dish
    render_freshness_effect(freshnessAnim.sourceEntityId, animProgress, true);

    if (freshnessAnim.previousEntityId != -1) {
      render_freshness_effect(freshnessAnim.previousEntityId, animProgress,
                              false);
    }

    if (freshnessAnim.nextEntityId != -1) {
      render_freshness_effect(freshnessAnim.nextEntityId, animProgress, false);
    }
  }

private:
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

    // Calculate center position
    float centerX = transform.position.x + offset_x + transform.size.x / 2.0f;
    float centerY = transform.position.y + offset_y + transform.size.y / 2.0f;

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

      // Draw freshness symbol (leaf-like shape)
      raylib::DrawCircle(static_cast<int>(centerX), static_cast<int>(bounceY),
                         static_cast<int>(freshnessSize), freshnessColor);

      // Draw "+1 Freshness" text
      raylib::Color textColor =
          raylib::Color{255, 255, 255, (unsigned char)alpha};
      raylib::DrawText("+1", static_cast<int>(centerX - 10),
                       static_cast<int>(bounceY - 5), 12, textColor);
    }
  }
};
