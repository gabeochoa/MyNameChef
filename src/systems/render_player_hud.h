#pragma once

#include "../components.h"
#include "../query.h"
#include <afterhours/ah.h>

struct RenderPlayerHUD : System<Transform, HasHealth> {

  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             const HasHealth &hasHealth, float) const override {
    // Always render health bar
    const float scale_x = 2.f;
    const float scale_y = 1.25f;

    raylib::Color color = entity.has_child_of<HasColor>()
                              ? entity.get_with_child<HasColor>().color()
                              : raylib::GREEN;

    float health_as_percent = static_cast<float>(hasHealth.amount) /
                              static_cast<float>(hasHealth.max_amount);

    vec2 rotation_origin{0, 0};

    // Render the red background bar
    raylib::DrawRectanglePro(
        Rectangle{
            transform.pos().x - ((transform.size.x * scale_x) / 2.f) +
                5.f, // Center with scaling
            transform.pos().y -
                (transform.size.y + 10.0f),    // Slightly above the entity
            transform.size.x * scale_x,        // Adjust length
            (transform.size.y / 4.f) * scale_y // Adjust height
        },
        rotation_origin, 0.0f, raylib::RED);

    // Render the green health bar
    raylib::DrawRectanglePro(
        Rectangle{
            transform.pos().x - ((transform.size.x * scale_x) / 2.f) +
                5.f, // Start at the same position as red bar
            transform.pos().y -
                (transform.size.y + 10.0f), // Same vertical position as red bar
            (transform.size.x * scale_x) *
                health_as_percent, // Adjust length based on health percentage
            (transform.size.y / 4.f) * scale_y // Adjust height
        },
        rotation_origin, 0.0f, color);

    // Simplified - no round manager logic
    render_lives(entity, transform, color);
  }

private:
  void render_lives(const Entity &entity, const Transform &transform,
                    raylib::Color color) const {
    if (!entity.has<HasMultipleLives>())
      return;

    const auto &hasMultipleLives = entity.get<HasMultipleLives>();
    float rad = 5.f;
    vec2 off{rad * 2 + 2, 0.f};
    for (int i = 0; i < hasMultipleLives.num_lives_remaining; i++) {
      raylib::DrawCircleV(
          transform.pos() -
              vec2{transform.size.x / 2.f, transform.size.y + 15.f + rad} +
              (off * (float)i),
          rad, color);
    }
  }

  void render_kills(const Entity &entity, const Transform &transform,
                    raylib::Color color) const {
    if (!entity.has<HasKillCountTracker>())
      return;

    const auto &hasKillCountTracker = entity.get<HasKillCountTracker>();
    std::string kills_text =
        std::to_string(hasKillCountTracker.kills) + " kills";
    float text_size = 12.f;

    raylib::DrawText(
        kills_text.c_str(), static_cast<int>(transform.pos().x - 30.f),
        static_cast<int>(transform.pos().y - transform.size.y - 25.f),
        static_cast<int>(text_size), color);
  }

  void render_tagger_indicator(const Entity &entity, const Transform &transform,
                               raylib::Color) const {
    // TODO add color to entity
    if (!entity.has<HasTagAndGoTracking>())
      return;

    const auto &taggerTracking = entity.get<HasTagAndGoTracking>();

    // Draw crown for the tagger
    if (taggerTracking.is_tagger) {
      // Draw a crown above the player who is "it"
      const float crown_size = 15.f;
      const float crown_y_offset = transform.size.y + 20.f;

      // Crown position (centered above the player)
      vec2 crown_pos = transform.pos() - vec2{crown_size / 2.f, crown_y_offset};

      // Draw crown using simple shapes
      raylib::Color crown_color = raylib::GOLD;

      // Crown base
      raylib::DrawRectangle(static_cast<int>(crown_pos.x),
                            static_cast<int>(crown_pos.y),
                            static_cast<int>(crown_size),
                            static_cast<int>(crown_size / 3.f), crown_color);

      // Crown points (3 triangles)
      float point_width = crown_size / 3.f;
      for (int i = 0; i < 3; i++) {
        float x = crown_pos.x + (i * point_width);
        raylib::DrawTriangle(
            vec2{x, crown_pos.y},
            vec2{x + point_width / 2.f, crown_pos.y - crown_size / 2.f},
            vec2{x + point_width, crown_pos.y}, crown_color);
      }

      // Crown jewels (small circles)
      raylib::DrawCircleV(crown_pos + vec2{crown_size / 2.f, crown_size / 6.f},
                          2.f, raylib::RED);
    }

    // Simplified - no round manager logic
    float current_time = static_cast<float>(raylib::GetTime());
    float tag_cooldown_time = 2.0f; // Default cooldown time
    if (current_time - taggerTracking.last_tag_time < tag_cooldown_time) {
      // TODO: Add pulsing animation to shield to make it more obvious
      // TODO: Add countdown timer above shield showing remaining safe time
      // Draw a shield above the player who is safe
      const float shield_size = 12.f;
      const float shield_y_offset = transform.size.y + 35.f; // Above crown

      // Shield position (centered above the player)
      vec2 shield_pos =
          transform.pos() - vec2{shield_size / 2.f, shield_y_offset};

      // Draw shield using simple shapes
      raylib::Color shield_color = raylib::SKYBLUE;

      // Shield base (triangle pointing down)
      raylib::DrawTriangle(
          vec2{shield_pos.x + shield_size / 2.f, shield_pos.y}, // Top point
          vec2{shield_pos.x, shield_pos.y + shield_size},       // Bottom left
          vec2{shield_pos.x + shield_size,
               shield_pos.y + shield_size}, // Bottom right
          shield_color);

      // Shield border
      raylib::DrawTriangleLines(
          vec2{shield_pos.x + shield_size / 2.f, shield_pos.y}, // Top point
          vec2{shield_pos.x, shield_pos.y + shield_size},       // Bottom left
          vec2{shield_pos.x + shield_size,
               shield_pos.y + shield_size}, // Bottom right
          raylib::WHITE);
    }
  }
};
