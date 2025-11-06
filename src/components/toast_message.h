#pragma once

#include <afterhours/ah.h>
#include <string>

struct ToastMessage : afterhours::BaseComponent {
  // TODO replace with translatable string
  std::string message;
  float lifetime;
  float initialLifetime;

  ToastMessage() = default;
  ToastMessage(const std::string &msg, float dur = 3.0f) {
    const float enterDuration = 0.3f;
    const float exitDuration = 0.3f;
    message = msg;
    initialLifetime = dur + enterDuration + exitDuration;
    lifetime = initialLifetime;
  }
};
