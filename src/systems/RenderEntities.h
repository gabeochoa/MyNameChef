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

struct RenderEntities : System<Transform> {

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             float) const override {
    // Skip entities that have sprites - they should be rendered by sprite
    // systems
    if (entity.has<afterhours::texture_manager::HasSprite>()) {
      return;
    }

    // Only runs on Shop; no filtering needed for Battle here
    raylib::Color color = raylib::RAYWHITE;

    // Check if this is a dish entity without a sprite (fallback case)
    if (entity.has<IsDish>() &&
        !entity.has<afterhours::texture_manager::HasSprite>()) {
      log_warn(
          "Dish entity {} has no HasSprite component, rendering pink fallback",
          entity.id);
      color = raylib::PINK;
      raylib::DrawRectangle(static_cast<int>(transform.position.x),
                            static_cast<int>(transform.position.y),
                            static_cast<int>(transform.size.x),
                            static_cast<int>(transform.size.y), color);
    } else if (entity.has_child_of<HasColor>()) {
      color = entity.get_with_child<HasColor>().color();

      raylib::DrawRectangle(static_cast<int>(transform.position.x),
                            static_cast<int>(transform.position.y),
                            static_cast<int>(transform.size.x),
                            static_cast<int>(transform.size.y), color);
    }
  };

  virtual void once(float) const override {
    // Collect all entities with Transform and HasRenderOrder
    std::vector<std::pair<const Entity *, int>> entities_to_render;

    for (auto &ref : EntityQuery()
                         .whereHasComponent<Transform>()
                         .whereHasComponent<HasRenderOrder>()
                         .gen()) {
      auto &entity = ref.get();
      auto render_order = entity.get<HasRenderOrder>().order;
      entities_to_render.emplace_back(&entity, static_cast<int>(render_order));
    }

    // Sort by render order
    std::sort(entities_to_render.begin(), entities_to_render.end(),
              [](const auto &a, const auto &b) { return a.second < b.second; });

    // Render in sorted order
    for (const auto &pair : entities_to_render) {
      const Entity &entity = *pair.first;
      const Transform &transform = entity.get<Transform>();

      // Skip entities that have sprites - they should be rendered by sprite
      // systems
      if (entity.has<afterhours::texture_manager::HasSprite>()) {
        continue;
      }

      raylib::Color color = raylib::RAYWHITE;

      // Check if this is a dish entity without a sprite (fallback case)
      if (entity.has<IsDish>() &&
          !entity.has<afterhours::texture_manager::HasSprite>()) {
        log_warn("Dish entity {} has no HasSprite component, rendering pink "
                 "fallback",
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
  }
};
