#pragma once

#include <afterhours/ah.h>

// Tag: dish is placed in the player's 7-slot inventory/menu at `slot`
struct IsInventoryItem : afterhours::BaseComponent {
  int slot = -1;
};
