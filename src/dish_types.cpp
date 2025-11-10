#include "dish_types.h"
#include "dish_builder.h"
#include <vector>

#include "components/animation_event.h"
#include "components/deferred_flavor_mods.h"
#include "components/dish_battle_state.h"
#include "components/dish_effect.h"
#include "components/is_dish.h"
#include "log.h"
#include "query.h"
#include "shop.h"
#include <afterhours/ah.h>
#include <magic_enum/magic_enum.hpp>

DishInfo DishBuilder::build() {
  if (!name_set_ || !flavor_set_ || !sprite_set_ || !tier_set_) {
    log_error("DishBuilder: Missing required fields (name, flavor, sprite, "
              "or tier)");
  }
  dish_.price = DishBuilder::kUnifiedDishPrice;
  dish_.effects = std::move(effects_);
  return dish_;
}

static DishInfo make_dish(const char *name, FlavorStats flavor,
                          SpriteLocation sprite, int tier = 2,
                          std::vector<DishEffect> effects = {}) {
  DishInfo result;
  result.name = name;
  result.price = DishBuilder::kUnifiedDishPrice;
  result.tier = tier;
  result.flavor = flavor;
  result.sprite = sprite;
  result.effects = std::move(effects);
  return result;
}

static DishInfo make_salmon(int level) {
  auto builder = dish()
                     .with_name("Salmon")
                     .with_flavor(FlavorStats{.umami = 3, .freshness = 2})
                     .with_sprite(SpriteLocation{6, 7})
                     .with_tier(1);

  switch (level) {
  case 1:
    return builder
        .register_on_serve(
            ServeEffect()
                .with_target(TargetScope::SelfAndAdjacent)
                .if_adjacent_has(FlavorStatType::Freshness)
                .add_flavor_stat(FlavorStatType::Freshness, 1)
                .with_animation(AnimationEventType::FreshnessChain))
        .build();
  case 2:
    return builder
        .register_on_serve(
            ServeEffect()
                .with_target(TargetScope::SelfAndAdjacent)
                .if_adjacent_has(FlavorStatType::Freshness)
                .add_flavor_stat(FlavorStatType::Freshness, 2)
                .with_animation(AnimationEventType::FreshnessChain))
        .register_on_course_start(
            OnCourseStartEffect().with_target(TargetScope::Self).add_body(1))
        .build();
  case 3:
  default:
    return builder
        .register_on_serve(
            ServeEffect()
                .with_target(TargetScope::SelfAndAdjacent)
                .if_adjacent_has(FlavorStatType::Freshness)
                .add_flavor_stat(FlavorStatType::Freshness, 3)
                .with_animation(AnimationEventType::FreshnessChain))
        .register_on_course_start(
            OnCourseStartEffect().with_target(TargetScope::Self).add_body(2))
        .build();
  }
}

static DishInfo make_french_fries(int level) {
  auto builder = dish()
                     .with_name("French Fries")
                     .with_flavor(FlavorStats{.satiety = 1, .richness = 1})
                     .with_sprite(SpriteLocation{1, 9})
                     .with_tier(1);

  switch (level) {
  case 1:
    return builder
        .register_on_serve(
            ServeEffect().with_target(TargetScope::FutureAllies).add_zing(1))
        .build();
  case 2:
    return builder
        .register_on_serve(
            ServeEffect().with_target(TargetScope::FutureAllies).add_zing(2))
        .register_on_serve(
            ServeEffect().with_target(TargetScope::Self).add_zing(1))
        .build();
  case 3:
  default:
    return builder
        .register_on_serve(
            ServeEffect().with_target(TargetScope::FutureAllies).add_zing(3))
        .register_on_serve(
            ServeEffect().with_target(TargetScope::Self).add_zing(2))
        .build();
  }
}

