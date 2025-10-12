#pragma once

#include "../components/has_camera.h"
#include <afterhours/ah.h>

struct BeginWorldRender : System<> {
  virtual void once(float) const override {
    raylib::BeginTextureMode(mainRT);
    raylib::ClearBackground(raylib::DARKGRAY);
  }
};

struct EndWorldRender : System<> {
  virtual void once(float) const override { raylib::EndTextureMode(); }
};

struct BeginCameraMode : System<HasCamera> {
  virtual void once(float) const override {
    auto *camera_entity = EntityHelper::get_singleton_cmp<HasCamera>();
    if (camera_entity) {
      raylib::BeginMode2D(camera_entity->camera);
    }
  }
};

struct EndCameraMode : System<HasCamera> {
  virtual void once(float) const override {
    auto *camera_entity = EntityHelper::get_singleton_cmp<HasCamera>();
    if (camera_entity) {
      raylib::EndMode2D();
    }
  }
};

struct BeginPostProcessingRender : System<> {
  virtual void once(float) const override { raylib::BeginDrawing(); }
};

struct EndDrawing : System<> {
  virtual void once(float) const override { raylib::EndDrawing(); }
};
