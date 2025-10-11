#pragma once

#include "../components.h"
#include "../query.h"
#include <afterhours/ah.h>

struct ProcessCollisionAbsorption : System<Transform, CollisionAbsorber> {

  virtual void for_each_with(Entity &entity, Transform &transform,
                             CollisionAbsorber &collision_absorber,
                             float) override {
    // We let the absorbed things (e.g. bullets) manage cleaning themselves
    // up, rather than the other way around.
    if (collision_absorber.absorber_type ==
        CollisionAbsorber::AbsorberType::Absorber) {
      return;
    }

    const auto unrelated_absorber =
        [&collision_absorber](const Entity &collider) {
          const auto &other_absorber = collider.get<CollisionAbsorber>();
          const auto are_related = collision_absorber.parent_id.value_or(-1) ==
                                   other_absorber.parent_id.value_or(-2);

          if (are_related) {
            return false;
          }

          return other_absorber.absorber_type ==
                 CollisionAbsorber::AbsorberType::Absorber;
        };

    auto collided_with_absorber =
        EQ().whereHasComponent<CollisionAbsorber>()
            .whereNotID(entity.id)
            .whereNotID(collision_absorber.parent_id.value_or(-1))
            .whereOverlaps(transform.rect())
            .whereLambda(unrelated_absorber)
            .gen();

    if (!collided_with_absorber.empty()) {
      entity.cleanup = true;
    }
  }
};
