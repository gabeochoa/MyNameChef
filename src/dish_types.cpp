#include "dish_types.h"
#include <vector>

#include "components/deferred_flavor_mods.h"
#include "components/dish_battle_state.h"
#include "components/dish_effect.h"
#include "components/is_dish.h"
#include "log.h"
#include "query.h"
#include "shop.h"
#include <afterhours/ah.h>

// All dishes now have the same price
static constexpr int kUnifiedDishPrice = 3;

static DishInfo make_dish(const char *name, FlavorStats flavor,
                          SpriteLocation sprite, int tier = 2,
                          OnServeHandler onServe = nullptr,
                          std::vector<DishEffect> effects = {}) {
  DishInfo result;
  result.name = name;
  result.price = kUnifiedDishPrice;
  result.tier = tier;
  result.flavor = flavor;
  result.sprite = sprite;
  result.onServe = onServe;
  result.effects = std::move(effects);
  return result;
}

static DishInfo make_salmon() {
  return make_dish(
      "Salmon", FlavorStats{.umami = 3, .freshness = 2}, SpriteLocation{6, 7},
      1,
      /*onServe=*/[](int sourceEntityId) {
        auto src_opt = EQ().whereID(sourceEntityId).gen_first();
        if (!src_opt || !src_opt->has<DishBattleState>()) {
          return;
        }

        auto &src_dbs = src_opt->get<DishBattleState>();
        int src_queue_index = src_dbs.queue_index;

        bool has_freshness_adjacent = false;

        auto prevDish =
            EQ().whereHasComponent<IsDish>()
                .whereHasComponent<DishBattleState>()
                .whereLambda(
                    [&src_dbs, src_queue_index](const afterhours::Entity &e) {
                      const DishBattleState &dbs = e.get<DishBattleState>();
                      return dbs.team_side == src_dbs.team_side &&
                             dbs.queue_index == src_queue_index - 1;
                    })
                .gen_first();
        if (prevDish.has_value()) {
          auto &dish = prevDish.value()->get<IsDish>();
          auto dish_info = get_dish_info(dish.type);
          if (dish_info.flavor.freshness > 0) {
            has_freshness_adjacent = true;
          }
        }

        if (!has_freshness_adjacent) {
          auto nextDish =
              EQ().whereHasComponent<IsDish>()
                  .whereHasComponent<DishBattleState>()
                  .whereLambda(
                      [&src_dbs, src_queue_index](const afterhours::Entity &e) {
                        const DishBattleState &dbs = e.get<DishBattleState>();
                        return dbs.team_side == src_dbs.team_side &&
                               dbs.queue_index == src_queue_index + 1;
                      })
                  .gen_first();
          if (nextDish.has_value()) {
            auto &dish = nextDish.value()->get<IsDish>();
            auto dish_info = get_dish_info(dish.type);
            if (dish_info.flavor.freshness > 0) {
              has_freshness_adjacent = true;
            }
          }
        }

        if (has_freshness_adjacent) {
          int previousEntityId = -1;
          int nextEntityId = -1;

          auto &src_deferred =
              src_opt->addComponentIfMissing<DeferredFlavorMods>();
          src_deferred.freshness += 1;

          afterhours::OptEntity prevDishBoost =
              EQ().whereHasComponent<IsDish>()
                  .whereHasComponent<DishBattleState>()
                  .whereLambda(
                      [&src_dbs, src_queue_index](const afterhours::Entity &e) {
                        const DishBattleState &dbs = e.get<DishBattleState>();
                        return dbs.team_side == src_dbs.team_side &&
                               dbs.queue_index == src_queue_index - 1;
                      })
                  .gen_first();
          if (prevDishBoost.has_value()) {
            auto &deferred = prevDishBoost.value()
                                 ->addComponentIfMissing<DeferredFlavorMods>();
            deferred.freshness += 1;
            previousEntityId = prevDishBoost.value()->id;
          }

          auto nextDish =
              EQ().whereHasComponent<IsDish>()
                  .whereHasComponent<DishBattleState>()
                  .whereLambda(
                      [&src_dbs, src_queue_index](const afterhours::Entity &e) {
                        const DishBattleState &dbs = e.get<DishBattleState>();
                        return dbs.team_side == src_dbs.team_side &&
                               dbs.queue_index == src_queue_index + 1;
                      })
                  .gen_first();
          if (nextDish.has_value()) {
            auto &deferred =
                nextDish.value()->addComponentIfMissing<DeferredFlavorMods>();
            deferred.freshness += 1;
            nextEntityId = nextDish.value()->id;
          }

          make_freshness_chain_animation(sourceEntityId, previousEntityId,
                                         nextEntityId);
        }
      });
}

