#include "test_input.h"
#include <afterhours/ah.h>
#include "../log.h"

namespace test_input {

SimulatedInputState &get_simulated_state() {
  static SimulatedInputState state;
  return state;
}

void set_mouse_position(vec2 pos) {
  auto &state = get_simulated_state();
  state.mouse_position = pos;
  state.input_simulation_active = true;
}

void simulate_mouse_button_press(raylib::MouseButton button) {
  auto &state = get_simulated_state();
  if (button == raylib::MOUSE_BUTTON_LEFT) {
    if (!state.mouse_button_left_held) {
      // Only set pressed flag if button wasn't already held
      state.mouse_button_left_pressed_this_frame = true;
      state.mouse_button_left_pressed_consumed = false; // Reset consumed flag
    }
    state.mouse_button_left_held = true;
    state.mouse_button_left_released_this_frame = false;
  }
  state.input_simulation_active = true;
}

void simulate_mouse_button_release(raylib::MouseButton button) {
  log_error("TEST_INPUT: simulate_mouse_button_release CALLED for button {}", static_cast<int>(button));
  auto &state = get_simulated_state();
  if (button == raylib::MOUSE_BUTTON_LEFT) {
    log_error("TEST_INPUT: simulate_mouse_button_release - current held state: {}", state.mouse_button_left_held);
    if (state.mouse_button_left_held) {
      // Only set released flag if button was held
      state.mouse_button_left_released_this_frame = true;
      // Log to trace when flag is set
      log_error("TEST_INPUT: simulate_mouse_button_release - setting released_this_frame=true, was_held=true");
    } else {
      log_error("TEST_INPUT: simulate_mouse_button_release - button was not held, not setting flag");
    }
    state.mouse_button_left_held = false;
    state.mouse_button_left_pressed_this_frame = false;
  }
  state.input_simulation_active = true;
}

void clear_simulated_input() {
  auto &state = get_simulated_state();
  state.mouse_position = std::nullopt;
  state.mouse_button_left_held = false;
  state.mouse_button_left_pressed_this_frame = false;
  state.mouse_button_left_pressed_consumed = false;
  state.mouse_button_left_released_this_frame = false;
  state.input_simulation_active = false;
}

void mark_press_consumed() {
  auto &state = get_simulated_state();
  state.mouse_button_left_pressed_consumed = true;
  state.mouse_button_left_pressed_this_frame = false; // Clear flag now that it's consumed
}

vec2 get_mouse_position() {
  auto &state = get_simulated_state();
  if (state.input_simulation_active && state.mouse_position.has_value()) {
    return state.mouse_position.value();
  }
  // Fall back to real input
  return afterhours::input::get_mouse_position();
}

bool is_mouse_button_released(raylib::MouseButton button) {
  auto &state = get_simulated_state();
  if (state.input_simulation_active && button == raylib::MOUSE_BUTTON_LEFT) {
    if (state.mouse_button_left_released_this_frame) {
      // One-shot: clear immediately after reading
      state.mouse_button_left_released_this_frame = false;
      log_error("TEST_INPUT: is_mouse_button_released - returning true and clearing flag");
      return true;
    } else {
      log_error("TEST_INPUT: is_mouse_button_released - flag is false, held={}", state.mouse_button_left_held);
    }
  }
  // Fall back to real input
  return afterhours::input::is_mouse_button_released(button);
}

bool is_mouse_button_pressed(raylib::MouseButton button) {
  auto &state = get_simulated_state();
  if (state.input_simulation_active && button == raylib::MOUSE_BUTTON_LEFT) {
    if (state.mouse_button_left_pressed_this_frame && !state.mouse_button_left_pressed_consumed) {
      // Flag is set and not yet consumed - return true but don't clear yet
      // The flag will be cleared when mark_press_consumed() is called
      return true;
    }
  }
  // Fall back to real input
  return afterhours::input::is_mouse_button_pressed(button);
}

bool is_mouse_button_down(raylib::MouseButton button) {
  auto &state = get_simulated_state();
  if (state.input_simulation_active && button == raylib::MOUSE_BUTTON_LEFT) {
    // Button is down if it's currently held
    return state.mouse_button_left_held;
  }
  // Fall back to real input
  return afterhours::input::is_mouse_button_down(button);
}

bool is_mouse_button_up(raylib::MouseButton button) {
  auto &state = get_simulated_state();
  if (state.input_simulation_active && button == raylib::MOUSE_BUTTON_LEFT) {
    // Button is up if it's not held
    return !state.mouse_button_left_held;
  }
  // Fall back to real input
  return afterhours::input::is_mouse_button_up(button);
}

bool is_simulation_active() {
  return get_simulated_state().input_simulation_active;
}

} // namespace test_input

