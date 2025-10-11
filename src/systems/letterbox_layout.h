#pragma once

#include <afterhours/ah.h>

struct LetterboxLayout {
  int bar_left = 0;
  int bar_right = 0;
  int bar_top = 0;
  int bar_bottom = 0;
  raylib::Rectangle dst{0.f, 0.f, 0.f, 0.f};
};

static inline LetterboxLayout
compute_letterbox_layout(const int window_width, const int window_height,
                         const int content_width, const int content_height) {
  LetterboxLayout layout;
  int dest_w = window_width;
  int dest_h = static_cast<int>(
      std::round(static_cast<double>(dest_w) * content_height / content_width));
  if (dest_h > window_height) {
    dest_h = window_height;
    dest_w = static_cast<int>(std::round(static_cast<double>(dest_h) *
                                         content_width / content_height));
  }

  const int bar_w_total = window_width - dest_w;
  const int bar_h_total = window_height - dest_h;
  layout.bar_left = bar_w_total / 2;
  layout.bar_right = bar_w_total - layout.bar_left;
  layout.bar_top = bar_h_total / 2;
  layout.bar_bottom = bar_h_total - layout.bar_top;
  layout.dst = raylib::Rectangle{
      static_cast<float>(layout.bar_left), static_cast<float>(layout.bar_top),
      static_cast<float>(dest_w), static_cast<float>(dest_h)};
  return layout;
}
