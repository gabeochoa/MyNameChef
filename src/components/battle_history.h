#pragma once

#include <afterhours/ah.h>
#include <chrono>
#include <string>
#include <vector>

struct BattleHistoryEntry {
  std::string filePath;
  std::string filename;
  uint64_t seed = 0;
  std::string opponentId;
  std::chrono::system_clock::time_point timestamp;
  int playerWins = 0;
  int opponentWins = 0;
  int ties = 0;
};

struct BattleHistory : afterhours::BaseComponent {
  std::vector<BattleHistoryEntry> entries;
  bool loaded = false;
};