static DishInfo make_debug_dish() {
  return dish()
      .with_name("Debug Dish")
      .with_flavor(FlavorStats{.satiety = 1,
                               .sweetness = 1,
                               .spice = 1,
                               .acidity = 1,
                               .umami = 1,
                               .richness = 1,
                               .freshness = 1})
      .with_sprite(SpriteLocation{0, 0})
      .with_tier(1)
      .register_on_start_battle(
          OnStartBattleEffect().with_target(TargetScope::Self).add_zing(1))
      .register_on_start_battle(
          OnStartBattleEffect().with_target(TargetScope::AllAllies).add_body(1))
      .register_on_serve(ServeEffect()
                             .with_target(TargetScope::Self)
                             .add_flavor_stat(FlavorStatType::Satiety, 1))
      .register_on_serve(ServeEffect()
                             .with_target(TargetScope::Self)
                             .add_flavor_stat(FlavorStatType::Sweetness, 1))
      .register_on_serve(ServeEffect()
                             .with_target(TargetScope::Self)
                             .add_flavor_stat(FlavorStatType::Spice, 1))
      .register_on_serve(ServeEffect()
                             .with_target(TargetScope::Self)
                             .add_flavor_stat(FlavorStatType::Acidity, 1))
      .register_on_serve(ServeEffect()
                             .with_target(TargetScope::Self)
                             .add_flavor_stat(FlavorStatType::Umami, 1))
      .register_on_serve(ServeEffect()
                             .with_target(TargetScope::Self)
                             .add_flavor_stat(FlavorStatType::Richness, 1))
      .register_on_serve(ServeEffect()
                             .with_target(TargetScope::Self)
                             .add_flavor_stat(FlavorStatType::Freshness, 1))
      .register_on_serve(
          ServeEffect().with_target(TargetScope::Opponent).remove_zing(1))
      .register_on_serve(
          ServeEffect().with_target(TargetScope::AllAllies).add_zing(1))
      .register_on_serve(
          ServeEffect().with_target(TargetScope::AllOpponents).remove_zing(1))
      .register_on_serve(
          ServeEffect().with_target(TargetScope::DishesAfterSelf).add_zing(1))
      .register_on_serve(
          ServeEffect().with_target(TargetScope::FutureAllies).add_zing(1))
      .register_on_serve(ServeEffect()
                             .with_target(TargetScope::FutureOpponents)
                             .remove_zing(1))
      .register_on_serve(
          ServeEffect().with_target(TargetScope::Previous).add_zing(1))
      .register_on_serve(
          ServeEffect().with_target(TargetScope::Next).add_zing(1))
      .register_on_serve(
          ServeEffect().with_target(TargetScope::Self).add_body(2))
      .register_on_serve(
          ServeEffect().with_target(TargetScope::Opponent).remove_body(1))
      .register_on_serve(
          ServeEffect().with_target(TargetScope::AllAllies).add_body(1))
      .register_on_course_start(
          OnCourseStartEffect().with_target(TargetScope::Self).add_zing(1))
      .register_on_course_start(
          OnCourseStartEffect().with_target(TargetScope::AllAllies).add_body(1))
      .register_on_bite_taken(
          OnBiteEffect().with_target(TargetScope::Self).add_zing(1))
      .register_on_bite_taken(
          OnBiteEffect().with_target(TargetScope::AllAllies).add_body(1))
      .register_on_dish_finished(OnDishFinishedEffect()
                                     .with_target(TargetScope::AllAllies)
                                     .add_zing(1))
      .register_on_dish_finished(OnDishFinishedEffect()
                                     .with_target(TargetScope::AllAllies)
                                     .add_body(2))
      .register_on_course_complete(OnCourseCompleteEffect()
                                       .with_target(TargetScope::AllAllies)
                                       .add_zing(1))
      .register_on_course_complete(OnCourseCompleteEffect()
                                       .with_target(TargetScope::AllAllies)
                                       .add_body(1))
      .build();
}

