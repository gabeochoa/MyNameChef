#pragma once

#include "../components/course_tag.h"
#include "../components/cuisine_tag.h"
#include "../components/brand_tag.h"
#include "../components/dish_archetype_tag.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../components/synergy_counts.h"
#include "../game_state_manager.h"
#include "../query.h"
#include <afterhours/ah.h>
#include <magic_enum/magic_enum.hpp>

using namespace afterhours;

struct SynergyCountingSystem : System<> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  void once(float) override {
    auto synergy_entity = EntityHelper::get_singleton<SynergyCounts>();
    if (!synergy_entity.get().has<SynergyCounts>()) {
      return;
    }

    auto &synergy_counts = synergy_entity.get().get<SynergyCounts>();
    synergy_counts.reset();

    for (Entity &entity : EntityQuery()
                              .whereHasComponent<IsInventoryItem>()
                              .whereHasComponent<IsDish>()
                              .gen()) {
      if (entity.cleanup) {
        continue;
      }

      if (entity.has<CourseTag>()) {
        const auto &tag = entity.get<CourseTag>();
        for (auto course : magic_enum::enum_values<CourseTagType>()) {
          if (tag.has(course)) {
            synergy_counts.course_counts[course]++;
          }
        }
      }

      if (entity.has<CuisineTag>()) {
        const auto &tag = entity.get<CuisineTag>();
        for (auto cuisine : magic_enum::enum_values<CuisineTagType>()) {
          if (tag.has(cuisine)) {
            synergy_counts.cuisine_counts[cuisine]++;
          }
        }
      }

      if (entity.has<BrandTag>()) {
        const auto &tag = entity.get<BrandTag>();
        for (auto brand : magic_enum::enum_values<BrandTagType>()) {
          if (tag.has(brand)) {
            synergy_counts.brand_counts[brand]++;
          }
        }
      }

      if (entity.has<DishArchetypeTag>()) {
        const auto &tag = entity.get<DishArchetypeTag>();
        for (auto archetype : magic_enum::enum_values<DishArchetypeTagType>()) {
          if (tag.has(archetype)) {
            synergy_counts.archetype_counts[archetype]++;
          }
        }
      }
    }
  }
};

