#pragma once

#include <afterhours/ah.h>
#include <string>
#include <vector>

struct BattleResult : afterhours::BaseComponent {
  enum struct Outcome { PlayerWin, OpponentWin, Tie } outcome = Outcome::Tie;

  struct JudgeScore {
    int playerScore = 0;
    int opponentScore = 0;
    std::string judgeName = "Judge";
  };

  std::vector<JudgeScore> judgeScores;
  int totalPlayerScore = 0;
  int totalOpponentScore = 0;

  BattleResult() = default;
};
