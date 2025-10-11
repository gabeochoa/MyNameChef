#pragma once

#include "../query.h"
#include "../shader_library.h"
#include "../shader_types.h"
#include <afterhours/ah.h>

struct BeginTagShaderRender : System<> {
  virtual void once(float) const override {
    raylib::BeginTextureMode(screenRT);
    raylib::ClearBackground(raylib::BLANK);

    bool useTagShader =
        ShaderLibrary::get().contains(ShaderType::post_processing_tag);
    if (useTagShader) {
      const auto &shader =
          ShaderLibrary::get().get(ShaderType::post_processing_tag);
      raylib::BeginShaderMode(shader);
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
      raylib::EndShaderMode();
    }
    raylib::EndTextureMode();
  }
};
