#pragma once

#include "../components/has_shader.h"
#include "../tags.h"
#include <afterhours/ah.h>

using namespace afterhours;

struct MarkEntitiesWithShaders : System<HasShader> {
  virtual void for_each_with(Entity &entity, HasShader &, float) override {
    if (!entity.hasTag(GameTag::SkipTextureRendering)) {
      entity.enableTag(GameTag::SkipTextureRendering);
      log_trace("Marked entity {} to skip texture_manager rendering",
                entity.id);
    }
  }
};
