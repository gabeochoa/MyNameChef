#pragma once

#include "LetterboxLayout.h"
#include <afterhours/ah.h>

struct RenderLetterboxBars : System<window_manager::ProvidesCurrentResolution> {
  virtual ~RenderLetterboxBars() {}
  virtual void for_each_with(const Entity &,
                             const window_manager::ProvidesCurrentResolution &,
                             float) const override {
    const int window_w = raylib::GetScreenWidth();
    const int window_h = raylib::GetScreenHeight();
    const int content_w = mainRT.texture.width;
    const int content_h = mainRT.texture.height;
    const LetterboxLayout layout =
        compute_letterbox_layout(window_w, window_h, content_w, content_h);

    if (layout.bar_left > 0) {
      raylib::DrawRectangle(0, 0, layout.bar_left, window_h, raylib::BLACK);
      raylib::DrawRectangle(window_w - layout.bar_right, 0, layout.bar_right,
                            window_h, raylib::BLACK);
    }
    if (layout.bar_top > 0) {
      raylib::DrawRectangle(0, 0, window_w, layout.bar_top, raylib::BLACK);
      raylib::DrawRectangle(0, window_h - layout.bar_bottom, window_w,
                            layout.bar_bottom, raylib::BLACK);
    }
  }
};
