#pragma once

#include "../components/transform.h"
#include "../components/has_shader.h"
#include "../query.h"
#include "../shader_library.h"
#include "../shader_types.h"
#include <afterhours/ah.h>

struct RenderEntities : System<Transform> {

  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             float) const override {
    if (entity.has<afterhours::texture_manager::HasSpritesheet>())
      return;
    if (entity.has<afterhours::texture_manager::HasAnimation>())
      return;

    // Check if entity has a shader and apply it
    bool has_shader = entity.has<HasShader>();
    raylib::Color render_color;

    if (has_shader) {
      const auto &shader_component = entity.get<HasShader>();

      if (!shader_component.shaders.empty()) {
        ShaderType primary_shader = shader_component.shaders[0];
        if (ShaderLibrary::get().contains(primary_shader)) {
          const auto &shader = ShaderLibrary::get().get(primary_shader);
          raylib::BeginShaderMode(shader);
          render_color = raylib::MAGENTA;
        } else {
          log_warn("Shader not found in library: {}",
                   static_cast<int>(primary_shader));
          has_shader = false;
          render_color = entity.has_child_of<HasColor>()
                             ? entity.get_with_child<HasColor>().color()
                             : raylib::RAYWHITE;
        }
      } else {
        render_color = entity.has_child_of<HasColor>()
                           ? entity.get_with_child<HasColor>().color()
                           : raylib::RAYWHITE;
      }

      raylib::DrawRectanglePro(
          Rectangle{
              transform.center().x,
              transform.center().y,
              transform.size.x,
              transform.size.y,
          },
          vec2{transform.size.x / 2.f, transform.size.y / 2.f}, transform.angle,
          render_color);

      // End shader mode if we started it
      if (has_shader) {
        raylib::EndShaderMode();
      }
    } else {
      // Render entities without shaders using their color
      raylib::Color fallback_color =
          entity.has_child_of<HasColor>()
              ? entity.get_with_child<HasColor>().color()
              : raylib::RAYWHITE;

      raylib::DrawRectanglePro(
          Rectangle{
              transform.center().x,
              transform.center().y,
              transform.size.x,
              transform.size.y,
          },
          vec2{transform.size.x / 2.f, transform.size.y / 2.f}, transform.angle,
          fallback_color);
    }
  };
};