static DishInfo make_french_fries() {
  std::vector<DishEffect> effects;
  DishEffect friesEffect;
  friesEffect.triggerHook = TriggerHook::OnServe;
  friesEffect.operation = EffectOperation::AddCombatZing;
  friesEffect.targetScope = TargetScope::FutureAllies;
  friesEffect.amount = 1;
  effects.push_back(friesEffect);

  return make_dish("French Fries", FlavorStats{.satiety = 1, .richness = 1},
                   SpriteLocation{1, 9}, 1, nullptr, effects);
}

static DishInfo make_debug_dish() {
  std::vector<DishEffect> effects;

  auto add_effect = [&effects](TriggerHook hook, EffectOperation op,
                               TargetScope scope, int amount,
                               FlavorStatType statType =
                                   FlavorStatType::Satiety) {
    DishEffect effect;
    effect.triggerHook = hook;
    effect.operation = op;
    effect.targetScope = scope;
    effect.amount = amount;
    if (op == EffectOperation::AddFlavorStat) {
      effect.flavorStatType = statType;
    }
    effects.push_back(effect);
  };

  add_effect(TriggerHook::OnStartBattle, EffectOperation::AddCombatZing,
             TargetScope::Self, 1);
  add_effect(TriggerHook::OnStartBattle, EffectOperation::AddCombatBody,
             TargetScope::AllAllies, 1);

  add_effect(TriggerHook::OnServe, EffectOperation::AddFlavorStat,
             TargetScope::Self, 1, FlavorStatType::Satiety);
  add_effect(TriggerHook::OnServe, EffectOperation::AddFlavorStat,
             TargetScope::Self, 1, FlavorStatType::Sweetness);
  add_effect(TriggerHook::OnServe, EffectOperation::AddFlavorStat,
             TargetScope::Self, 1, FlavorStatType::Spice);
  add_effect(TriggerHook::OnServe, EffectOperation::AddFlavorStat,
             TargetScope::Self, 1, FlavorStatType::Acidity);
  add_effect(TriggerHook::OnServe, EffectOperation::AddFlavorStat,
             TargetScope::Self, 1, FlavorStatType::Umami);
  add_effect(TriggerHook::OnServe, EffectOperation::AddFlavorStat,
             TargetScope::Self, 1, FlavorStatType::Richness);
  add_effect(TriggerHook::OnServe, EffectOperation::AddFlavorStat,
             TargetScope::Self, 1, FlavorStatType::Freshness);
  add_effect(TriggerHook::OnServe, EffectOperation::AddCombatZing,
             TargetScope::Opponent, -1);
  add_effect(TriggerHook::OnServe, EffectOperation::AddCombatZing,
             TargetScope::AllAllies, 1);
  add_effect(TriggerHook::OnServe, EffectOperation::AddCombatZing,
             TargetScope::AllOpponents, -1);
  add_effect(TriggerHook::OnServe, EffectOperation::AddCombatZing,
             TargetScope::DishesAfterSelf, 1);
  add_effect(TriggerHook::OnServe, EffectOperation::AddCombatZing,
             TargetScope::FutureAllies, 1);
  add_effect(TriggerHook::OnServe, EffectOperation::AddCombatZing,
             TargetScope::FutureOpponents, -1);
  add_effect(TriggerHook::OnServe, EffectOperation::AddCombatZing,
             TargetScope::Previous, 1);
  add_effect(TriggerHook::OnServe, EffectOperation::AddCombatZing,
             TargetScope::Next, 1);
  add_effect(TriggerHook::OnServe, EffectOperation::AddCombatBody,
             TargetScope::Self, 2);
  add_effect(TriggerHook::OnServe, EffectOperation::AddCombatBody,
             TargetScope::Opponent, -1);
  add_effect(TriggerHook::OnServe, EffectOperation::AddCombatBody,
             TargetScope::AllAllies, 1);

  add_effect(TriggerHook::OnCourseStart, EffectOperation::AddCombatZing,
             TargetScope::Self, 1);
  add_effect(TriggerHook::OnCourseStart, EffectOperation::AddCombatBody,
             TargetScope::AllAllies, 1);

  add_effect(TriggerHook::OnBiteTaken, EffectOperation::AddCombatZing,
             TargetScope::Self, 1);
  add_effect(TriggerHook::OnBiteTaken, EffectOperation::AddCombatBody,
             TargetScope::AllAllies, 1);

  add_effect(TriggerHook::OnDishFinished, EffectOperation::AddCombatZing,
             TargetScope::AllAllies, 1);
  add_effect(TriggerHook::OnDishFinished, EffectOperation::AddCombatBody,
             TargetScope::AllAllies, 2);

  add_effect(TriggerHook::OnCourseComplete, EffectOperation::AddCombatZing,
             TargetScope::AllAllies, 1);
  add_effect(TriggerHook::OnCourseComplete, EffectOperation::AddCombatBody,
             TargetScope::AllAllies, 1);

  return make_dish(
      "Debug Dish",
      FlavorStats{.satiety = 1,
                  .sweetness = 1,
                  .spice = 1,
                  .acidity = 1,
                  .umami = 1,
                  .richness = 1,
                  .freshness = 1},
      SpriteLocation{0, 0}, 1,
      [](int sourceEntityId) {
        log_info("DEBUG_DISH: OnServe handler triggered for entity {}",
                 sourceEntityId);
      },
      effects);
}

