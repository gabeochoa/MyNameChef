#pragma once

#include "../../game_state_manager.h"
#include "../TestInteraction.h"

struct GotoBattleTest {
  static void execute() {
    TestInteraction::set_screen(GameStateManager::Screen::Battle);
    GameStateManager::get().update_screen();
  }
};
