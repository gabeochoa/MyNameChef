#pragma once

#include "../input_mapping.h"
#include <afterhours/ah.h>

struct RenderDebugGridOverlay
    : System<window_manager::ProvidesCurrentResolution> {
  input::PossibleInputCollector inpc;

  virtual void once(float) override { inpc = input::get_input_collector(); }

  virtual bool should_run(float) override {
    inpc = input::get_input_collector();
    if (!inpc.has_value())
      return false;
    bool toggle_pressed =
        std::ranges::any_of(inpc.inputs_pressed(), [](const auto &a) {
          return action_matches(a.action, InputAction::ToggleUIDebug);
        });
    static bool enabled = false;
    if (toggle_pressed)
      enabled = !enabled;
    return enabled;
  }

  virtual void for_each_with(
      const Entity &,
      const window_manager::ProvidesCurrentResolution &pCurrentResolution,
      float) const override {
    auto resolution = pCurrentResolution.current_resolution;
    const int step = 80;
    const raylib::Color line_color{255, 255, 255, 60};
    const raylib::Color text_color{255, 255, 0, 200};

    for (int x = 0; x <= resolution.width; x += step) {
      raylib::DrawLine(x, 0, x, resolution.height, line_color);
      std::string label = std::to_string(x);
      raylib::DrawText(label.c_str(), x + 4, 6, 12, text_color);
    }

    for (int y = 0; y <= resolution.height; y += step) {
      raylib::DrawLine(0, y, resolution.width, y, line_color);
      std::string label = std::to_string(y);
      raylib::DrawText(label.c_str(), 6, y + 4, 12, text_color);
    }
  }
};
