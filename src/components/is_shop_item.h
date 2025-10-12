#pragma once

#include <afterhours/ah.h>

struct IsShopItem : afterhours::BaseComponent {
  int slot = -1;
  
  IsShopItem() = default;
  explicit IsShopItem(int s) : slot(s) {}
};
