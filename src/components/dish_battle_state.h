#pragma once

#include <afterhours/ah.h>

struct DishBattleState : afterhours::BaseComponent {
  enum struct TeamSide { Player, Opponent } team_side = TeamSide::Player;
  int queue_index = 0; // left-to-right order
  enum struct Phase { InQueue, Presenting, Judged } phase = Phase::InQueue;
  float phase_progress = 0.0f; // 0..1 for animation toward judges
};
