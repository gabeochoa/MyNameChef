#pragma once

#include "../query.h"
#include "../shader_library.h"
#include "../shader_types.h"
#include "LetterboxLayout.h"
#include <afterhours/ah.h>

struct BeginTagShaderRender : System<> {
  virtual void once(float) const override {
    render_backend::BeginTextureMode(screenRT);

    // First draw mainRT to screenRT as background
    const int window_w = raylib::GetScreenWidth();
    const int window_h = raylib::GetScreenHeight();
    const int content_w = mainRT.texture.width;
    const int content_h = mainRT.texture.height;
    const LetterboxLayout layout =
        compute_letterbox_layout(window_w, window_h, content_w, content_h);
    const raylib::Rectangle src{0.0f, 0.0f, (float)mainRT.texture.width,
                                -(float)mainRT.texture.height};
    render_backend::DrawTexturePro(mainRT.texture, src, layout.dst,
                                   {0.0f, 0.0f}, 0.0f, raylib::WHITE);

    bool useTagShader =
        ShaderLibrary::get().contains(ShaderType::post_processing_tag);
    if (useTagShader) {
      const auto &shader =
          ShaderLibrary::get().get(ShaderType::post_processing_tag);
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

struct EndTagShaderRender : System<> {
  virtual void once(float) const override {
    bool useTagShader =
        ShaderLibrary::get().contains(ShaderType::post_processing_tag);
    if (useTagShader) {
      render_backend::EndShaderMode();
    }
    render_backend::EndTextureMode();
  }
};
