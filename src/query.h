
#pragma once

#include "components/dish_battle_state.h"
#include "components/is_drop_slot.h"
#include "components/transform.h"
#include "math_util.h"
#include "rl.h"
#include <afterhours/src/ecs.h>

using namespace afterhours;

struct EQ : public EntityQuery<EQ> {
  struct WhereInRange : EntityQuery::Modification {
    vec2 position;
    float range;

    // This might not always be the correct epsilon
    explicit WhereInRange(vec2 pos, float r = 0.01f)
        : position(pos), range(r) {}
    bool operator()(const afterhours::Entity &entity) const override {
      vec2 pos = entity.get<Transform>().pos();
      return distance_sq(position, pos) < (range * range);
    }
  };

  EQ &whereInRange(const vec2 &position, float range) {
    return add_mod(new WhereInRange(position, range));
  }

  EQ &orderByDist(const vec2 &position) {
    return orderByLambda(
        [position](const afterhours::Entity &a, const afterhours::Entity &b) {
          float a_dist = distance_sq(a.get<Transform>().pos(), position);
          float b_dist = distance_sq(b.get<Transform>().pos(), position);
          return a_dist < b_dist;
        });
  }

  struct WhereOverlaps : EntityQuery::Modification {
    Rectangle rect;

    explicit WhereOverlaps(Rectangle rect_) : rect(rect_) {}

    static bool overlaps(Rectangle r1, Rectangle r2) {
      // Check if the rectangles overlap on the x-axis
      const bool xOverlap = r1.x < r2.x + r2.width && r2.x < r1.x + r1.width;

      // Check if the rectangles overlap on the y-axis
      const bool yOverlap = r1.y < r2.y + r2.height && r2.y < r1.y + r1.height;

      // Return true if both x and y overlap
      return xOverlap && yOverlap;
    }

    bool operator()(const afterhours::Entity &entity) const override {
      return overlaps(rect, entity.get<Transform>().rect());
    }
  };

  EQ &whereOverlaps(const Rectangle r) { return add_mod(new WhereOverlaps(r)); }

  struct WhereSlotID : EntityQuery::Modification {
    int slot_id;

    explicit WhereSlotID(int id) : slot_id(id) {}

    bool operator()(const afterhours::Entity &entity) const override {
      return entity.has<IsDropSlot>() &&
             entity.get<IsDropSlot>().slot_id == slot_id;
    }
  };

  EQ &whereSlotID(int id) { return add_mod(new WhereSlotID(id)); }

  struct WhereInSlotIndex : EntityQuery::Modification {
    int slot_index;

    explicit WhereInSlotIndex(int index) : slot_index(index) {}

    bool operator()(const afterhours::Entity &entity) const override {
      return entity.has<DishBattleState>() &&
             entity.get<DishBattleState>().queue_index == slot_index;
    }
  };

  EQ &whereInSlotIndex(int slot_index) {
    return add_mod(new WhereInSlotIndex(slot_index));
  }

  struct WhereTeamSide : EntityQuery::Modification {
    DishBattleState::TeamSide team_side;

    explicit WhereTeamSide(DishBattleState::TeamSide side) : team_side(side) {}

    bool operator()(const afterhours::Entity &entity) const override {
      return entity.has<DishBattleState>() &&
             entity.get<DishBattleState>().team_side == team_side;
    }
  };

  EQ &whereTeamSide(DishBattleState::TeamSide side) {
    return add_mod(new WhereTeamSide(side));
  }

  template <typename T>
  afterhours::OptEntity
  gen_max(std::function<T(const afterhours::Entity &)> extractor) const {
    const auto results = gen();
    if (results.empty()) {
      return afterhours::OptEntity();
    }

    auto it = std::max_element(
        results.begin(), results.end(),
        [&extractor](const afterhours::Entity &a, const afterhours::Entity &b) {
          return extractor(a) < extractor(b);
        });
    return afterhours::OptEntity(*it);
  }

  template <typename T>
  afterhours::OptEntity
  gen_min(std::function<T(const afterhours::Entity &)> extractor) const {
    const auto results = gen();
    if (results.empty()) {
      return afterhours::OptEntity();
    }

    auto it = std::min_element(
        results.begin(), results.end(),
        [&extractor](const afterhours::Entity &a, const afterhours::Entity &b) {
          return extractor(a) < extractor(b);
        });
    return afterhours::OptEntity(*it);
  }

  template <typename T>
  std::optional<T>
  gen_max_value(std::function<T(const afterhours::Entity &)> extractor) const {
    const auto results = gen();
    if (results.empty()) {
      return std::nullopt;
    }

    T max_val = extractor(results[0].get());
    for (const auto &ref : results) {
      T val = extractor(ref.get());
      if (val > max_val) {
        max_val = val;
      }
    }
    return max_val;
  }
};
