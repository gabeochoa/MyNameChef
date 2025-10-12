#pragma once

#include "../components/dish_battle_state.h"
#include "../components/render_order.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/texture_manager.h>

struct RenderSpritesByOrder
    : System<afterhours::texture_manager::HasSprite, HasRenderOrder> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.is_game_active();
  }

  virtual void
  for_each_with(const Entity &entity,
                const afterhours::texture_manager::HasSprite &hasSprite,
                const HasRenderOrder &render_order, float) const override {
    // Check if this entity should render on the current screen
    auto &gsm = GameStateManager::get();
    RenderScreen current_screen = get_current_render_screen(gsm);

    if (!render_order.should_render_on_screen(current_screen)) {
      return;
    }

    // If this is a battle dish entity, skip here so the specialized
    // RenderBattleTeams system is the single source of truth for drawing.
    if (entity.has<DishBattleState>()) {
      if (gsm.active_screen == GameStateManager::Screen::Battle ||
          gsm.active_screen == GameStateManager::Screen::Results) {
        return;
      }
    }

    // Additional phase filtering for battle entities
    if (gsm.active_screen == GameStateManager::Screen::Battle &&
        entity.has<DishBattleState>()) {
      const auto &dbs = entity.get<DishBattleState>();
      if (dbs.phase == DishBattleState::Phase::Judged) {
        return;
      }
    }

    // Get spritesheet
    auto *spritesheet_component = EntityHelper::get_singleton_cmp<
        afterhours::texture_manager::HasSpritesheet>();
    if (!spritesheet_component) {
      return;
    }

    raylib::Texture2D sheet = spritesheet_component->texture;

    // Render the sprite
    raylib::DrawTexturePro(sheet, hasSprite.frame, hasSprite.destination(),
                           vec2{hasSprite.transform.size.x / 2.f,
                                hasSprite.transform.size.y / 2.f},
                           hasSprite.angle(), hasSprite.colorTint);
  }

private:
  RenderScreen get_current_render_screen(const GameStateManager &gsm) const {
    switch (gsm.active_screen) {
    case GameStateManager::Screen::Shop:
      return RenderScreen::Shop;
    case GameStateManager::Screen::Battle:
      return RenderScreen::Battle;
    case GameStateManager::Screen::Results:
      return RenderScreen::Results;
    case GameStateManager::Screen::Main:
    case GameStateManager::Screen::Settings:
    default:
      return RenderScreen::All; // Default to all screens for other screens
    }
  }
};
