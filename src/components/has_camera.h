#pragma once

#include <afterhours/ah.h>

struct HasCamera : BaseComponent {
  raylib::Camera2D camera;

  HasCamera() {
    camera.offset = {0.0f, 0.0f};
    camera.target = {0.0f, 0.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
  }

  explicit HasCamera(raylib::Camera2D cam) : camera(cam) {}
};
