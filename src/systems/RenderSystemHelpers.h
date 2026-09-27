#pragma once

#include "../components/has_camera.h"
#include "../ui/cloche_theme.h"
#include <afterhours/ah.h>

struct BeginWorldRender : System<> {
  virtual void once(float) const override {
    render_backend::BeginTextureMode(mainRT);
    // DS two-tone page: light page left, palette-green page right,
    // seam at the center (Silver Cloche house theme).
    render_backend::ClearBackground(cloche_theme::PAGE_LEFT);
    if (mainRT.texture.width > 0 && mainRT.texture.height > 0) {
      float width = static_cast<float>(mainRT.texture.width);
      float height = static_cast<float>(mainRT.texture.height);
      raylib::DrawRectangle(static_cast<int>(width / 2.f), 0,
                            static_cast<int>(width - width / 2.f),
                            static_cast<int>(height), cloche_theme::PAGE_RIGHT);
      raylib::DrawRectangle(static_cast<int>(width / 2.f) - 1, 0, 2,
                            static_cast<int>(height), cloche_theme::SEAM);
    }
  }
};

struct EndWorldRender : System<> {
  virtual void once(float) const override { render_backend::EndTextureMode(); }
};

struct BeginCameraMode : System<HasCamera> {
  virtual void once(float) const override {
    auto *camera_entity = EntityHelper::get_singleton_cmp<HasCamera>();
    if (camera_entity) {
      render_backend::BeginMode2D(camera_entity->camera);
    }
  }
};

struct EndCameraMode : System<HasCamera> {
  virtual void once(float) const override {
    auto *camera_entity = EntityHelper::get_singleton_cmp<HasCamera>();
    if (camera_entity) {
      render_backend::EndMode2D();
    }
  }
};

struct BeginPostProcessingRender : System<> {
  virtual void once(float) const override { render_backend::BeginDrawing(); }
};

struct EndDrawing : System<> {
  virtual void once(float) const override { render_backend::EndDrawing(); }
};
