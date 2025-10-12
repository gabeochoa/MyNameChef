#pragma once

#include <afterhours/ah.h>
#include <string>

struct BattleLoadRequest : afterhours::BaseComponent {
  std::string playerJsonPath = "";
  std::string opponentJsonPath = "";
  bool loaded = false;

  BattleLoadRequest() = default;
  BattleLoadRequest(const std::string &playerPath,
                    const std::string &opponentPath)
      : playerJsonPath(playerPath), opponentJsonPath(opponentPath),
        loaded(false) {}
};