DishInfo get_dish_info(DishType type) {
  switch (type) {
  // Tier 1: Raw/Single Ingredient
  case DishType::Potato:
    return make_dish("Potato", FlavorStats{.satiety = 1}, SpriteLocation{13, 3},
                     1); // potato.png x=416,y=96 - Tier 1
  case DishType::Salmon:
    return make_salmon();
  case DishType::Bagel: {
    std::vector<DishEffect> effects;
    DishEffect bagelEffect;
    bagelEffect.triggerHook = TriggerHook::OnServe;
    bagelEffect.operation = EffectOperation::AddFlavorStat;
    bagelEffect.targetScope = TargetScope::DishesAfterSelf;
    bagelEffect.amount = 1;
    bagelEffect.flavorStatType = FlavorStatType::Richness;
    effects.push_back(bagelEffect);
    return make_dish("Bagel", FlavorStats{.satiety = 1}, SpriteLocation{13, 0},
                     1, nullptr, effects);
  }
  case DishType::Baguette: {
    std::vector<DishEffect> effects;
    DishEffect baguetteEffect;
    baguetteEffect.triggerHook = TriggerHook::OnServe;
    baguetteEffect.operation = EffectOperation::AddCombatZing;
    baguetteEffect.targetScope = TargetScope::Opponent;
    baguetteEffect.amount = -1;
    effects.push_back(baguetteEffect);
    return make_dish("Baguette", FlavorStats{.satiety = 1},
                     SpriteLocation{1, 0}, 1, nullptr, effects);
  }
  case DishType::GarlicBread: {
    std::vector<DishEffect> effects;
    DishEffect garlicBreadEffect;
    garlicBreadEffect.triggerHook = TriggerHook::OnServe;
    garlicBreadEffect.operation = EffectOperation::AddFlavorStat;
    garlicBreadEffect.targetScope = TargetScope::FutureAllies;
    garlicBreadEffect.amount = 1;
    garlicBreadEffect.flavorStatType = FlavorStatType::Spice;
    effects.push_back(garlicBreadEffect);
    return make_dish("Garlic Bread", FlavorStats{.satiety = 1, .richness = 1},
                     SpriteLocation{0, 9}, 1, nullptr, effects);
  }
  case DishType::FriedEgg: {
    std::vector<DishEffect> effects;
    DishEffect friedEggEffect;
    friedEggEffect.triggerHook = TriggerHook::OnDishFinished;
    friedEggEffect.operation = EffectOperation::AddCombatBody;
    friedEggEffect.targetScope = TargetScope::AllAllies;
    friedEggEffect.amount = 2;
    effects.push_back(friedEggEffect);
    return make_dish("Fried Egg", FlavorStats{.richness = 1},
                     SpriteLocation{2, 7}, 1, nullptr, effects);
  }
  case DishType::FrenchFries:
    return make_french_fries();

  // Tier 2: Simple 2-ingredient items
  case DishType::Pancakes:
    return make_dish("Pancakes", FlavorStats{.sweetness = 1, .satiety = 1},
                     SpriteLocation{33, 0},
                     2); // 79_pancakes.png x=1056,y=0 - Tier 2
  case DishType::Sandwich:
    return make_dish("Sandwich", FlavorStats{.satiety = 2},
                     SpriteLocation{40, 0},
                     2); // 92_sandwich.png x=1280,y=0 - Tier 2
  case DishType::Nacho:
    return make_dish("Nacho", FlavorStats{.satiety = 1, .richness = 1},
                     SpriteLocation{29, 0},
                     2); // 71_nacho.png x=928,y=0 - Tier 2

  // Tier 3: Moderate complexity
  case DishType::Burger:
    return make_dish("Burger", FlavorStats{.satiety = 2, .richness = 1},
                     SpriteLocation{8, 0},
                     3); // 15_burger.png x=256,y=0 - Tier 3
  case DishType::Taco:
    return make_dish("Taco", FlavorStats{.satiety = 2, .spice = 1},
                     SpriteLocation{47, 0},
                     3); // 99_taco.png x=1504,y=0 - Tier 3
  case DishType::Omlet:
    return make_dish("Omlet", FlavorStats{.satiety = 1, .richness = 1},
                     SpriteLocation{31, 0},
                     3); // 73_omlet.png x=992,y=0 - Tier 3
  case DishType::IceCream:
    return make_dish("Ice Cream", FlavorStats{.sweetness = 3},
                     SpriteLocation{18, 1},
                     3); // 57_icecream.png x=576,y=32 - Tier 3

  // Tier 4: Complex dishes
  case DishType::Pizza:
    return make_dish("Pizza", FlavorStats{.satiety = 2, .umami = 1},
                     SpriteLocation{35, 0},
                     4); // 81_pizza.png x=1120,y=0 - Tier 4
  case DishType::Ramen:
    return make_dish("Ramen", FlavorStats{.satiety = 2, .umami = 2},
                     SpriteLocation{39, 0},
                     4); // 87_ramen.png x=1248,y=0 - Tier 4
  case DishType::MacNCheese:
    return make_dish("Mac 'n' Cheese", FlavorStats{.satiety = 2, .richness = 2},
                     SpriteLocation{25, 0},
                     4); // 67_macncheese.png x=800,y=0 - Tier 4
  case DishType::Dumplings:
    return make_dish("Dumplings", FlavorStats{.satiety = 2, .umami = 1},
                     SpriteLocation{17, 0},
                     4); // 36_dumplings.png x=544,y=0 - Tier 4
  case DishType::Burrito:
    return make_dish("Burrito", FlavorStats{.satiety = 2, .umami = 1},
                     SpriteLocation{11, 0},
                     4); // 18_burrito.png x=352,y=0 - Tier 4
  case DishType::Spaghetti:
    return make_dish("Spaghetti", FlavorStats{.satiety = 2, .umami = 1},
                     SpriteLocation{42, 0},
                     4); // 94_spaghetti.png x=1344,y=0 - Tier 4
  case DishType::RoastedChicken:
    return make_dish("Roasted Chicken", FlavorStats{.satiety = 2, .umami = 2},
                     SpriteLocation{37, 0},
                     4); // 85_roastedchicken.png x=1184,y=0 - Tier 4

  // Tier 5: Most complex/Gourmet
  case DishType::Steak:
    return make_dish("Steak", FlavorStats{.satiety = 3, .umami = 2},
                     SpriteLocation{43, 0},
                     5); // 95_steak.png x=1376,y=0 - Tier 5
  case DishType::Sushi:
    return make_dish("Sushi", FlavorStats{.freshness = 2, .umami = 2},
                     SpriteLocation{45, 0},
                     5); // 97_sushi.png x=1440,y=0 - Tier 5
  case DishType::Cheesecake:
    return make_dish("Cheesecake", FlavorStats{.sweetness = 3, .richness = 2},
                     SpriteLocation{2, 1},
                     5); // 22_cheesecake.png x=64,y=32 - Tier 5
  case DishType::LemonPie:
    return make_dish("Lemon Pie", FlavorStats{.sweetness = 2, .acidity = 1},
                     SpriteLocation{22, 1},
                     5); // 63_lemonpie.png x=704,y=32 - Tier 5
  case DishType::Donut:
    return make_dish("Donut", FlavorStats{.sweetness = 2, .richness = 1},
                     SpriteLocation{10, 1},
                     5); // 34_donut.png x=320,y=32 - Tier 5
  case DishType::Meatball:
    return make_dish("Meatball", FlavorStats{.umami = 2, .satiety = 1},
                     SpriteLocation{27, 0},
                     5); // 69_meatball.png x=864,y=0 - Tier 5
  case DishType::DebugDish:
    return make_debug_dish();
  }
  return DishInfo{};
}

const std::vector<DishType> &get_default_dish_pool() {
  static const std::vector<DishType> pool = {
      DishType::Burger,      DishType::Burrito,        DishType::Cheesecake,
      DishType::Donut,       DishType::Dumplings,      DishType::FriedEgg,
      DishType::FrenchFries, DishType::IceCream,       DishType::LemonPie,
      DishType::MacNCheese,  DishType::Meatball,       DishType::Nacho,
      DishType::Omlet,       DishType::Pancakes,       DishType::Pizza,
      DishType::Ramen,       DishType::RoastedChicken, DishType::Sandwich,
      DishType::Spaghetti,   DishType::Steak,          DishType::Sushi,
      DishType::Taco,        DishType::Salmon,         DishType::Bagel,
      DishType::Baguette,    DishType::GarlicBread,    DishType::Potato,
  };
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