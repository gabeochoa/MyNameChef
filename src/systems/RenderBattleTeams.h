#pragma once

#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../components/transform.h"
#include "../font_info.h"
#include "../game_state_manager.h"
#include "../render_backend.h"
#include "../rl.h"
#include "../ui/text_formatting.h"
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

    // Only render entities that are in appropriate phases for the current
    // screen
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen == GameStateManager::Screen::Battle) {
      // On battle screen, show entities that are entering or in combat
      if (dbs.phase == DishBattleState::Phase::Finished) {
        return;
      }
    } else if (gsm.active_screen == GameStateManager::Screen::Results) {
      // On results screen, show all phases
    } else {
      // On other screens, don't render battle entities
      log_info(
          "OTHER SCREEN RENDER: Skipping entity {} (not battle/results screen)",
          entity.id);
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

    // Enter animation: while Entering, interpolate vertically toward the
    // battle midline based on enter_progress; once InCombat, draw at final
    // snapped Transform (BattleEnterAnimationSystem snaps it on completion).
    float present_offset_y = 0.0f;
    if (dbs.phase == DishBattleState::Phase::Entering) {
      float present_v = std::clamp(dbs.enter_progress, 0.0f, 1.0f);
      float battle_midline_y = 360.0f;
      present_offset_y = (battle_midline_y - transform.position.y) * present_v;
    }

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
        raylib::DrawRectangle((int)(transform.position.x + offset_x),
                              (int)(transform.position.y + offset_y),
                              (int)transform.size.x, (int)transform.size.y,
                              raylib::PINK);
        return;
      }

      raylib::Texture2D sheet = spritesheet_component->texture;

      float dest_width = hasSprite.frame.width * hasSprite.scale;
      float dest_height = hasSprite.frame.height * hasSprite.scale;

      raylib::DrawTexturePro(
          sheet, hasSprite.frame,
          Rectangle{transform.position.x + offset_x + transform.size.x / 2.f,
                    transform.position.y + offset_y + present_offset_y +
                        transform.size.y / 2.f,
                    dest_width, dest_height},
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

    raylib::Color success_color = text_formatting::TextFormatting::get_color(
        text_formatting::SemanticColor::Success,
        text_formatting::FormattingContext::Combat);
    raylib::Color error_color = text_formatting::TextFormatting::get_color(
        text_formatting::SemanticColor::Error,
        text_formatting::FormattingContext::Combat);
    raylib::Color text_color = text_formatting::TextFormatting::get_color(
        text_formatting::SemanticColor::Text,
        text_formatting::FormattingContext::Combat);

    // Draw border
    raylib::Color borderColor = isPlayer ? success_color : error_color;
    raylib::DrawRectangleLines(
        (int)(transform.position.x + offset_x),
        (int)(transform.position.y + offset_y + present_offset_y),
        (int)transform.size.x, (int)transform.size.y, borderColor);

    // Draw dish name above the rectangle
    std::string dishName = dish.name();
    float textWidth = render_backend::MeasureTextWithActiveFont(
        dishName.c_str(), font_sizes::Normal);
    float textX =
        transform.position.x + offset_x + (transform.size.x - textWidth) / 2.0f;
    float textY = transform.position.y + offset_y + present_offset_y - 25.0f;

    render_backend::DrawTextWithActiveFont(dishName.c_str(), (int)textX,
                                           (int)textY, font_sizes::Normal,
                                           text_color);

    // Draw team label
    std::string teamLabel = isPlayer ? "PLAYER" : "OPPONENT";
    float labelWidth = render_backend::MeasureTextWithActiveFont(
        teamLabel.c_str(), font_sizes::Small);
    float labelX = transform.position.x + offset_x +
                   (transform.size.x - labelWidth) / 2.0f;
    float labelY = transform.position.y + offset_y + present_offset_y +
                   transform.size.y + 5.0f;

    render_backend::DrawTextWithActiveFont(teamLabel.c_str(), (int)labelX,
                                           (int)labelY, font_sizes::Small,
                                           borderColor);
  }
};
