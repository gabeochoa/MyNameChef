#pragma once

#include <afterhours/ah.h>
#include <vector>

struct BattleResult : afterhours::BaseComponent {
  enum struct Outcome { PlayerWin, OpponentWin, Tie } outcome = Outcome::Tie;
  struct CourseOutcome {
    int slotIndex = 0;
    enum struct Winner { Player, Opponent, Tie } winner = Winner::Tie;
    int ticks = 0;
  };
  int playerWins = 0;
  int opponentWins = 0;
  int ties = 0;
  std::vector<CourseOutcome> outcomes;
};
