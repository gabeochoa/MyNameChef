#pragma once

#include <afterhours/ah.h>

struct DeferredFlavorMods : afterhours::BaseComponent {
  int satiety = 0;
  int sweetness = 0;
  int spice = 0;
  int acidity = 0;
  int umami = 0;
  int richness = 0;
  int freshness = 0;
  
  DeferredFlavorMods() = default;
  
  void add_mod(int sat, int sweet, int sp, int acid, int um, int rich, int fresh) {
    satiety += sat;
    sweetness += sweet;
    spice += sp;
    acidity += acid;
    umami += um;
    richness += rich;
    freshness += fresh;
  }
  
  void clear() {
    satiety = sweetness = spice = acidity = umami = richness = freshness = 0;
  }
};
