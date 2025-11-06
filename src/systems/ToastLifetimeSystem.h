#pragma once

#include "../components/toast_message.h"
#include <afterhours/ah.h>

struct ToastLifetimeSystem : afterhours::System<ToastMessage> {
  void for_each_with(afterhours::Entity &entity, ToastMessage &toast,
                     float dt) override {
    toast.lifetime -= dt;

    if (toast.lifetime <= 0.0f) {
      entity.cleanup = true;
    }
  }
};
