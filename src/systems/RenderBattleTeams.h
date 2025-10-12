#pragma once

#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../rl.h"
#include "BattleAnimations.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/animation.h>
#include <afterhours/src/plugins/texture_manager.h>

struct RenderBattleTeams : afterhours::System<Transform, IsDish> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle ||
           gsm.active_screen == GameStateManager::Screen::Results;
  }

  void for_each_with(const afterhours::Entity &entity,
                     const Transform &transform, const IsDish &dish,
                     float) const override {

    // Only render battle items
    if (!entity.has<DishBattleState>())
      return;
    const auto &dbs = entity.get<DishBattleState>();
    bool isPlayer = dbs.team_side == DishBattleState::TeamSide::Player;

    // Only render entities that are in appropriate phases for the current screen
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen == GameStateManager::Screen::Battle) {
      // On battle screen, only show InQueue and Presenting phases
      if (dbs.phase == DishBattleState::Phase::Judged) {
        return;
      }
    } else if (gsm.active_screen == GameStateManager::Screen::Results) {
      // On results screen, show all phases (including Judged)
      // No filtering needed
    } else {
      // On other screens, don't render battle entities
      return;
    }

    // Apply slide-in animation offset (vertical: player from top, opponent from
    // bottom)
    float slide_v = 1.0f;
    if (auto v = afterhours::animation::get_value(BattleAnimKey::SlideIn,
                                                  (size_t)entity.id);
        v.has_value()) {
      slide_v = std::clamp(v.value(), 0.0f, 1.0f);
    }

    float offset_x = 0.0f;
    float offset_y = 0.0f;
    if (isPlayer) {
      // Slide in from top
      float off = -(transform.position.y + transform.size.y + 20.0f);
      offset_y = (1.0f - slide_v) * off;
    } else {
      // Slide in from bottom
      float screen_h = raylib::GetScreenHeight();
      float off = (screen_h - transform.position.y) + 20.0f;
      offset_y = (1.0f - slide_v) * off;
    }

    // Presentation animation: move toward judges (center line)
    float present_v = dbs.phase == DishBattleState::Phase::Presenting
                          ? std::clamp(dbs.phase_progress, 0.0f, 1.0f)
                          : 0.0f;
    float judge_center_y = 360.0f; // rough judges row
    float present_offset_y =
        (judge_center_y - transform.position.y) * present_v;

    // Draw the dish sprite or fallback rectangle
    if (entity.has<afterhours::texture_manager::HasSprite>()) {
      const auto &hasSprite =
          entity.get<afterhours::texture_manager::HasSprite>();

      // Get spritesheet
      auto *spritesheet_component = afterhours::EntityHelper::get_singleton_cmp<
          afterhours::texture_manager::HasSpritesheet>();
      if (!spritesheet_component) {
        log_warn("No spritesheet found, rendering pink fallback for entity {}",
                 entity.id);
        raylib::DrawRectangle(
            (int)(transform.position.x + offset_x),
            (int)(transform.position.y + offset_y + present_offset_y),
            (int)transform.size.x, (int)transform.size.y, raylib::PINK);
        return;
      }

      raylib::Texture2D sheet = spritesheet_component->texture;

      float dest_width = hasSprite.frame.width * hasSprite.scale;
      float dest_height = hasSprite.frame.height * hasSprite.scale;

      raylib::DrawTexturePro(
          sheet, hasSprite.frame,
          Rectangle{
              transform.position.x + offset_x + transform.size.x / 2.f,
              transform.position.y + offset_y + present_offset_y +
                  transform.size.y / 2.f,
              dest_width,
              dest_height,
          },
          vec2{dest_width / 2.f, dest_height / 2.f}, 0.0f, raylib::WHITE);
    } else {
      // Fallback: draw pink rectangle and log warning
      log_warn("Battle dish entity {} has no HasSprite component, rendering "
               "pink fallback",
               entity.id);
      raylib::DrawRectangle(
          (int)(transform.position.x + offset_x),
          (int)(transform.position.y + offset_y + present_offset_y),
          (int)transform.size.x, (int)transform.size.y, raylib::PINK);
    }

    // Draw border
    raylib::Color borderColor = isPlayer ? raylib::GREEN : raylib::RED;
    raylib::DrawRectangleLines(
        (int)(transform.position.x + offset_x),
        (int)(transform.position.y + offset_y + present_offset_y),
        (int)transform.size.x, (int)transform.size.y, borderColor);

    // Draw dish name above the rectangle
    std::string dishName = dish.name();
    float textWidth = raylib::MeasureText(dishName.c_str(), 16);
    float textX =
        transform.position.x + offset_x + (transform.size.x - textWidth) / 2.0f;
    float textY = transform.position.y + offset_y + present_offset_y - 25.0f;

    raylib::DrawText(dishName.c_str(), (int)textX, (int)textY, 16,
                     raylib::WHITE);

    // Draw team label
    std::string teamLabel = isPlayer ? "PLAYER" : "OPPONENT";
    float labelWidth = raylib::MeasureText(teamLabel.c_str(), 12);
    float labelX = transform.position.x + offset_x +
                   (transform.size.x - labelWidth) / 2.0f;
    float labelY = transform.position.y + offset_y + present_offset_y +
                   transform.size.y + 5.0f;

    raylib::DrawText(teamLabel.c_str(), (int)labelX, (int)labelY, 12,
                     borderColor);
  }
};
