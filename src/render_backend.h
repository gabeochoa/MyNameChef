#pragma once

namespace render_backend {
extern bool is_headless_mode;

// Drawing context management
inline void BeginDrawing() {
  if (!is_headless_mode)
    raylib::BeginDrawing();
}

inline void EndDrawing() {
  if (!is_headless_mode)
    raylib::EndDrawing();
}

inline void BeginTextureMode(raylib::RenderTexture2D target) {
  if (!is_headless_mode)
    raylib::BeginTextureMode(target);
}

inline void EndTextureMode() {
  if (!is_headless_mode)
    raylib::EndTextureMode();
}

inline void BeginMode2D(raylib::Camera2D camera) {
  if (!is_headless_mode)
    raylib::BeginMode2D(camera);
}

inline void EndMode2D() {
  if (!is_headless_mode)
    raylib::EndMode2D();
}

inline void BeginShaderMode(raylib::Shader shader) {
  if (!is_headless_mode)
    raylib::BeginShaderMode(shader);
}

inline void EndShaderMode() {
  if (!is_headless_mode)
    raylib::EndShaderMode();
}

inline void BeginScissorMode(int x, int y, int width, int height) {
  if (!is_headless_mode)
    raylib::BeginScissorMode(x, y, width, height);
}

inline void EndScissorMode() {
  if (!is_headless_mode)
    raylib::EndScissorMode();
}

// Background clearing
inline void ClearBackground(raylib::Color color) {
  if (!is_headless_mode)
    raylib::ClearBackground(color);
}

// Texture drawing
inline void DrawTexture(raylib::Texture2D texture, int posX, int posY,
                        raylib::Color tint) {
  if (!is_headless_mode)
    raylib::DrawTexture(texture, posX, posY, tint);
}

inline void DrawTextureEx(raylib::Texture2D texture, raylib::Vector2 position,
                          float rotation, float scale, raylib::Color tint) {
  if (!is_headless_mode)
    raylib::DrawTextureEx(texture, position, rotation, scale, tint);
}

inline void DrawTexturePro(raylib::Texture2D texture, raylib::Rectangle source,
                           raylib::Rectangle dest, raylib::Vector2 origin,
                           float rotation, raylib::Color tint) {
  if (!is_headless_mode)
    raylib::DrawTexturePro(texture, source, dest, origin, rotation, tint);
}

// Rectangle drawing
inline void DrawRectangle(int posX, int posY, int width, int height,
                          raylib::Color color) {
  if (!is_headless_mode)
    raylib::DrawRectangle(posX, posY, width, height, color);
}

inline void DrawRectangleRec(raylib::Rectangle rec, raylib::Color color) {
  if (!is_headless_mode)
    raylib::DrawRectangleRec(rec, color);
}

inline void DrawRectangleLinesEx(raylib::Rectangle rec, float lineThick,
                                 raylib::Color color) {
  if (!is_headless_mode)
    raylib::DrawRectangleLinesEx(rec, lineThick, color);
}

inline void DrawRectangleRounded(raylib::Rectangle rec, float roundness,
                                 int segments, raylib::Color color) {
  if (!is_headless_mode)
    raylib::DrawRectangleRounded(rec, roundness, segments, color);
}

// Text drawing
inline void DrawText(const char *text, int posX, int posY, int fontSize,
                     raylib::Color color) {
  if (!is_headless_mode)
    raylib::DrawText(text, posX, posY, fontSize, color);
}

inline void DrawTextEx(raylib::Font font, const char *text,
                       raylib::Vector2 position, float fontSize, float spacing,
                       raylib::Color tint) {
  if (!is_headless_mode)
    raylib::DrawTextEx(font, text, position, fontSize, spacing, tint);
}

// Triangle drawing
inline void DrawTriangleStrip(raylib::Vector2 *points, int pointCount,
                              raylib::Color color) {
  if (!is_headless_mode)
    raylib::DrawTriangleStrip(points, pointCount, color);
}

inline void DrawRectanglePro(raylib::Rectangle rec, raylib::Vector2 origin,
                             float rotation, raylib::Color color) {
  if (!is_headless_mode)
    raylib::DrawRectanglePro(rec, origin, rotation, color);
}

// Circle drawing
inline void DrawCircle(int centerX, int centerY, float radius,
                       raylib::Color color) {
  if (!is_headless_mode)
    raylib::DrawCircle(centerX, centerY, radius, color);
}

inline void DrawCircleLines(int centerX, int centerY, float radius,
                            raylib::Color color) {
  if (!is_headless_mode)
    raylib::DrawCircleLines(centerX, centerY, radius, color);
}

// Additional drawing functions used in the codebase
inline void DrawSplineSegmentLinear(raylib::Vector2 p1, raylib::Vector2 p2,
                                    float thick, raylib::Color color) {
  if (!is_headless_mode)
    raylib::DrawSplineSegmentLinear(p1, p2, thick, color);
}

inline void DrawSplineLinear(const raylib::Vector2 *points, int pointCount,
                             float thick, raylib::Color color) {
  if (!is_headless_mode)
    raylib::DrawSplineLinear(points, pointCount, thick, color);
}
} // namespace render_backend
