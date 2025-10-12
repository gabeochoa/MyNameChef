#pragma once

#include "rl.h"
#include <afterhours/ah.h>
#include <array>
#include <optional>
#include <string>
#include <vector>

struct Wallet : afterhours::BaseComponent {
  int gold = 10;
};

struct Health : afterhours::BaseComponent {
  int current = 5;
  int max = 5;
};

struct ShopState : afterhours::BaseComponent {
  bool initialized = false;
};

void make_shop_manager(afterhours::Entity &);
void register_shop_update_systems(afterhours::SystemManager &systems);
void register_shop_render_systems(afterhours::SystemManager &systems);
