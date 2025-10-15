#pragma once

#include "../components/is_dish.h"
#include "../components/render_order.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/color.h>
#include <afterhours/src/plugins/texture_manager.h>
#include <algorithm>
#include <vector>

struct RenderEntitiesByOrder : System<Transform, HasRenderOrder> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return GameStateManager::should_render_world_entities(gsm.active_screen);
  }

  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             const HasRenderOrder &render_order,
                             float) const override {
    // Check if this entity should render on the current screen
    auto &gsm = GameStateManager::get();
    RenderScreen current_screen = get_current_render_screen(gsm);

    if (!render_order.should_render_on_screen(current_screen)) {
      return;
    }

    // Skip entities that have sprites - they should be rendered by sprite
    // systems
    if (entity.has<afterhours::texture_manager::HasSprite>()) {
      return;
    }

    raylib::Color color = raylib::RAYWHITE;

    // Check if this is a dish entity without a sprite (fallback case)
    if (entity.has<IsDish>() &&
        !entity.has<afterhours::texture_manager::HasSprite>()) {
      log_warn(
          "Dish entity {} has no HasSprite component, rendering pink fallback",
          entity.id);
      color = raylib::PINK;
    } else if (entity.has_child_of<HasColor>()) {
      color = entity.get_with_child<HasColor>().color();
    }

    raylib::DrawRectangle(static_cast<int>(transform.position.x),
                          static_cast<int>(transform.position.y),
                          static_cast<int>(transform.size.x),
                          static_cast<int>(transform.size.y), color);
  }

private:
  RenderScreen get_current_render_screen(const GameStateManager &gsm) const {
    // Use centralized policy mapping
    return static_cast<RenderScreen>(
        GameStateManager::render_screen_for(gsm.active_screen));
  }
};
