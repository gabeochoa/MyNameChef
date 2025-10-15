#pragma once

#include <afterhours/ah.h>
#include <afterhours/src/library.h>
#include <optional>

SINGLETON_FWD(GameStateManager)
struct GameStateManager {
  SINGLETON(GameStateManager)

  enum struct GameState {
    Menu,
    Playing,
    Paused,
  } current_state = GameState::Menu;

  enum struct Screen {
    Main,
    Settings,
    Dishes,
    Shop,
    Battle,
    Results,
  } active_screen = Screen::Main;

  std::optional<Screen> next_screen = std::nullopt;

  void start_game() {
    current_state = GameState::Playing;
    set_next_screen(Screen::Shop);
  }

  void end_game() {
    current_state = GameState::Menu;
    active_screen = Screen::Main;
  }

  void pause_game() {
    if (current_state == GameState::Playing) {
      current_state = GameState::Paused;
    }
  }

  void unpause_game() {
    if (current_state == GameState::Paused) {
      current_state = GameState::Playing;
    }
  }

  void set_screen(Screen screen) { active_screen = screen; }

  void set_next_screen(Screen screen) { next_screen = screen; }

  void to_battle() { set_next_screen(Screen::Battle); }
  void to_results() { set_next_screen(Screen::Results); }

  // Call this at the start of each frame to apply queued screen changes
  void update_screen() {
    if (next_screen.has_value()) {
      active_screen = next_screen.value();
      next_screen = {};
    }
  }

  [[nodiscard]] bool is_game_active() const {
    return current_state == GameState::Playing;
  }
  [[nodiscard]] bool is_menu_active() const {
    return current_state == GameState::Menu ||
           current_state == GameState::Paused;
  }
  [[nodiscard]] bool is_paused() const {
    return current_state == GameState::Paused;
  }

  // Centralized screen policy helpers
  [[nodiscard]] static bool should_render_world_entities(Screen s) {
    switch (s) {
    case Screen::Shop:
    case Screen::Battle:
    case Screen::Results:
    case Screen::Dishes:
      return true;
    case Screen::Main:
    case Screen::Settings:
      return false;
    }
    return false;
  }

  [[nodiscard]] static bool should_render_tooltips(Screen s) {
    switch (s) {
    case Screen::Shop:
    case Screen::Battle:
    case Screen::Dishes:
      return true;
    case Screen::Results:
    case Screen::Main:
    case Screen::Settings:
      return false;
    }
    return false;
  }

  [[nodiscard]] static int render_screen_for(Screen s) {
    switch (s) {
    case Screen::Shop:
      return 1; // RenderScreen::Shop
    case Screen::Battle:
      return 2; // RenderScreen::Battle
    case Screen::Results:
      return 4; // RenderScreen::Results
    case Screen::Dishes:
    case Screen::Main:
    case Screen::Settings:
    default:
      return 1 | 2 | 4; // RenderScreen::All
    }
  }
};
