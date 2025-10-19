#pragma once

#include "../components/has_camera.h"
#include <afterhours/ah.h>

struct BeginWorldRender : System<> {
  virtual void once(float) const override {
    render_backend::BeginTextureMode(mainRT);
    render_backend::ClearBackground(raylib::DARKGRAY);
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
