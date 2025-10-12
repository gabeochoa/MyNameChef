#pragma once

#include "game_state_manager.h"

inline bool is_on_battle_screens(const GameStateManager &gsm) {
  return gsm.active_screen == GameStateManager::Screen::Battle ||
         gsm.active_screen == GameStateManager::Screen::Results;
}


