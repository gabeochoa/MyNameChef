# Dish Effects Implementation Plan

## Overview

This document provides the complete design and implementation plan for dish effects across all tiers. Effects are designed to scale with both tier (complexity/power) and level (magnitude), ensuring higher tier dishes remain desirable even when lower tier dishes are leveled up.

## Current State Analysis

**Existing Effects:**
- Tier 1: Salmon (freshness chain), Bagel (future richness), Baguette (opponent -zing), GarlicBread (future spice), FriedEgg (on finish +body), FrenchFries (future +zing)
- Tier 2-5: Most dishes have NO effects

**Level Scaling:**
- Currently: Stats multiply by 2^(level-1) (level 2=2x, level 3=4x, level 4=8x)
- Effects are static (don't scale with level) - **needs implementation**

**Design Goals:**
1. Effects get more complex/powerful as tier increases
2. Effects should scale with level (magnitude increases)
3. Higher tier dishes must outcompete leveled lower tier dishes
4. Players should want to replace 2-3 lower tier dishes with 1 higher tier dish

## Design Principles

### Tier Progression
- **Tier 1**: Simple single-target effects, small amounts (+1/-1)
- **Tier 2**: Multi-target or conditional effects, moderate amounts (+2-3)
- **Tier 3**: Multiple effects or complex targeting, amounts (+3-4)
- **Tier 4**: Multiple triggers, conditional chains, amounts (+4-5)
- **Tier 5**: Multiple effects across triggers, powerful synergies, amounts (+5-6)

### Level Scaling Strategy
- **Option A**: Effects scale linearly with level (level 2 = 2x, level 3 = 3x)
- **Option B**: Effects scale exponentially like stats (level 2 = 2x, level 3 = 4x)
- **Option C**: Higher tier effects have higher base values that outscale even leveled lower tiers

**Recommendation**: Use Option C (higher base values) + Option A (linear scaling) for balance. This ensures tier 5 dishes with +5 effects at level 1 beat tier 1 dishes with +1 effects at level 3.

### Power Scaling by Tier

**Tier 1: Foundation**
- 1 effect per dish
- +1 amounts
- Simple, single-purpose
- **Value**: 1 dish = 1 effect

**Tier 2: Improvement**
- 2 effects per dish
- +2-3 amounts
- Dual-purpose or better targeting
- **Value**: 1 dish = 2 effects worth 1.5x Tier 1

**Tier 3: Significant Upgrade**
- 2-3 effects per dish
- +3-4 amounts
- Multiple triggers or complex conditions
- **Value**: 1 dish = 2.5 effects worth 2x Tier 1

**Tier 4: Major Upgrade**
- 3-4 effects per dish
- +4-5 amounts
- Multiple triggers, conditional chains
- **Value**: 1 dish = 3.5 effects worth 3x Tier 1

**Tier 5: Game-Changing**
- 4-5 effects per dish
- +5-6 amounts
- Multiple effects across all triggers
- **Value**: 1 dish = 4.5 effects worth 4-5x Tier 1

### Replacement Value Examples

**Scenario 1: Replacing 3 Tier 1 with 1 Tier 3**
- 3 Tier 1: 3 effects total, +1 each = 3 total value
- 1 Tier 3 (Burger): 3 effects, +3/+2/+2 = 7 total value
- **Result**: ✅ Worth it (2.3x value)

**Scenario 2: Replacing 2 Tier 2 with 1 Tier 4**
- 2 Tier 2: 4 effects total, +2-3 each = ~10 total value
- 1 Tier 4 (Ramen): 4 effects, +3/+4/+2/+2 = 11 total value
- **Result**: ✅ Worth it (1.1x value)

**Scenario 3: Replacing 4 Tier 1 with 1 Tier 5**
- 4 Tier 1: 4 effects total, +1 each = 4 total value
- 1 Tier 5 (Steak): 6 effects, +2/+4/+3/+6/+3/+5 = 23 total value
- **Result**: ✅ Definitely worth it (5.75x value)

## Complete Effect Tables

### Tier 1 Dishes (Simple, Single Effect)

| Dish | Level 1 Effect | Level 2 Effect | Level 3 Effect | Flavor Text |
|------|---------------|---------------|---------------|-------------|
| **Potato** | No effects | No effects | OnServe: Self +1 Satiety | A raw, unprocessed ingredient. Basic but versatile - perfect for combining into something greater. At higher levels, it becomes more substantial. |
| **Salmon** | OnServe: SelfAndAdjacent +1 Freshness (if adjacent has Freshness, chain) | OnServe: SelfAndAdjacent +2 Freshness (if adjacent has Freshness, chain); OnCourseStart: Self +1 Body | OnServe: SelfAndAdjacent +3 Freshness (if adjacent has Freshness, chain); OnCourseStart: Self +2 Body | Fresh fish pairs beautifully with other fresh ingredients, creating a chain reaction of crisp, clean flavors that enhance each other. At higher levels, it gains staying power. |
| **Bagel** | OnServe: DishesAfterSelf +1 Richness | OnServe: DishesAfterSelf +2 Richness; OnServe: Self +1 Richness | OnServe: DishesAfterSelf +3 Richness; OnServe: Self +2 Richness | The rich, doughy base sets the stage for what comes next, making subsequent dishes more indulgent and satisfying. Higher levels enrich itself as well. |
| **Baguette** | OnServe: Opponent -1 Zing | OnServe: Opponent -2 Zing; OnCourseStart: Self +1 Body | OnServe: Opponent -3 Zing; OnCourseStart: Self +2 Body | A simple, unassuming bread that takes the edge off your opponent's attack, dulling their sharp flavors with its plain, neutral presence. Higher levels provide defensive bulk. |
| **GarlicBread** | OnServe: FutureAllies +1 Spice | OnServe: FutureAllies +2 Spice; OnServe: Self +1 Spice | OnServe: FutureAllies +3 Spice; OnServe: Self +2 Spice | The garlic's bold flavor lingers and infuses everything that follows, giving your upcoming dishes a spicy kick. Higher levels spice itself up too. |
| **FriedEgg** | OnDishFinished: AllAllies +2 Body | OnDishFinished: AllAllies +4 Body; OnServe: Self +1 Body | OnDishFinished: AllAllies +6 Body; OnServe: Self +2 Body | A complete protein that, even after being consumed, provides lasting nourishment to your entire team - the gift that keeps on giving. Higher levels start stronger. |
| **FrenchFries** | OnServe: FutureAllies +1 Zing | OnServe: FutureAllies +2 Zing; OnServe: Self +1 Zing | OnServe: FutureAllies +3 Zing; OnServe: Self +2 Zing | Crispy, energizing, and addictive - these fries give your upcoming dishes an extra burst of excitement and flavor. Higher levels energize itself as well. |

**Tier 1 Synergies:**
- **Support Chain**: Bagel → GarlicBread → FrenchFries (future dishes get +Richness, +Spice, +Zing)
- **Defensive**: Baguette debuffs opponent, FriedEgg gives body on finish
- **Freshness Chain**: Salmon chains with other fresh dishes
- **Works well together**: Support dishes set up future dishes

### Tier 2 Dishes (Dual Effects or Better Targeting)

| Dish | Level 1 Effect | Level 2 Effect | Level 3 Effect | Flavor Text |
|------|---------------|---------------|---------------|-------------|
| **Pancakes** | OnServe: AllAllies +2 Sweetness | OnServe: AllAllies +4 Sweetness; OnCourseStart: Self +2 Body | OnServe: AllAllies +6 Sweetness; OnCourseStart: Self +4 Body; OnBiteTaken: Self +2 Body | A sweet morning boost that energizes the whole team. Higher levels gain staying power and resilience. |
| **Sandwich** | OnServe: Self +3 Satiety | OnServe: Self +6 Satiety; OnBiteTaken: Self +2 Body | OnServe: Self +9 Satiety; OnBiteTaken: Self +4 Body; OnDishFinished: AllAllies +2 Body | A filling meal that fortifies itself, growing more defensive as it takes hits. At max level, it shares its strength with the team. |
| **Nacho** | OnServe: SelfAndAdjacent +2 Richness | OnServe: SelfAndAdjacent +4 Richness; OnBiteTaken: Opponent -2 Zing | OnServe: SelfAndAdjacent +6 Richness; OnBiteTaken: Opponent -4 Zing; OnCourseStart: Self +2 Body | Rich, cheesy goodness shared with neighbors. Higher levels counter attacks and gain defensive bulk. |

**Tier 2 Synergies:**
- **Pancakes**: Sweetness boost for team
- **Sandwich**: Self-buffing tank
- **Nacho**: Adjacent support + opponent debuff
- **Note**: Less synergy between them, more self-contained

### Tier 3 Dishes (Multiple Effects or Complex Conditions)

| Dish | Level 1 Effect | Level 2 Effect | Level 3 Effect | Flavor Text |
|------|---------------|---------------|---------------|-------------|
| **Burger** | OnServe: Self +3 Richness, AllAllies +2 Body | OnServe: Self +6 Richness, AllAllies +4 Body; OnBiteTaken: Self +2 Zing | OnServe: Self +9 Richness, AllAllies +6 Body; OnBiteTaken: Self +4 Zing; OnDishFinished: AllAllies +3 Body | A rich, satisfying meal that gets stronger with every bite. Higher levels unlock combat scaling and team finish bonus. |
| **Taco** | OnServe: FutureAllies +3 Spice | OnServe: FutureAllies +6 Spice; OnBiteTaken: Opponent -2 Body | OnServe: FutureAllies +9 Spice; OnBiteTaken: Opponent -4 Body; OnCourseStart: Self +2 Zing | Spicy preparation for what's coming. Higher levels burn opponent defenses and gain offensive power. |
| **Omlet** | OnServe: Self +2 Richness, Self +2 Satiety | OnServe: Self +4 Richness, Self +4 Satiety; OnDishFinished: AllAllies +4 Body | OnServe: Self +6 Richness, Self +6 Satiety; OnDishFinished: AllAllies +8 Body; OnCourseStart: Self +2 Body | A complete, balanced meal that fortifies itself. Higher levels unlock team finish bonus and defensive bulk. |
| **Ice Cream** | OnServe: AllAllies +3 Sweetness | OnServe: AllAllies +6 Sweetness; OnCourseStart: Self +3 Body; OnBiteTaken: Opponent -2 Zing | OnServe: AllAllies +9 Sweetness; OnCourseStart: Self +6 Body; OnBiteTaken: Opponent -4 Zing; OnDishFinished: AllAllies +3 Body | Cold, sweet protection that freezes your opponent's momentum. Higher levels gain defensive bulk, debuff power, and team finish bonus. |

**Tier 3 Synergies:**
- **Burger**: Self-buff + team support + combat scaling
- **Taco**: Future spice prep + opponent debuff
- **Omlet**: Self-buff + team finish bonus
- **Ice Cream**: Team sweetness + self defense + opponent debuff
- **Better**: More self-contained, less dependent on team

### Tier 4 Dishes (Complex Multi-Trigger Effects)

| Dish | Level 1 Effect | Level 2 Effect | Level 3 Effect | Flavor Text |
|------|---------------|---------------|---------------|-------------|
| **Pizza** | OnServe: AllAllies +2 Umami, DishesAfterSelf +2 Satiety | OnServe: AllAllies +4 Umami, DishesAfterSelf +4 Satiety; OnCourseStart: Self +3 Body; OnBiteTaken: Self +2 Zing | OnServe: AllAllies +6 Umami, DishesAfterSelf +6 Satiety; OnCourseStart: Self +6 Body; OnBiteTaken: Self +4 Zing; OnDishFinished: AllAllies +3 Body | A complete meal that sets up your team. Higher levels gain combat scaling and team finish bonus. |
| **Ramen** | OnServe: AllAllies +3 Umami | OnServe: AllAllies +6 Umami; OnCourseStart: Self +4 Body, AllAllies +2 Zing; OnBiteTaken: Self +2 Umami | OnServe: AllAllies +9 Umami; OnCourseStart: Self +8 Body, AllAllies +4 Zing; OnBiteTaken: Self +4 Umami; OnDishFinished: AllAllies +4 Body | A rich, savory broth that builds up your team. Higher levels gain combat scaling, team zing, and finish bonus. |
| **MacNCheese** | OnServe: Self +3 Richness, AllAllies +2 Richness | OnServe: Self +6 Richness, AllAllies +4 Richness; OnCourseStart: Self +4 Body; OnDishFinished: AllAllies +3 Body | OnServe: Self +9 Richness, AllAllies +6 Richness; OnCourseStart: Self +8 Body; OnDishFinished: AllAllies +6 Body; OnBiteTaken: Self +2 Body | Ultimate comfort food that enriches the team. Higher levels gain defensive bulk and combat resilience. |
| **Dumplings** | OnServe: SelfAndAdjacent +2 Umami | OnServe: SelfAndAdjacent +4 Umami; OnCourseStart: Self +3 Body, Self +2 Zing; OnBiteTaken: Opponent -2 Body | OnServe: SelfAndAdjacent +6 Umami; OnCourseStart: Self +6 Body, Self +4 Zing; OnBiteTaken: Opponent -4 Body; OnDishFinished: AllAllies +3 Body | Steamed to perfection, sharing savory flavor with neighbors. Higher levels gain combat power, debuff, and team finish bonus. |
| **Burrito** | OnServe: FutureAllies +3 Umami | OnServe: FutureAllies +6 Umami; OnCourseStart: Self +3 Body, Self +2 Zing; OnBiteTaken: Self +2 Satiety | OnServe: FutureAllies +9 Umami; OnCourseStart: Self +6 Body, Self +4 Zing; OnBiteTaken: Self +4 Satiety; OnDishFinished: AllAllies +3 Body | A filling wrap that prepares your future dishes. Higher levels gain combat scaling and team finish bonus. |
| **Spaghetti** | OnServe: AllAllies +2 Umami, DishesAfterSelf +2 Satiety | OnServe: AllAllies +4 Umami, DishesAfterSelf +4 Satiety; OnCourseStart: Self +3 Body; OnBiteTaken: Self +2 Zing, Opponent -2 Zing | OnServe: AllAllies +6 Umami, DishesAfterSelf +6 Satiety; OnCourseStart: Self +6 Body; OnBiteTaken: Self +4 Zing, Opponent -4 Zing; OnDishFinished: AllAllies +4 Body | A tug-of-war of flavors. Higher levels gain combat scaling and team finish bonus. |
| **RoastedChicken** | OnServe: AllAllies +3 Umami, Self +2 Satiety | OnServe: AllAllies +6 Umami, Self +4 Satiety; OnCourseStart: Self +5 Body; OnDishFinished: AllAllies +4 Body | OnServe: AllAllies +9 Umami, Self +6 Satiety; OnCourseStart: Self +10 Body; OnDishFinished: AllAllies +8 Body; OnBiteTaken: Self +3 Body | A feast that nourishes your team. Higher levels gain massive defensive bulk and combat resilience. |

**Tier 4 Synergies:**
- **Pizza**: Team umami + future satiety + self scaling
- **Ramen**: Team umami + team zing + self scaling
- **MacNCheese**: Richness stacking + finish bonus
- **Dumplings**: Adjacent umami + self scaling + opponent debuff
- **Burrito**: Future umami + self scaling
- **Spaghetti**: Team umami + future satiety + tug-of-war
- **RoastedChicken**: Team umami + massive self body + finish bonus
- **Good**: Each dish is self-sufficient but also supports team

### Tier 5 Dishes (Powerful Multi-Effect Synergies)

| Dish | Level 1 Effect | Level 2 Effect | Level 3 Effect | Flavor Text |
|------|---------------|---------------|---------------|-------------|
| **Steak** | OnStartBattle: AllAllies +2 Zing; OnServe: Self +4 Umami, Self +3 Body | OnStartBattle: AllAllies +4 Zing; OnServe: Self +8 Umami, Self +6 Body; OnCourseStart: Self +6 Body; OnBiteTaken: Self +3 Zing; OnDishFinished: AllAllies +5 Body | OnStartBattle: AllAllies +6 Zing; OnServe: Self +12 Umami, Self +9 Body; OnCourseStart: Self +12 Body; OnBiteTaken: Self +6 Zing; OnDishFinished: AllAllies +10 Body | Premium preparation, premium cut. Higher levels unlock combat scaling, bite power, and team finish bonus. |
| **Sushi** | OnServe: SelfAndAdjacent +3 Freshness (if adjacent has freshness, chain) | OnServe: SelfAndAdjacent +6 Freshness (if adjacent has freshness, chain); OnCourseStart: Self +4 Body, AllAllies +2 Zing; OnBiteTaken: Self +3 Zing; OnDishFinished: AllAllies +3 Freshness | OnServe: SelfAndAdjacent +9 Freshness (if adjacent has freshness, chain); OnCourseStart: Self +8 Body, AllAllies +4 Zing; OnBiteTaken: Self +6 Zing; OnDishFinished: AllAllies +6 Freshness | Precision-crafted freshness that chains with other fresh ingredients. Higher levels unlock team zing, combat power, and finish bonus. |
| **Cheesecake** | OnServe: AllAllies +4 Sweetness | OnServe: AllAllies +8 Sweetness; OnCourseStart: Self +5 Body, AllAllies +2 Body; OnBiteTaken: Opponent -3 Zing; OnDishFinished: AllAllies +4 Body | OnServe: AllAllies +12 Sweetness; OnCourseStart: Self +10 Body, AllAllies +4 Body; OnBiteTaken: Opponent -6 Zing; OnDishFinished: AllAllies +8 Body | Rich, overwhelming sweetness that protects your team. Higher levels unlock defensive bulk, team body, debuff, and finish bonus. |
| **LemonPie** | OnServe: AllAllies +3 Sweetness, Opponent -2 Zing | OnServe: AllAllies +6 Sweetness, Opponent -4 Zing; OnCourseStart: Self +4 Body; OnBiteTaken: Opponent -3 Zing; OnDishFinished: AllAllies +3 Zing | OnServe: AllAllies +9 Sweetness, Opponent -6 Zing; OnCourseStart: Self +8 Body; OnBiteTaken: Opponent -6 Zing; OnDishFinished: AllAllies +6 Zing | Sweet yet tart - a zesty combination. Higher levels unlock defensive bulk, stronger debuffs, and team finish bonus. |
| **Donut** | OnServe: AllAllies +3 Sweetness, FutureAllies +2 Richness | OnServe: AllAllies +6 Sweetness, FutureAllies +4 Richness; OnCourseStart: Self +4 Body; OnBiteTaken: Self +2 Body; OnDishFinished: AllAllies +3 Body | OnServe: AllAllies +9 Sweetness, FutureAllies +6 Richness; OnCourseStart: Self +8 Body; OnBiteTaken: Self +4 Body; OnDishFinished: AllAllies +6 Body | A sweet treat that energizes your team now and enriches what's to come. Higher levels unlock defensive bulk and combat resilience. |
| **Meatball** | OnServe: Self +3 Umami, AllAllies +2 Umami | OnServe: Self +6 Umami, AllAllies +4 Umami; OnCourseStart: Self +4 Body, Self +2 Zing; OnBiteTaken: Self +3 Zing; OnDishFinished: AllAllies +4 Body | OnServe: Self +9 Umami, AllAllies +6 Umami; OnCourseStart: Self +8 Body, Self +4 Zing; OnBiteTaken: Self +6 Zing; OnDishFinished: AllAllies +8 Body | Meaty, satisfying, and powerful. Higher levels unlock combat scaling, bite power, and team finish bonus. |

**Tier 5 Synergies:**
- **Steak**: Battle start + serve + course + bite + finish (6 triggers!)
- **Sushi**: Freshness chain + team zing + precision + finish
- **Cheesecake**: Team sweetness + team body + opponent debuff + finish
- **LemonPie**: Team sweetness + opponent debuff + finish zing
- **Donut**: Team sweetness + future richness + self resilience + finish
- **Meatball**: Team umami + self scaling + finish
- **Excellent**: Each dish is a complete package

## Level-Based Effects Implementation

**Proposed Approach**: Each level can have completely different effects, not just scaled amounts
- Level 1: Base effects (as shown in Level 1 column)
- Level 2: Can have different effects, different triggers, or no effects
- Level 3: Can have different effects, different triggers, or no effects
- Level 4+: Can have different effects, different triggers, or no effects

**Key Design Principle**: Levels can unlock new effects, change effect types, or remain the same. This allows for:
- **Progressive Unlocks**: Level 1 has no effects, Level 3 unlocks effects (e.g., Potato)
- **Effect Evolution**: Level 1 has simple effects, Level 2 adds new triggers, Level 3 adds more complexity
- **Flexible Scaling**: Some dishes scale amounts, others unlock new effects entirely

**Example**: 
- **Potato**: Level 1-2: No effects, Level 3: OnServe: Self +1 Satiety (unlocks basic effect)
- **FrenchFries**: Level 1: +1 Zing, Level 2: +2 Zing, Level 3: +3 Zing (linear scaling)
- **Steak**: Level 1: 5 effects, Level 2: Same 5 effects with 2x amounts, Level 3: Same 5 effects with 3x amounts (scaled)
- **Burger**: Level 1: 3 effects, Level 2: 3 effects + 1 new effect, Level 3: 4 effects + 1 more new effect (progressive)

**Balance Check**: Even a level 3 tier 1 dish (4x stats, potentially new effects) can't compete with tier 5 dishes that have:
- Higher base stats (3 satiety + 2 umami vs 1 satiety)
- Multiple powerful effects from level 1 (5+ effects vs 0-2)
- Better effect amounts (+5-6 base vs +1 base or none)

## Implementation Notes

1. **Effect Amounts**: Higher tier = higher base amounts, ensuring tier advantage
2. **Effect Count**: Higher tier = more effects across more triggers
3. **Effect Complexity**: Higher tier = conditional chains, multi-target synergies
4. **Level-Based Effects**: Each level can have completely different effects:
   - **Progressive Unlocks**: Lower levels have no effects, higher levels unlock them
   - **Effect Evolution**: Effects can change type, target, or trigger at different levels
   - **Flexible Scaling**: Some dishes scale amounts, others unlock entirely new effects
5. **Balance**: Tier 5 level 1 should beat Tier 1 level 3 through raw power and effect count
6. **Implementation**: `get_dish_info(DishType type, int level = 1)` will return different effect sets based on level

## Files to Modify

- `src/dish_types.h`: Update `get_dish_info()` signature to accept level parameter: `DishInfo get_dish_info(DishType type, int level = 1)`
- `src/dish_types.cpp`: 
  - Update `get_dish_info()` to accept level parameter
  - Implement level-based effect selection (switch on level to return different effect sets)
  - Add effects to all dishes using DishBuilder, with different effects per level
- `src/systems/EffectResolutionSystem.h`: 
  - Update to get dish level from entity and pass to `get_dish_info(dish.type, level)`
  - Remove any level-based amount scaling (effects are now level-specific)

## Implementation Example

Here's how `get_dish_info()` would work with level-based effects using helper functions:

```cpp
static DishInfo make_potato(int level) {
  auto builder = dish()
      .with_name("Potato")
      .with_sprite(SpriteLocation{13, 3})
      .with_tier(1);
  
  switch (level) {
  case 1:
    return builder.with_flavor(FlavorStats{.satiety = 1}).build();
  case 2:
    return builder.with_flavor(FlavorStats{.satiety = 2}).build();
  case 3:
  default:
    return builder
        .with_flavor(FlavorStats{.satiety = 3})
        .register_on_serve(
            ServeEffect().with_target(TargetScope::Self)
                        .add_flavor_stat(FlavorStatType::Satiety, 1))
        .build();
  }
}

DishInfo get_dish_info(DishType type, int level = 1) {
  switch (type) {
  case DishType::Potato:
    return make_potato(level);
  // ... other dishes
  }
}
```

And in `EffectResolutionSystem`, we'd get the level from the entity:

```cpp
void process_trigger_event(const TriggerEvent &ev) {
  auto src_opt = EQ({.ignore_temp_warning = true})
                     .whereID(ev.sourceEntityId)
                     .gen_first();
  if (!src_opt || !src_opt->has<IsDish>()) {
    return;
  }

  const auto &dish = src_opt->get<IsDish>();
  int level = 1; // Default level
  if (src_opt->has<DishLevel>()) {
    level = src_opt->get<DishLevel>().level;
  }
  
  const auto &info = get_dish_info(dish.type, level); // Pass level here

  for (const auto &effect : info.effects) {
    if (effect.triggerHook == ev.hook) {
      apply_effect(effect, ev);
    }
  }
}
```

## Implementation Steps

1. **Update `get_dish_info()` signature** to accept level parameter (default to 1 for backward compatibility)
2. **Update `EffectResolutionSystem`** to get dish level from entity and pass to `get_dish_info()`
3. **Implement level-based effect selection** in `get_dish_info()`:
   - Use switch statement on level to return different effect sets
   - For dishes that scale: return same effects with different amounts
   - For dishes that unlock: return empty effects for lower levels, effects for higher levels
   - For dishes that evolve: return different effects per level
4. **Add effects to Tier 1 dishes** with level-based selection (Potato: no effects L1-2, effects L3+)
5. **Add effects to Tier 2 dishes** (Pancakes, Sandwich, Nacho) with level-based selection
6. **Add effects to Tier 3 dishes** (Burger, Taco, Omlet, Ice Cream) with level-based selection
7. **Add effects to Tier 4 dishes** (Pizza, Ramen, MacNCheese, Dumplings, Burrito, Spaghetti, RoastedChicken) with level-based selection
8. **Add effects to Tier 5 dishes** (Steak, Sushi, Cheesecake, LemonPie, Donut, Meatball) with level-based selection
9. **Test and validate** all effects work correctly at each level

## Validation

After implementation:
1. Test tier 1 level 3 vs tier 5 level 1 (tier 5 should win)
2. Test that dishes have correct effects at each level (e.g., Potato has no effects at L1-2, effects at L3)
3. Test that level-based effect selection works correctly (different levels return different effects)
4. Test all trigger hooks fire correctly at each level
5. Verify effect targeting works as expected at each level
6. Verify replacement value: 1 tier 3 dish should outperform 2-3 tier 1 dishes
7. Verify replacement value: 1 tier 5 dish should outperform 4-5 tier 1 dishes
8. Test backward compatibility: existing code that calls `get_dish_info(type)` without level should default to level 1

