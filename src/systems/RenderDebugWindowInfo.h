#pragma once

#include "../game_state_manager.h"
#include "../input_mapping.h"
#include <afterhours/ah.h>
#include <magic_enum/magic_enum.hpp>

struct RenderDebugWindowInfo
    : System<window_manager::ProvidesCurrentResolution> {
  mutable input::PossibleInputCollector inpc;

  virtual void once(float) const override {
    inpc = input::get_input_collector();
  }

  virtual bool should_run(float) override {
    return true; // @nocommit just for  testing ui scale
    inpc = input::get_input_collector();
    if (!inpc.has_value())
      return false;
    bool toggle_pressed =
        std::ranges::any_of(inpc.inputs_pressed(), [](const auto &a) {
          return action_matches(a.action, InputAction::ToggleUILayoutDebug);
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
    const int window_w = raylib::GetScreenWidth();
    const int window_h = raylib::GetScreenHeight();
    const auto rez = pCurrentResolution.current_resolution;
    const auto &gsm = GameStateManager::get();

    const int x = pCurrentResolution.width() - 200; // Increased width for more text
    const int y0 = 18;
    const int y1 = 36;
    const int y2 = 54;
    const int y3 = 72;
    const int font = 14;
    const raylib::Color col = raylib::WHITE;

    raylib::DrawText(fmt::format("win {}x{}", window_w, window_h).c_str(), x,
                     y0, font, col);
    raylib::DrawText(fmt::format("game {}x{}", rez.width, rez.height).c_str(),
                     x, y1, font, col);
    
    // Add game state and screen information
    std::string state_name = std::string(magic_enum::enum_name(gsm.current_state));
    std::string screen_name = std::string(magic_enum::enum_name(gsm.active_screen));
    raylib::DrawText(fmt::format("state: {}", state_name).c_str(), x, y2, font, col);
    raylib::DrawText(fmt::format("screen: {}", screen_name).c_str(), x, y3, font, col);
  }
};
