#pragma once

#include "../components/transform.h"
#include "../query.h"
#include "../tags.h"
#include <afterhours/ah.h>

struct UpdateSpriteTransform
    : System<Transform, afterhours::texture_manager::HasSprite> {

  virtual void for_each_with(Entity &entity, Transform &transform,
                             afterhours::texture_manager::HasSprite &hasSprite,
                             float) override {
    hasSprite.update_transform(transform.position, transform.size,
                               transform.angle);

    if (entity.has_child_of<HasColor>()) {
      hasSprite.update_color(entity.get_with_child<HasColor>().color());
    }
  }
};