static DishInfo make_potato(int level) {
  auto builder = dish()
                     .with_name("Potato")
                     .with_flavor(FlavorStats{.satiety = 1})
                     .with_sprite(SpriteLocation{13, 3})
                     .with_tier(1);

  switch (level) {
  case 1:
  case 2:
    return builder.build();
  case 3:
  default:
    return builder
        .register_on_serve(ServeEffect()
                               .with_target(TargetScope::Self)
                               .add_flavor_stat(FlavorStatType::Satiety, 1))
        .build();
  }
}

static DishInfo make_bagel(int level) {
  auto builder = dish()
                     .with_name("Bagel")
                     .with_flavor(FlavorStats{.satiety = 1})
                     .with_sprite(SpriteLocation{13, 0})
                     .with_tier(1);

  switch (level) {
  case 1:
    return builder
        .register_on_serve(ServeEffect()
                               .with_target(TargetScope::DishesAfterSelf)
                               .add_flavor_stat(FlavorStatType::Richness, 1))
        .build();
  case 2:
    return builder
        .register_on_serve(ServeEffect()
                               .with_target(TargetScope::DishesAfterSelf)
                               .add_flavor_stat(FlavorStatType::Richness, 2))
        .register_on_serve(ServeEffect()
                               .with_target(TargetScope::Self)
                               .add_flavor_stat(FlavorStatType::Richness, 1))
        .build();
  case 3:
  default:
    return builder
        .register_on_serve(ServeEffect()
                               .with_target(TargetScope::DishesAfterSelf)
                               .add_flavor_stat(FlavorStatType::Richness, 3))
        .register_on_serve(ServeEffect()
                               .with_target(TargetScope::Self)
                               .add_flavor_stat(FlavorStatType::Richness, 2))
        .build();
  }
}

static DishInfo make_baguette(int level) {
  auto builder = dish()
                     .with_name("Baguette")
                     .with_flavor(FlavorStats{.satiety = 1})
                     .with_sprite(SpriteLocation{1, 0})
                     .with_tier(1);

  switch (level) {
  case 1:
    return builder
        .register_on_serve(
            ServeEffect().with_target(TargetScope::Opponent).remove_zing(1))
        .build();
  case 2:
    return builder
        .register_on_serve(
            ServeEffect().with_target(TargetScope::Opponent).remove_zing(2))
        .register_on_course_start(
            OnCourseStartEffect().with_target(TargetScope::Self).add_body(1))
        .build();
  case 3:
  default:
    return builder
        .register_on_serve(
            ServeEffect().with_target(TargetScope::Opponent).remove_zing(3))
        .register_on_course_start(
            OnCourseStartEffect().with_target(TargetScope::Self).add_body(2))
        .build();
  }
}

static DishInfo make_garlic_bread(int level) {
  auto builder = dish()
                     .with_name("Garlic Bread")
                     .with_flavor(FlavorStats{.satiety = 1, .richness = 1})
                     .with_sprite(SpriteLocation{0, 9})
                     .with_tier(1);

  switch (level) {
  case 1:
    return builder
        .register_on_serve(ServeEffect()
                               .with_target(TargetScope::FutureAllies)
                               .add_flavor_stat(FlavorStatType::Spice, 1))
        .build();
  case 2:
    return builder
        .register_on_serve(ServeEffect()
                               .with_target(TargetScope::FutureAllies)
                               .add_flavor_stat(FlavorStatType::Spice, 2))
        .register_on_serve(ServeEffect()
                               .with_target(TargetScope::Self)
                               .add_flavor_stat(FlavorStatType::Spice, 1))
        .build();
  case 3:
  default:
    return builder
        .register_on_serve(ServeEffect()
                               .with_target(TargetScope::FutureAllies)
                               .add_flavor_stat(FlavorStatType::Spice, 3))
        .register_on_serve(ServeEffect()
                               .with_target(TargetScope::Self)
                               .add_flavor_stat(FlavorStatType::Spice, 2))
        .build();
  }
}

