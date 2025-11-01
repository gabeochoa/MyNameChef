#pragma once

#include <afterhours/ah.h>

struct DishBattleState : afterhours::BaseComponent {
  enum struct TeamSide { Player, Opponent } team_side = TeamSide::Player;
  int queue_index = 0; // left-to-right order
  enum struct Phase {
    InQueue,
    Entering,
    InCombat,
    Finished
  } phase = Phase::InQueue;
  float enter_progress = 0.0f;     // slide-in animation 0..1
  float bite_timer = 0.0f;         // pacing timer during combat
  bool players_turn = true;        // maintained on player-side entity
  bool first_bite_decided = false; // set once per pairing
  bool onserve_fired = false;      // track if OnServe trigger has been fired

  // Bite cadence substate for head-to-head pacing
  enum struct BiteCadence { PrePause, PostPause } bite_cadence = BiteCadence::PrePause;
  float bite_cadence_timer = 0.0f;
};
