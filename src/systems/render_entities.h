#pragma once

#include "../components/has_shader.h"
#include "../components/transform.h"
#include "../query.h"
#include "../shader_library.h"
#include "../shader_types.h"
#include <afterhours/ah.h>

struct RenderEntities : System<Transform> {

  virtual void for_each_with(const Entity &entity, const Transform &transform,
                             float) const override {
    raylib::Color color = raylib::RAYWHITE;
    if (entity.has_child_of<HasColor>()) {
      color = entity.get_with_child<HasColor>().color();
    }

    raylib::DrawRectangle(static_cast<int>(transform.position.x),
                          static_cast<int>(transform.position.y),
                          static_cast<int>(transform.size.x),
                          static_cast<int>(transform.size.y), color);
  };
};