static DishInfo make_fried_egg(int level) {
  auto builder = dish()
                     .with_name("Fried Egg")
                     .with_flavor(FlavorStats{.richness = 1})
                     .with_sprite(SpriteLocation{2, 7})
                     .with_tier(1);

  switch (level) {
  case 1:
    return builder
        .register_on_dish_finished(OnDishFinishedEffect()
                                       .with_target(TargetScope::AllAllies)
                                       .add_body(2))
        .build();
  case 2:
    return builder
        .register_on_dish_finished(OnDishFinishedEffect()
                                       .with_target(TargetScope::AllAllies)
                                       .add_body(4))
        .register_on_serve(
            ServeEffect().with_target(TargetScope::Self).add_body(1))
        .build();
  case 3:
  default:
    return builder
        .register_on_dish_finished(OnDishFinishedEffect()
                                       .with_target(TargetScope::AllAllies)
                                       .add_body(6))
        .register_on_serve(
            ServeEffect().with_target(TargetScope::Self).add_body(2))
        .build();
  }
}

DishInfo get_dish_info(DishType type, int level) {
  switch (type) {
  case DishType::Potato:
    return make_potato(level);
  case DishType::Salmon:
    return make_salmon(level);
  case DishType::Bagel:
    return make_bagel(level);
  case DishType::Baguette:
    return make_baguette(level);
  case DishType::GarlicBread:
    return make_garlic_bread(level);
  case DishType::FriedEgg:
    return make_fried_egg(level);
  case DishType::FrenchFries:
    return make_french_fries(level);

  // Tier 2: Simple 2-ingredient items
  case DishType::Pancakes:
    return dish()
        .with_name("Pancakes")
        .with_flavor(FlavorStats{.sweetness = 1, .satiety = 1})
        .with_sprite(SpriteLocation{33, 0})
        .with_tier(2)
        .build();
  case DishType::Sandwich:
    return dish()
        .with_name("Sandwich")
        .with_flavor(FlavorStats{.satiety = 2})
        .with_sprite(SpriteLocation{40, 0})
        .with_tier(2)
        .build();
  case DishType::Nacho:
    return dish()
        .with_name("Nacho")
        .with_flavor(FlavorStats{.satiety = 1, .richness = 1})
        .with_sprite(SpriteLocation{29, 0})
        .with_tier(2)
        .build();

  // Tier 3: Moderate complexity
  case DishType::Burger:
    return dish()
        .with_name("Burger")
        .with_flavor(FlavorStats{.satiety = 2, .richness = 1})
        .with_sprite(SpriteLocation{8, 0})
        .with_tier(3)
        .build();
  case DishType::Taco:
    return dish()
        .with_name("Taco")
        .with_flavor(FlavorStats{.satiety = 2, .spice = 1})
        .with_sprite(SpriteLocation{47, 0})
        .with_tier(3)
        .build();
  case DishType::Omlet:
    return dish()
        .with_name("Omlet")
        .with_flavor(FlavorStats{.satiety = 1, .richness = 1})
        .with_sprite(SpriteLocation{31, 0})
        .with_tier(3)
        .build();
  case DishType::IceCream:
    return dish()
        .with_name("Ice Cream")
        .with_flavor(FlavorStats{.sweetness = 3})
        .with_sprite(SpriteLocation{18, 1})
        .with_tier(3)
        .build();

  // Tier 4: Complex dishes
  case DishType::Pizza:
    return dish()
        .with_name("Pizza")
        .with_flavor(FlavorStats{.satiety = 2, .umami = 1})
        .with_sprite(SpriteLocation{35, 0})
        .with_tier(4)
        .build();
  case DishType::Ramen:
    return dish()
        .with_name("Ramen")
        .with_flavor(FlavorStats{.satiety = 2, .umami = 2})
        .with_sprite(SpriteLocation{39, 0})
        .with_tier(4)
        .build();
  case DishType::MacNCheese:
    return dish()
        .with_name("Mac 'n' Cheese")
        .with_flavor(FlavorStats{.satiety = 2, .richness = 2})
        .with_sprite(SpriteLocation{25, 0})
        .with_tier(4)
        .build();
  case DishType::Dumplings:
    return dish()
        .with_name("Dumplings")
        .with_flavor(FlavorStats{.satiety = 2, .umami = 1})
        .with_sprite(SpriteLocation{17, 0})
        .with_tier(4)
        .build();
  case DishType::Burrito:
    return dish()
        .with_name("Burrito")
        .with_flavor(FlavorStats{.satiety = 2, .umami = 1})
        .with_sprite(SpriteLocation{11, 0})
        .with_tier(4)
        .build();
  case DishType::Spaghetti:
    return dish()
        .with_name("Spaghetti")
        .with_flavor(FlavorStats{.satiety = 2, .umami = 1})
        .with_sprite(SpriteLocation{42, 0})
        .with_tier(4)
        .build();
  case DishType::RoastedChicken:
    return dish()
        .with_name("Roasted Chicken")
        .with_flavor(FlavorStats{.satiety = 2, .umami = 2})
        .with_sprite(SpriteLocation{37, 0})
        .with_tier(4)
        .build();

  // Tier 5: Most complex/Gourmet
  case DishType::Steak:
    return dish()
        .with_name("Steak")
        .with_flavor(FlavorStats{.satiety = 3, .umami = 2})
        .with_sprite(SpriteLocation{43, 0})
        .with_tier(5)
        .build();
  case DishType::Sushi:
    return dish()
        .with_name("Sushi")
        .with_flavor(FlavorStats{.freshness = 2, .umami = 2})
        .with_sprite(SpriteLocation{45, 0})
        .with_tier(5)
        .build();
  case DishType::Cheesecake:
    return dish()
        .with_name("Cheesecake")
        .with_flavor(FlavorStats{.sweetness = 3, .richness = 2})
        .with_sprite(SpriteLocation{2, 1})
        .with_tier(5)
        .build();
  case DishType::LemonPie:
    return dish()
        .with_name("Lemon Pie")
        .with_flavor(FlavorStats{.sweetness = 2, .acidity = 1})
        .with_sprite(SpriteLocation{22, 1})
        .with_tier(5)
        .build();
  case DishType::Donut:
    return dish()
        .with_name("Donut")
        .with_flavor(FlavorStats{.sweetness = 2, .richness = 1})
        .with_sprite(SpriteLocation{10, 1})
        .with_tier(5)
        .build();
  case DishType::Meatball:
    return dish()
        .with_name("Meatball")
        .with_flavor(FlavorStats{.umami = 2, .satiety = 1})
        .with_sprite(SpriteLocation{27, 0})
        .with_tier(5)
        .build();
  case DishType::DebugDish:
    return make_debug_dish();
  }
  return DishInfo{};
}

const std::vector<DishType> &get_default_dish_pool() {
  static const std::vector<DishType> pool = []() {
    auto all_dishes = magic_enum::enum_values<DishType>();
    std::vector<DishType> result;
    result.reserve(all_dishes.size());

    // Filter out placeholder/debug dishes
    for (const auto &dish : all_dishes) {
      if (dish != DishType::DebugDish) {
        result.push_back(dish);
      }
    }

    return result;
  }();
  return pool;
}

const std::vector<DishType> &get_dishes_for_tier(int max_tier) {
  static std::vector<DishType> tier_pool;
  tier_pool.clear();

  const auto &all_dishes = get_default_dish_pool();
  for (const auto &dish : all_dishes) {
    auto info = get_dish_info(dish);
    if (info.tier <= max_tier) {
      tier_pool.push_back(dish);
    }
  }

  return tier_pool;
}