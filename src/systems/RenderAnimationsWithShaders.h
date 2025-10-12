#pragma once

#include "../components.h"
#include "../query.h"
#include "../shader_library.h"
#include "../shader_types.h"
#include <afterhours/ah.h>

struct RenderAnimationsWithShaders
    : System<Transform, afterhours::texture_manager::HasAnimation, HasShader,
             HonkState> {
  virtual void for_each_with(const Entity &, const Transform &transform,
                             const afterhours::texture_manager::HasAnimation &,
                             const HasShader &hasShader, const HonkState &,
                             float) const override {
    if (hasShader.shaders.empty()) {
      log_warn("No shaders found in HasShader component");
      return;
    }

    ShaderType primary_shader = hasShader.shaders[0];
    if (!ShaderLibrary::get().contains(primary_shader)) {
      log_warn("Shader not found: {}", static_cast<int>(primary_shader));
      return;
    }

    // Apply shader
    const auto &shader = ShaderLibrary::get().get(primary_shader);
    raylib::BeginShaderMode(shader);

    // Render animation entities as SKYBLUE for visual distinction
    raylib::DrawRectanglePro(
        Rectangle{
            transform.center().x,
            transform.center().y,
            transform.size.x,
            transform.size.y,
        },
        vec2{transform.size.x / 2.f, transform.size.y / 2.f}, transform.angle,
        raylib::SKYBLUE);

    // End shader
    raylib::EndShaderMode();
  }
};
