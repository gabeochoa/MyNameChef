#pragma once

#include "../rl.h"
#include <afterhours/ah.h>
#include <string>

struct HasTooltip : afterhours::BaseComponent {
  std::string text;
  raylib::Color background_color{40, 40, 40, 220};
  raylib::Color text_color{255, 255, 255, 255};
  float font_size = 16.0f;
  float padding = 8.0f;
  float delay = 0.5f; // Delay before showing tooltip in seconds
  
  HasTooltip() = default;
  HasTooltip(const std::string& text) : text(text) {}
  HasTooltip(const std::string& text, raylib::Color bg_color, raylib::Color txt_color) 
    : text(text), background_color(bg_color), text_color(txt_color) {}
};
