#pragma once

#include "../rl.h"
#include <optional>

namespace test_input {

// Test-only input simulation state
struct SimulatedInputState {
  std::optional<vec2> mouse_position;
  bool mouse_button_left_held = false; // True while button is held down
  bool mouse_button_left_pressed_this_frame = false; // One-shot: true on frame of press
  bool mouse_button_left_pressed_consumed = false; // Track if press was actually used
  bool mouse_button_left_released_this_frame = false; // One-shot: true on frame of release
  bool input_simulation_active = false;
};

// Get the global simulated input state (test-only)
SimulatedInputState &get_simulated_state();

// Test helper functions to simulate input
void set_mouse_position(vec2 pos);
void simulate_mouse_button_press(raylib::MouseButton button);
void simulate_mouse_button_release(raylib::MouseButton button);
void clear_simulated_input();
void mark_press_consumed(); // Mark that the press flag was actually used

// Wrapper functions that check simulated state first, then fall back to real input
vec2 get_mouse_position();
bool is_mouse_button_released(raylib::MouseButton button);
bool is_mouse_button_pressed(raylib::MouseButton button);
bool is_mouse_button_down(raylib::MouseButton button);
bool is_mouse_button_up(raylib::MouseButton button);

// Check if input simulation is currently active
bool is_simulation_active();

} // namespace test_input

