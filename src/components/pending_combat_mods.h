#pragma once

#include <afterhours/ah.h>

struct PendingCombatMods : afterhours::BaseComponent {
  int deltaZing = 0;
  int deltaBody = 0;
  
  PendingCombatMods() = default;
  
  void add_mods(int zing, int body) {
    deltaZing += zing;
    deltaBody += body;
  }
  
  void clear() {
    deltaZing = deltaBody = 0;
  }
};
