#pragma once

#include "../components/toast_message.h"
#include "../components/transform.h"
#include "../render_backend.h"
#include "../rl.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/color.h>

struct ToastAnimationSystem
    : afterhours::System<ToastMessage, Transform, HasColor> {
  virtual bool should_run(float) override {
    return !render_backend::is_headless_mode;
  }

  void for_each_with(afterhours::Entity &, ToastMessage &toast,
                     Transform &transform, HasColor &hasColor, float) override {
    const float enterDuration = 0.3f;
    const float exitDuration = 0.3f;
    const float elapsed = toast.initialLifetime - toast.lifetime;

    float screenHeight = static_cast<float>(raylib::GetScreenHeight());
    float startY = screenHeight + 50.0f;
    float targetY = screenHeight - 100.0f;

    float alpha = 1.0f;
    float y = targetY;

    if (elapsed < enterDuration) {
      float progress = elapsed / enterDuration;
      y = startY + (targetY - startY) * progress;
      alpha = progress;
    } else if (toast.lifetime > exitDuration) {
      y = targetY;
      alpha = 1.0f;
    } else {
      float exitProgress = toast.lifetime / exitDuration;
      y = targetY;
      alpha = exitProgress;
    }

    transform.position.y = y;
    raylib::Color bgColor = hasColor.color();
    bgColor.a = static_cast<unsigned char>(alpha * 200.0f);
    hasColor.set(bgColor);
  }
};
