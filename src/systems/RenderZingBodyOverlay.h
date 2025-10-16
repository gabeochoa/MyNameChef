#pragma once

#include "../components/dish_battle_state.h"
#include "../components/dish_level.h"
#include "../components/is_dish.h"
#include "../components/render_order.h"
#include "../components/transform.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../rl.h"
#include "BattleAnimations.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/animation.h>

// Draw Zing/Body badges on top of dish sprites (top-left: Zing, top-right:
// Body)
struct RenderZingBodyOverlay : afterhours::System<HasRenderOrder, IsDish> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return GameStateManager::should_render_world_entities(gsm.active_screen);
  }

  virtual void for_each_with(const afterhours::Entity &entity,
                             const HasRenderOrder &render_order,
                             const IsDish &is_dish, float) const override {
    // Do not render overlays for entities marked for cleanup
    if (entity.cleanup)
      return;
    // Only overlay on battle/result dishes; skip shop/inventory duplicates
    if (!entity.has<DishBattleState>())
      return;
    const DishBattleState &dbs_check = entity.get<DishBattleState>();

    // Respect screen render flags
    auto &gsm = GameStateManager::get();
    RenderScreen current_screen = static_cast<RenderScreen>(
        GameStateManager::render_screen_for(gsm.active_screen));
    if (!render_order.should_render_on_screen(current_screen))
      return;

    // On battle screen, skip overlays for finished dishes
    if (GameStateManager::get().active_screen ==
        GameStateManager::Screen::Battle) {
      if (dbs_check.phase == DishBattleState::Phase::Finished)
        return;
    }
    if (!entity.has<Transform>())
      return;
    const auto &transform = entity.get<Transform>();

    // Compute on-screen rect including battle animation offsets so the overlay
    // follows the moving sprite
    float offset_x = 0.0f;
    float offset_y = 0.0f;

    if (entity.has<DishBattleState>()) {
      const auto &dbs = entity.get<DishBattleState>();
      bool isPlayer = dbs.team_side == DishBattleState::TeamSide::Player;

      // Slide-in animation (vertical)
      float slide_v = 1.0f;
      if (auto v = afterhours::animation::get_value(BattleAnimKey::SlideIn,
                                                    (size_t)entity.id);
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

      // While entering, mirror the RenderBattleTeams midline ease so overlays
      // stay glued to the sprite. Once InCombat, position remains at
      // snapped transform.
      if (dbs.phase == DishBattleState::Phase::Entering &&
          dbs.enter_progress >= 0.0f) {
        float present_v = std::clamp(dbs.enter_progress, 0.0f, 1.0f);
        float judge_center_y = 360.0f;
        offset_y += (judge_center_y - transform.position.y) * present_v;
      }
    }

    // Compute Zing and Body from FlavorStats using per-point sums
    const FlavorStats flavor = is_dish.flavor();
    int zing = flavor.zing();
    int body = flavor.body();

    // Level scaling: if level > 1, multiply both by 2
    if (entity.has<DishLevel>()) {
      const auto &lvl = entity.get<DishLevel>();
      if (lvl.level > 1) {
        zing *= 2;
        body *= 2;
      }
    }

    // Badge sizes relative to sprite rect
    const Rectangle rect = Rectangle{transform.position.x + offset_x,
                                     transform.position.y + offset_y,
                                     transform.size.x, transform.size.y};
    const float badgeSize =
        std::max(18.0f, std::min(rect.width, rect.height) * 0.26f);
    const float padding = 5.0f;

    // Zing: green rhombus (diamond) top-left
    const float zx = rect.x + padding + badgeSize * 0.5f;
    const float zy = rect.y + padding + badgeSize * 0.5f;
    // Zing: red
    raylib::Color zingColor = raylib::Color{200, 40, 40, 245};
    // Draw as a rotated square to guarantee a full diamond
    Rectangle diamond{zx, zy, badgeSize, badgeSize};
    raylib::DrawRectanglePro(diamond, vec2{badgeSize * 0.5f, badgeSize * 0.5f},
                             45.0f, zingColor);

    // Zing number (supports up to two digits)
    const int fontSize = static_cast<int>(badgeSize * 0.72f);
    const std::string zingText = std::to_string(zing);
    const int zw = raylib::MeasureText(zingText.c_str(), fontSize);
    raylib::DrawText(zingText.c_str(), static_cast<int>(zx - zw / 2.0f),
                     static_cast<int>(zy - fontSize / 2.0f), fontSize,
                     raylib::BLACK);

    // Body: pale yellow square top-right
    const float bx = rect.x + rect.width - padding - badgeSize;
    const float by = rect.y + padding;
    // Body: green
    raylib::Color bodyColor = raylib::Color{30, 160, 70, 245};
    raylib::DrawRectangle(static_cast<int>(bx), static_cast<int>(by),
                          static_cast<int>(badgeSize),
                          static_cast<int>(badgeSize), bodyColor);

    const std::string bodyText = std::to_string(body);
    const int bw = raylib::MeasureText(bodyText.c_str(), fontSize);
    const int bh = fontSize;
    raylib::DrawText(bodyText.c_str(),
                     static_cast<int>(bx + (badgeSize - bw) * 0.5f),
                     static_cast<int>(by + (badgeSize - bh) * 0.5f), fontSize,
                     raylib::BLACK);
  }
};
