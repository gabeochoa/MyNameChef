#pragma once

#include "../rl.h"
#include <chrono>
#include <string>
#include <vector>

#include "../game_state_manager.h"

struct TestContext {
  enum class Mode { Interactive, Headless };

  Mode currentMode = Mode::Headless;
  bool initialized = false;
  afterhours::SystemManager systems;
  afterhours::Entity *rootEntity = nullptr;

  TestContext();
  ~TestContext();

  void initialize(Mode mode);
  void shutdown();
  void pump_once(float dt);
  void run_for_seconds(float seconds);
  std::vector<std::string> list_buttons();
  bool click_button(const std::string &label);
  bool wait_for_screen(GameStateManager::Screen screen, float timeout_seconds);

private:
  void setup_singletons();
  void teardown_window();
};
