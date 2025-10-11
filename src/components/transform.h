#pragma once

#include "../rl.h"
#include <afterhours/ah.h>

struct CollisionConfig {
  float mass = 1.0f;
  float restitution =
      0.5f; // Bounciness (0.0 = no bounce, 1.0 = perfect bounce)
  float friction =
      0.7f; // Surface friction (0.0 = no friction, 1.0 = max friction)
  bool is_static = false; // If true, object doesn't move from collisions
  bool is_trigger =
      false; // If true, collision doesn't affect physics but triggers events
};

struct Transform : afterhours::BaseComponent {
  vec2 position{0, 0};
  vec2 size{32, 32};
  float angle = 0.0f;
  vec2 velocity{0, 0};
  float accel = 0.0f;
  float accel_mult = 1.0f;
  float speed_dot_angle = 0.0f;
  bool render_out_of_bounds = true;
  bool cleanup_out_of_bounds = false;
  CollisionConfig collision_config;

  Transform() = default;
  Transform(vec2 pos, vec2 sz, float ang = 0.0f)
      : position(pos), size(sz), angle(ang) {}

  vec2 pos() const { return position; }
  vec2 center() const { return position + size * 0.5f; }
  Rectangle rect() const {
    return Rectangle{position.x, position.y, size.x, size.y};
  }
  float speed() const { return Vector2Length(velocity); }
  float as_rad() const { return angle * M_PI / 180.0f; }
  bool is_reversing() const { return speed_dot_angle < 0.0f; }
};
