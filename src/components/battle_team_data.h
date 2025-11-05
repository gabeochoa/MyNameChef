#pragma once

#include "../dish_types.h"
#include <afterhours/ah.h>
#include <vector>

struct TeamDishSpec {
  DishType dishType;
  int slot;
  int level;
  std::vector<int> powerups;
};

struct BattleTeamDataPlayer : afterhours::BaseComponent {
  std::vector<TeamDishSpec> team;
  bool instantiated = false;
};

struct BattleTeamDataOpponent : afterhours::BaseComponent {
  std::vector<TeamDishSpec> team;
  bool instantiated = false;
};
