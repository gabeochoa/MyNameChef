#include "navigation.h"

#include "../game.h"
#include "../query.h"
#include <afterhours/ah.h>

static inline MenuNavigationStack &nav() {
  return *afterhours::EntityHelper::get_singleton_cmp<MenuNavigationStack>();
}

namespace navigation {

void to(Screen screen) {
  auto &gsm = GameStateManager::get();
  if (gsm.active_screen != screen) {
    nav().stack.push_back(gsm.active_screen);
  }
  gsm.set_next_screen(screen);
}

void back() {
  auto &gsm = GameStateManager::get();

  // Exit game if on main screen, stack is empty, or game is paused
  if (gsm.active_screen == GameStateManager::Screen::Main ||
      nav().stack.empty() || gsm.is_paused()) {
    running = false;
    return;
  }

  // Navigate to previous screen
  Screen previous = nav().stack.back();
  nav().stack.pop_back();
  gsm.set_next_screen(previous);
}

} // namespace navigation

static void update_ui_visibility(MenuNavigationStack &nav_stack) {
  auto &gsm = GameStateManager::get();

  if (gsm.is_game_active()) {
    nav_stack.ui_visible = false;
  } else if (gsm.is_menu_active()) {
    nav_stack.ui_visible = true;
  }
}

static void handle_escape_key(MenuNavigationStack &nav_stack) {
  auto &gsm = GameStateManager::get();

  if (!nav_stack.ui_visible) {
    // Game is active and UI is hidden - pause the game and show UI
    gsm.pause_game();
    navigation::to(GameStateManager::Screen::Main);
    nav_stack.ui_visible = true;
    return;
  }

  // UI is visible - handle menu navigation or unpause
  if (gsm.is_paused()) {
    if (gsm.active_screen == GameStateManager::Screen::Main) {
      // Paused on main menu - allow exit
      navigation::back();
    } else {
      // Paused on other screen - unpause
      gsm.unpause_game();
      nav_stack.ui_visible = false;
    }
  } else {
    // Regular menu navigation
    navigation::back();
  }
}

void NavigationSystem::once(float) {
  inpc = input::get_input_collector();

  // Initialize stack with Main on first run if needed
  auto &n = nav();
  if (n.stack.empty()) {
    if (GameStateManager::get().active_screen !=
        GameStateManager::Screen::Main) {
      n.stack.push_back(GameStateManager::Screen::Main);
    }
  }

  // Update UI visibility based on game state
  update_ui_visibility(n);

  // Toggle UI visibility with WidgetMod (start button)
  const bool start_pressed =
      std::ranges::any_of(inpc.inputs_pressed(), [](const auto &a) {
        return action_matches(a.action, InputAction::WidgetMod);
      });
  if (!n.ui_visible && start_pressed) {
    navigation::to(GameStateManager::Screen::Main);
    n.ui_visible = true;
  } else if (n.ui_visible && start_pressed) {
    n.ui_visible = false;
  }

  // Handle escape key for both menu navigation and pause functionality
  const bool escape_pressed =
      std::ranges::any_of(inpc.inputs_pressed(), [](const auto &a) {
        return action_matches(a.action, InputAction::MenuBack) ||
               action_matches(a.action, InputAction::PauseButton);
      });

  if (escape_pressed) {
    handle_escape_key(n);
  }
}
