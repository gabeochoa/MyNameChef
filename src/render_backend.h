#pragma once

#include "font_info.h"
#include "rl.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/autolayout.h>

namespace render_backend {
extern bool is_headless_mode;
extern bool is_offscreen_render_mode; // Headless + hidden window: real GL
                                      // rendering, nothing on screen
// Rendering is active in normal mode and in offscreen-render mode.
inline bool should_render() {
  return !is_headless_mode || is_offscreen_render_mode;
}
extern int step_delay_ms; // Delay between test steps in non-headless mode
                          // (milliseconds)
extern float timing_speed_scale; // Scale factor for timing-based waits and
                                 // animations (1.0 = normal speed)

// Drawing context management
inline void BeginDrawing() {
  if (should_render())
    raylib::BeginDrawing();
}

inline void EndDrawing() {
  if (should_render())
    raylib::EndDrawing();
}

inline void BeginTextureMode(raylib::RenderTexture2D target) {
  if (should_render())
    raylib::BeginTextureMode(target);
}

inline void EndTextureMode() {
  if (should_render())
    raylib::EndTextureMode();
}

inline void BeginMode2D(raylib::Camera2D camera) {
  if (should_render())
    raylib::BeginMode2D(camera);
}

inline void EndMode2D() {
  if (should_render())
    raylib::EndMode2D();
}

inline void BeginShaderMode(raylib::Shader shader) {
  if (should_render())
    raylib::BeginShaderMode(shader);
}

inline void EndShaderMode() {
  if (should_render())
    raylib::EndShaderMode();
}

inline void BeginScissorMode(int x, int y, int width, int height) {
  if (should_render())
    raylib::BeginScissorMode(x, y, width, height);
}

inline void EndScissorMode() {
  if (should_render())
    raylib::EndScissorMode();
}

// Background clearing
inline void ClearBackground(raylib::Color color) {
  if (should_render())
    raylib::ClearBackground(color);
}

// Texture drawing
inline void DrawTexture(raylib::Texture2D texture, int posX, int posY,
                        raylib::Color tint) {
  if (should_render())
    raylib::DrawTexture(texture, posX, posY, tint);
}

inline void DrawTextureEx(raylib::Texture2D texture, raylib::Vector2 position,
                          float rotation, float scale, raylib::Color tint) {
  if (should_render())
    raylib::DrawTextureEx(texture, position, rotation, scale, tint);
}

inline void DrawTexturePro(raylib::Texture2D texture, raylib::Rectangle source,
                           raylib::Rectangle dest, raylib::Vector2 origin,
                           float rotation, raylib::Color tint) {
  if (should_render())
    raylib::DrawTexturePro(texture, source, dest, origin, rotation, tint);
}

// Rectangle drawing
inline void DrawRectangle(int posX, int posY, int width, int height,
                          raylib::Color color) {
  if (should_render())
    raylib::DrawRectangle(posX, posY, width, height, color);
}

inline void DrawRectangleRec(raylib::Rectangle rec, raylib::Color color) {
  if (should_render())
    raylib::DrawRectangleRec(rec, color);
}

inline void DrawRectangleLinesEx(raylib::Rectangle rec, float lineThick,
                                 raylib::Color color) {
  if (should_render())
    raylib::DrawRectangleLinesEx(rec, lineThick, color);
}

inline void DrawRectangleRounded(raylib::Rectangle rec, float roundness,
                                 int segments, raylib::Color color) {
  if (should_render())
    raylib::DrawRectangleRounded(rec, roundness, segments, color);
}

// Text drawing
inline void DrawText(const char *text, int posX, int posY, int fontSize,
                     raylib::Color color) {
  if (should_render())
    raylib::DrawText(text, posX, posY, fontSize, color);
}

inline void DrawTextEx(raylib::Font font, const char *text,
                       raylib::Vector2 position, float fontSize, float spacing,
                       raylib::Color tint) {
  if (should_render())
    raylib::DrawTextEx(font, text, position, fontSize, spacing, tint);
}

inline void DrawTextWithActiveFont(const char *text, int posX, int posY,
                                   float fontSize, raylib::Color color) {
  if (!should_render())
    return;

  afterhours::Entity &font_manager_opt =
      afterhours::EntityHelper::get_singleton<afterhours::ui::FontManager>();
  if (!font_manager_opt.has<afterhours::ui::FontManager>()) {
    raylib::DrawText(text, posX, posY, static_cast<int>(fontSize), color);
    return;
  }

  afterhours::ui::FontManager &fm =
      font_manager_opt.get<afterhours::ui::FontManager>();
  std::string font_name = get_active_font_name();
  fm.set_active(font_name);
  afterhours::Font font_ah = fm.get_active_font();
  raylib::Font font = *reinterpret_cast<raylib::Font *>(&font_ah);
  raylib::DrawTextEx(
      font, text,
      raylib::Vector2{static_cast<float>(posX), static_cast<float>(posY)},
      fontSize, 1.0f, color);
}

inline float MeasureTextWithActiveFont(const char *text, float fontSize) {
  if (!should_render())
    return 0.0f;

  afterhours::Entity &font_manager_opt =
      afterhours::EntityHelper::get_singleton<afterhours::ui::FontManager>();
  if (!font_manager_opt.has<afterhours::ui::FontManager>()) {
    return static_cast<float>(
        raylib::MeasureText(text, static_cast<int>(fontSize)));
  }

  afterhours::ui::FontManager &fm =
      font_manager_opt.get<afterhours::ui::FontManager>();
  std::string font_name = get_active_font_name();
  fm.set_active(font_name);
  afterhours::Font font_ah = fm.get_active_font();
  raylib::Font font = *reinterpret_cast<raylib::Font *>(&font_ah);
  raylib::Vector2 size = raylib::MeasureTextEx(font, text, fontSize, 1.0f);
  return size.x;
}

// Triangle drawing
inline void DrawTriangleStrip(raylib::Vector2 *points, int pointCount,
                              raylib::Color color) {
  if (should_render())
    raylib::DrawTriangleStrip(points, pointCount, color);
}

inline void DrawRectanglePro(raylib::Rectangle rec, raylib::Vector2 origin,
                             float rotation, raylib::Color color) {
  if (should_render())
    raylib::DrawRectanglePro(rec, origin, rotation, color);
}

// Circle drawing
inline void DrawCircle(int centerX, int centerY, float radius,
                       raylib::Color color) {
  if (should_render())
    raylib::DrawCircle(centerX, centerY, radius, color);
}

inline void DrawCircleLines(int centerX, int centerY, float radius,
                            raylib::Color color) {
  if (should_render())
    raylib::DrawCircleLines(centerX, centerY, radius, color);
}

// Additional drawing functions used in the codebase
inline void DrawSplineSegmentLinear(raylib::Vector2 p1, raylib::Vector2 p2,
                                    float thick, raylib::Color color) {
  if (should_render())
    raylib::DrawSplineSegmentLinear(p1, p2, thick, color);
}

inline void DrawSplineLinear(const raylib::Vector2 *points, int pointCount,
                             float thick, raylib::Color color) {
  if (should_render())
    raylib::DrawSplineLinear(points, pointCount, thick, color);
}
} // namespace render_backend
