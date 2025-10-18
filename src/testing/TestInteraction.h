#pragma once

#include <string>

#include "../game_state_manager.h"

struct TestInteraction {
  static bool is_action_allowed(const std::string &action_name) {
    // Allowlist-based control surface for test-only mutations
    // Expand as needed to gate risky operations
    return action_name == "start_game" || action_name == "set_screen" ||
           action_name == "end_game";
  }

  static void start_game() {
    if (!is_action_allowed("start_game"))
      return;
    GameStateManager::get().start_game();
  }

  static void end_game() {
    if (!is_action_allowed("end_game"))
      return;
    GameStateManager::get().end_game();
  }

  static void set_screen(GameStateManager::Screen screen) {
    if (!is_action_allowed("set_screen"))
      return;
    GameStateManager::get().set_next_screen(screen);
  }
};
