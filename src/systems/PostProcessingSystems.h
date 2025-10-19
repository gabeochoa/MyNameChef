#pragma once

#include "../query.h"
#include "../settings.h"
#include "../shader_library.h"
#include "../shader_types.h"
#include "LetterboxLayout.h"
#include <afterhours/ah.h>

struct SetupPostProcessingShader : System<> {
  virtual void once(float) const override {
    if (Settings::get().get_post_processing_enabled() &&
        ShaderLibrary::get().contains(ShaderType::post_processing)) {
      const auto &shader =
          ShaderLibrary::get().get(ShaderType::post_processing);
      render_backend::BeginShaderMode(shader);
      float t = static_cast<float>(raylib::GetTime());
      int timeLoc = raylib::GetShaderLocation(shader, "time");
      if (timeLoc != -1) {
        raylib::SetShaderValue(shader, timeLoc, &t,
                               raylib::SHADER_UNIFORM_FLOAT);
      }
      auto *rez = EntityHelper::get_singleton_cmp<
          window_manager::ProvidesCurrentResolution>();
      if (rez) {
        vec2 r = {static_cast<float>(rez->current_resolution.width),
                  static_cast<float>(rez->current_resolution.height)};
        int rezLoc = raylib::GetShaderLocation(shader, "resolution");
        if (rezLoc != -1) {
          raylib::SetShaderValue(shader, rezLoc, &r,
                                 raylib::SHADER_UNIFORM_VEC2);
        }
      }
    }
  }
};

struct RenderScreenToWindow : System<> {
  virtual void once(float) const override {
    const int window_w = raylib::GetScreenWidth();
    const int window_h = raylib::GetScreenHeight();
    const int content_w = screenRT.texture.width;
    const int content_h = screenRT.texture.height;
    const LetterboxLayout layout =
        compute_letterbox_layout(window_w, window_h, content_w, content_h);
    const raylib::Rectangle src{0.0f, 0.0f, (float)screenRT.texture.width,
                                -(float)screenRT.texture.height};
    render_backend::DrawTexturePro(screenRT.texture, src, layout.dst,
                                   {0.0f, 0.0f}, 0.0f, raylib::WHITE);
  }
};

struct EndPostProcessingShader : System<> {
  virtual void once(float) const override {
    if (Settings::get().get_post_processing_enabled() &&
        ShaderLibrary::get().contains(ShaderType::post_processing)) {
      render_backend::EndShaderMode();
    }
  }
};
