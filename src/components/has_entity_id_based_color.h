#pragma once

#include <afterhours/ah.h>

struct HasEntityIDBasedColor : HasColor {
  int id = -1;
  raylib::Color default_;

  HasEntityIDBasedColor() : HasColor(raylib::WHITE) {}
  HasEntityIDBasedColor(int entity_id, raylib::Color default_color)
      : HasColor(default_color), id(entity_id), default_(default_color) {
    set(default_color);
  }
};
