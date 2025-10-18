#pragma once

#include <string>

#include <afterhours/ah.h>

#include "../game_state_manager.h"
#include "../testing/TestInteraction.h"

struct TestSystem : afterhours::System<> {
  std::string test_name;
  bool initialized = false;

  explicit TestSystem(std::string name) : test_name(std::move(name)) {}

  virtual bool should_run(float) override { return true; }

  virtual void once(float) override {
    if (initialized)
      return;

    initialized = true;

    if (test_name == "play_navigates_to_shop") {
      TestInteraction::start_game();
      GameStateManager::get().update_screen();
    } else if (test_name == "goto_battle") {
      TestInteraction::set_screen(GameStateManager::Screen::Battle);
      GameStateManager::get().update_screen();
    }
  }

  virtual void for_each(afterhours::Entity &, float) override {}

  virtual void after(float) override {
    GameStateManager::get().update_screen();
  }
};
