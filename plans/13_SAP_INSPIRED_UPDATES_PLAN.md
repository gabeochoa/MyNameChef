# Super Auto Pets Inspired Updates Plan

## At-a-Glance
- **Sequence:** 13 / 21 — content roadmap for drinks/dishes inspired by SAP once core systems (09–12) are online.
- **Strategic Goal:** Deliver a compelling progression arc with new drink pairings, dish archetypes, and effect operations that mirror SAP depth while fitting our combat model.
- **Status:** Design matrices for drinks/dishes/effect ops are complete; engineering blocked on prerequisite systems (swap stats, status effects, duration, summoning).
- **KPIs:** Increased player retention through higher-tier content engagement, telemetry shows new items replacing lower tiers at desired rates.

## Work Breakdown Snapshot
|Wave|Scope|Prereqs|Deliverables|Exit Criteria|
|---|---|---|---|---|
|1. Drink Core Pack|Tier 1–3 drinks|Effect ops (AddFlavorStat, AddCombat modifiers) already available|Implement Coffee/OrangeJuice/Sodas with scaling rules|Shop tests verify costs/effects, telemetry toggled|
|2. Drink Advanced Pack|Tier 4–5 drinks|New operations (MultiplyDamage, PreventDefeat)|Implement Wine/Watermelon/Yellow Soda family|Balance pass vs dishes, effect ops stable|
|3. Dish Pack A|Tier 2–3 dishes|SwapStats, SummonDish, ApplyStatus|Add Miso/Tempura/Risotto/Pho/Churros|Combat sims confirm replacement value|
|4. Dish Pack B|Tier 4–5 dishes|MultiplyDamage, PreventDefeat, CopyEffect|Add Bouillabaisse/Foie Gras/Wagyu etc.|High-tier dishes beat lower-tier combos per design|
|5. Economy & Shop Enhancements|Future|Shop modifiers, gold generation ops|Implement ShopModifier/GenerateGold effects|Telemetry ensures economy stays stable|

## Overview
Design and implement new drink types and dish types with effects inspired by Super Auto Pets mechanics. This plan covers:
- **Drink pairings**: Drinks applied to dishes that enhance them during battle
- **New dish types**: Advanced dishes with SAP-inspired mechanics
- **New effect operations**: System enhancements to support advanced mechanics

**Relationship to Other Plans:**
- `DISH_EFFECTS_IMPLEMENTATION.md` focuses on implementing effects for existing dishes
- All plans share the same effect system infrastructure

**Super Auto Pets Inspiration**: This design incorporates mechanics inspired by Super Auto Pets, including:
- **Scaling effects** (Carrot-style): Effects that trigger repeatedly over time (OnCourseComplete, OnBiteTaken)
- **Trade-off effects** (Broccoli/Fried Shrimp-style): Strategic choices with negative stat modifications
- **Team support** (Stew/Taco-style): Buffing multiple allies or adjacent dishes
- **Reactive scaling** (Cucumber-style): Effects that trigger when taking damage
- **Protection effects** (Pepper-style): Prevent defeat
- **Double damage** (Cheese-style): Amplify next attack
- **Stat swapping** (Lollipop-style): Exchange stats
- **Summoning** (Popcorn-style): Create temporary dishes

## Current State Analysis

### Drinks
- Only `Water` drink exists (no effect currently)
- `DrinkPairing` component stores drink type on dishes
- No system processes drink pairings to apply effects during battle

### Dishes
- Existing dishes have basic effects (see `DISH_EFFECTS_IMPLEMENTATION.md`)
- Most Tier 2-5 dishes have no effects yet
- Effect system supports: `TriggerHook`, `EffectOperation` (AddFlavorStat, AddCombatZing, AddCombatBody), `TargetScope`

### Effect System
- Available triggers: OnStartBattle, OnServe, OnCourseStart, OnBiteTaken, OnDishFinished, OnCourseComplete
- Available operations: AddFlavorStat, AddCombatZing, AddCombatBody
- Available target scopes: Self, Opponent, AllAllies, AllOpponents, DishesAfterSelf, FutureAllies, FutureOpponents, Previous, Next, SelfAndAdjacent
- Negative amounts are supported (for trade-off effects)

## Available Drink Sprites

Based on the spritesheet JSON (`resources/images/spritesheet_pixelfood.json`), the following drink sprites are available in the "beverages" group (row 5, y=160, each sprite is 32x32):
- Column 0: `coffee_bag.png` - Coffee
- Column 1: `hot_cocoa_mix.png` - Hot Cocoa
- Column 2: `orange_juice.png` - Orange Juice
- Column 3: `soft_drink_blue.png` - Soda (blue)
- Column 4: `soft_drink_green.png` - Soda (green)
- Column 5: `soft_drink_red.png` - Soda (red)
- Column 6: `soft_drink_yellow.png` - Soda (yellow)
- Column 7: `water.png` - Water (currently used, sprite location: i=7, j=5)
- Column 8: `watermelon1.png` - Watermelon (variant 1)
- Column 9: `watermelon2.png` - Watermelon (variant 2)
- Column 10: `wine_red.png` - Red Wine
- Column 11: `wine_white.png` - White Wine
- Column 12: `wine_white2.png` - White Wine (variant 2)
- Column 13: `wine_white3.png` - White Wine (variant 3)

## Proposed New Effect Operations

### High Priority (Feasible with Current System)
1. **`SwapStats`** - Exchange Zing and Body values
   - Use case: Lollipop-style strategic repositioning
   - Implementation: Read current stats, swap them
   - Needed for: Churros (Tier 3 dish), potential advanced drinks

2. **`MultiplyDamage`** - Double/triple next attack
   - Use case: Cheese-style burst damage
   - Implementation: Temporary component consumed on first damage
   - Needed for: Bouillabaisse (Tier 4 dish), potential advanced drinks

3. **`PreventDefeat`** - Body cannot drop below 1
   - Use case: Pepper-style protection
   - Implementation: Hook into damage calculation
   - Needed for: Foie Gras (Tier 5 dish), potential advanced drinks

### Medium Priority (Requires New Systems)
4. **`SummonDish`** - Create temporary dish on trigger
   - Use case: Popcorn-style summoning
   - Implementation: Battle entity creation system
   - Needed for: Pho (Tier 3 dish)

5. **`CopyEffect`** - Copy effect from target dish
   - Use case: Mimic-style copying
   - Implementation: Read target's effects, apply to self
   - Needed for: Paella (Tier 4 dish)

6. **`ApplyStatus`** - Apply debuff to opponent
   - Use case: Weakness, poison, etc.
   - Implementation: New status effect component system
   - Needed for: Wagyu Steak (Tier 5 dish)

7. **`RandomTarget`** - Target random allies/opponents
   - Use case: Stew/Taco-style random buffing
   - Implementation: New `TargetScope::RandomAllies(N)` or `TargetScope::RandomOpponents(N)`
   - Enables "give +X to 2 random allies" effects

### Low Priority (Complex Systems)
8. **`TransformDish`** - Temporarily change dish type
   - Use case: Transformation effects
   - Implementation: Complex state management

9. **`ShopModifier`** - Modify shop phase
   - Use case: Reduce costs, improve rolls
   - Implementation: New shop interaction system

10. **`GenerateGold`** - Economic effects
    - Use case: Grapes-style resource generation
    - Implementation: Hook into shop/economy system
    - Outside combat scope, but could trigger OnStartBattle

**Implementation Priority Summary:**
- **High**: Negative amounts (already supported), Scaling effects (already supported via OnCourseComplete, OnBiteTaken)
- **Medium**: SwapStats, MultiplyDamage, PreventDefeat, RandomTarget scope, SummonDish, CopyEffect, ApplyStatus
- **Low**: TransformDish, ShopModifier, GenerateGold

## Proposed Drink Types & Effects

### Tier 1 (Basic - 1-2 gold)

1. **Water** (existing, column 7, row 5)
   - Price: 1 gold
   - Effect: None (baseline)
   - Sprite: i=7, j=5

2. **Orange Juice** (column 2, row 5)
   - Price: 1 gold
   - Effect: OnServe - Add +1 Freshness (flavor stat, increases Body)
   - Theme: Fresh, revitalizing
   - Sprite: i=2, j=5

3. **Coffee** (column 0, row 5)
   - Price: 2 gold
   - Effect: OnStartBattle - Add +2 Combat Zing to Self
   - Theme: Energizing, quick boost
   - Sprite: i=0, j=5

4. **Soda** (use red variant, column 5, row 5) - *SAP-inspired: Scaling effect*
   - Price: 2 gold
   - Effect: OnCourseComplete - Add +1 Combat Zing to Self (scales each course)
   - Theme: Bubbly, per-course boost that accumulates
   - Sprite: i=5, j=5
   - Note: Inspired by Carrot perk - provides scaling value over time

### Tier 2 (Moderate - 2-3 gold)

5. **Hot Cocoa** (column 1, row 5)
   - Price: 2 gold
   - Effect: OnServe - Add +1 Sweetness and +1 Richness (flavor stats)
   - Theme: Warm, comforting, body-focused
   - Sprite: i=1, j=5

6. **Red Wine** (column 10, row 5) - *SAP-inspired: Team support*
   - Price: 3 gold
   - Effect: OnServe - Add +1 Richness to Self and +1 Richness to Next Ally
   - Theme: Sophisticated, body-focused, supports adjacent dishes
   - Sprite: i=10, j=5
   - Note: Uses `TargetScope::Next` for adjacent support

7. **Green Soda** (column 4, row 5) - *SAP-inspired: Trade-off effect*
   - Price: 2 gold
   - Effect: OnServe - Add +2 Combat Zing, Add -1 Combat Body to Self
   - Theme: Energy drink, glass cannon build
   - Sprite: i=4, j=5
   - Note: Inspired by Fried Shrimp - offensive trade-off

8. **Blue Soda** (column 3, row 5) - *SAP-inspired: Scaling durability*
   - Price: 2 gold
   - Effect: OnCourseComplete - Add +1 Combat Body to Self
   - Theme: Refreshing, scales durability over time
   - Sprite: i=3, j=5
   - Note: Inspired by Cucumber - gradual health scaling

### Tier 3 (Advanced - 3-4 gold)

9. **White Wine** (column 11, row 5) - *SAP-inspired: Team-wide buff*
   - Price: 3 gold
   - Effect: OnStartBattle - Add +1 Combat Zing to All Allies
   - Theme: Celebratory, team boost
   - Sprite: i=11, j=5
   - Note: Inspired by Taco/Stew - team-wide support

10. **Watermelon Juice** (use column 8, row 5) - *SAP-inspired: Multi-stat scaling*
    - Price: 3 gold
    - Effect: OnCourseComplete - Add +1 Freshness and +1 Combat Body to Self
    - Theme: Refreshing, maximum freshness with scaling durability
    - Sprite: i=8, j=5
    - Note: Inspired by Carrot - multi-stat scaling effect

11. **Yellow Soda** (column 6, row 5) - *SAP-inspired: Per-bite scaling*
    - Price: 3 gold
    - Effect: OnBiteTaken - Add +1 Combat Zing to Self (when this dish takes damage)
    - Theme: Adrenaline rush, scales with combat intensity
    - Sprite: i=6, j=5
    - Note: Inspired by scaling mechanics - reactive growth

### Future Drink Expansion (when sprites available)
- Milk - Could be: OnServe - Add +1 Body to Self (nourishing)
- Tea - Could be: OnCourseStart - Add +1 Zing to Self (energizing)
- Energy Drink - Could be: OnStartBattle - Add +3 Zing, -2 Body (extreme trade-off)
- Smoothie - Could be: OnServe - Add +1 to all flavor stats (balanced boost)
- Honey - Could be: OnServe - Add +2 Sweetness (flavor-focused)
- Vinegar - Could be: OnServe - Add +2 Acidity, -1 Body (sour trade-off)
- Maple Syrup - Could be: OnServe - Add +3 Sweetness (sweet-focused)
- Lemonade - Could be: OnCourseComplete - Add +1 Freshness and +1 Zing (scaling refresh)

## Proposed New Dish Types

### Tier 2-3: Scaling & Support Dishes

**Miso Soup** (Tier 2)
- Effect: OnCourseComplete - Add +1 Body to Self (scaling durability)
- Theme: Nourishing, builds over time
- SAP Inspiration: Cucumber (health scaling)
- **Uses existing operations**: AddCombatBody

**Tempura** (Tier 2)
- Effect: OnBiteTaken - Add +1 Zing to Self (reactive scaling)
- Theme: Crispy, gets stronger when hit
- SAP Inspiration: Reactive growth mechanics
- **Uses existing operations**: AddCombatZing

**Risotto** (Tier 3)
- Effect: OnServe - Add +1 Richness to Self and Next Ally
- Theme: Creamy, supports adjacent dishes
- SAP Inspiration: Team support effects
- **Uses existing operations**: AddFlavorStat with TargetScope::Next

### Tier 3-4: Advanced Mechanics

**Pho** (Tier 3)
- Effect: OnDishFinished - Summon 1/1 Broth (weak dish) to same slot
- Theme: Nourishing broth remains after main ingredient
- SAP Inspiration: Summoning on defeat
- **Requires**: `SummonDish` operation (Medium priority)

**Churros** (Tier 3)
- Effect: OnServe - Swap Zing and Body values
- Theme: Sweet but fragile, strategic repositioning
- SAP Inspiration: Lollipop (stat swap)
- **Requires**: `SwapStats` operation (High priority)

**Paella** (Tier 4)
- Effect: OnStartBattle - Copy effect from random ally
- Theme: Complex dish that adapts to team
- SAP Inspiration: Copy abilities
- **Requires**: `CopyEffect` operation (Medium priority)

### Tier 4-5: Complex Strategic Dishes

**Bouillabaisse** (Tier 4)
- Effect: OnServe - Next attack deals 2x damage
- Theme: Rich, powerful first strike
- SAP Inspiration: Cheese (double damage)
- **Requires**: `MultiplyDamage` operation (High priority)

**Foie Gras** (Tier 5)
- Effect: OnStartBattle - Body cannot drop below 1 (one-time)
- Theme: Luxurious, protected
- SAP Inspiration: Pepper (survival)
- **Requires**: `PreventDefeat` operation (High priority)

**Wagyu Steak** (Tier 5)
- Effect: OnBiteTaken - Apply -1 Zing to Opponent (debuff)
- Theme: Premium, weakens opponents
- SAP Inspiration: Status effects
- **Requires**: `ApplyStatus` operation (Medium priority)

## Implementation Strategy

### Phase 1: Core Drink System (Can be done independently)
1. Extend `DrinkType` enum (`src/drink_types.h`)
   - Add new drink types: Coffee, OrangeJuice, Soda (Red), GreenSoda, BlueSoda, YellowSoda, HotCocoa, RedWine, WhiteWine, WatermelonJuice
   - Total: 11 drink types (including Water)

2. Update `DrinkInfo` structure (`src/drink_types.h`)
   - Add `SpriteLocation` field to `DrinkInfo` (similar to `DishInfo`)
   - Update `get_drink_info()` to return sprite locations for each drink

3. Implement drink info (`src/drink_types.cpp`)
   - Implement `get_drink_info()` for all new drinks with name, price, sprite location

4. Create Drink Effect System (`src/systems/ApplyDrinkPairingEffects.h`)
   - New system that processes `DrinkPairing` components
   - Converts drink pairings into `DishEffect` components when dish enters battle
   - Runs during battle initialization (similar to `ApplyPairingsAndClashesSystem`)
   - Maps drink types to their corresponding effects
   - Should run when dish phase is InQueue (before combat starts)

5. Integrate with Effect Resolution
   - Ensure drink-generated effects are processed by `EffectResolutionSystem`
   - Effects should trigger at appropriate `TriggerHook` points
   - Effects are added to dish's effect list (similar to how dish effects work)

6. Update Shop Integration
   - Update `GenerateDrinkShop` to use sprite locations from `DrinkInfo`
   - Update `RenderDrinkIcon` to use sprite locations from `DrinkInfo`

7. Test Core Drink System
   - Test basic drink effects (Water, Orange Juice, Coffee)
   - Test scaling effects (Red Soda, Blue Soda, Watermelon Juice, Yellow Soda)
   - Test trade-off effects (Green Soda with negative Body)
   - Test team effects (White Wine, Red Wine with Next target)
   - Verify negative amounts work correctly in effect system
   - Test sprite rendering for all drinks
   - Test drink shop generation includes new drinks
   - Test drink application to dishes works correctly
   - Test multiple effects per drink (Red Wine, Green Soda)

### Phase 2: New Dish Types (Using Existing Operations)
1. Add Miso Soup, Tempura, Risotto to `DishType` enum
2. Implement dish info in `dish_types.cpp`
3. Use existing `OnCourseComplete`, `OnBiteTaken`, `TargetScope::Next`
4. Test scaling effects work correctly
5. Test new dishes balance against existing dishes

### Phase 3: New Effect Operations (High Priority)
**Note**: These operations benefit both drinks and new dishes. Implement together for maximum value.

1. Add `SwapStats` to `EffectOperation` enum (`src/components/dish_effect.h`)
2. Implement swap logic in `EffectResolutionSystem`
3. Add `MultiplyDamage` operation with temporary component
4. Implement damage multiplier in combat system
5. Add `PreventDefeat` operation
6. Hook into damage calculation to enforce minimum Body
7. Test stat swapping doesn't break combat calculations
8. Test damage multipliers apply correctly and are consumed
9. Test prevent defeat works at exactly 1 Body

### Phase 4: Advanced Dishes (Requires Phase 3)
1. Add Churros (uses `SwapStats`)
2. Add Bouillabaisse (uses `MultiplyDamage`)
3. Add Foie Gras (uses `PreventDefeat`)
4. Test new dishes with new operations
5. Test integration with drink pairings

### Phase 5: Advanced Mechanics (Medium Priority)
1. Implement `SummonDish` operation
2. Create battle entity spawning system
3. Add Pho dish with summoning
4. Implement `CopyEffect` operation
5. Add Paella dish
6. Test summoning creates valid battle entities
7. Test effect copying reads and applies correctly

### Phase 6: Complex Systems (Low Priority)
1. Implement status effect system
2. Add `ApplyStatus` operation
3. Create debuff components
4. Add Wagyu Steak with debuff
5. Test status effects apply and persist correctly

## Effect Implementation Details

**Effect Mapping Structure:**
- Each drink type maps to one or more `DishEffect` instances
- Effects are added to dish when it enters battle (InQueue phase)
- Effects use existing `EffectOperation` and `TargetScope` enums
- Effects stored as `DishEffect` components on dish entities

**Implementation Approach:**
- Directly add `DishEffect` components to dish entities when they enter battle
- This reuses existing infrastructure and is processed by `EffectResolutionSystem`

**Effect Examples:**
```cpp
// Coffee: OnStartBattle - Add +2 Combat Zing to Self
DishEffect coffee_effect(
    TriggerHook::OnStartBattle,
    EffectOperation::AddCombatZing,
    TargetScope::Self,
    2
);

// Orange Juice: OnServe - Add +1 Freshness
DishEffect orange_juice_effect(
    TriggerHook::OnServe,
    EffectOperation::AddFlavorStat,
    TargetScope::Self,
    1,
    FlavorStatType::Freshness
);

// White Wine: OnStartBattle - Add +1 Combat Zing to All Allies
DishEffect white_wine_effect(
    TriggerHook::OnStartBattle,
    EffectOperation::AddCombatZing,
    TargetScope::AllAllies,
    1
);

// Red Soda: OnCourseComplete - Add +1 Combat Zing to Self (scaling)
DishEffect red_soda_effect(
    TriggerHook::OnCourseComplete,
    EffectOperation::AddCombatZing,
    TargetScope::Self,
    1
);

// Green Soda: OnServe - Add +2 Combat Zing, then -1 Combat Body (trade-off)
// Note: Requires two separate DishEffect instances
DishEffect green_soda_zing(
    TriggerHook::OnServe,
    EffectOperation::AddCombatZing,
    TargetScope::Self,
    2
);
DishEffect green_soda_body(
    TriggerHook::OnServe,
    EffectOperation::AddCombatBody,
    TargetScope::Self,
    -1  // Negative amount for trade-off
);

// Red Wine: OnServe - Add +1 Richness to Self and Next Ally
DishEffect red_wine_self(
    TriggerHook::OnServe,
    EffectOperation::AddFlavorStat,
    TargetScope::Self,
    1,
    FlavorStatType::Richness
);
DishEffect red_wine_next(
    TriggerHook::OnServe,
    EffectOperation::AddFlavorStat,
    TargetScope::Next,
    1,
    FlavorStatType::Richness
);

// Yellow Soda: OnBiteTaken - Add +1 Combat Zing to Self (reactive scaling)
DishEffect yellow_soda_effect(
    TriggerHook::OnBiteTaken,
    EffectOperation::AddCombatZing,
    TargetScope::Self,
    1
);
```

## Files to Modify

### New Files:
- `src/systems/ApplyDrinkPairingEffects.h` - New system to apply drink effects

### Existing Files to Modify:
1. **`src/drink_types.h`** - Add new drink types to enum, add SpriteLocation to DrinkInfo
2. **`src/drink_types.cpp`** - Implement `get_drink_info()` for all drinks with sprite locations
3. **`src/dish_types.h`** - Add new dish types to enum (Miso Soup, Tempura, Risotto, Pho, Churros, Paella, Bouillabaisse, Foie Gras, Wagyu Steak)
4. **`src/dish_types.cpp`** - Implement new dish info
5. **`src/components/dish_effect.h`** - Add new `EffectOperation` values (SwapStats, MultiplyDamage, PreventDefeat, SummonDish, CopyEffect, ApplyStatus)
6. **`src/systems/EffectResolutionSystem.h`** - Implement new operations
7. **`src/systems/GenerateDrinkShop.h`** - Update to use sprite locations from DrinkInfo
8. **`src/systems/RenderDrinkIcon.h`** - Update to use sprite locations from DrinkInfo
9. **`src/systems/system_registry.cpp`** - Register new `ApplyDrinkPairingEffects` system
10. **`src/components/combat_stats.h`** - May need damage multiplier component
11. **`src/components/status_effects.h`** - New file for status effects (if Phase 6 implemented)

## Design Principles

1. **Thematic Consistency**: Drink and dish names and effects match real-world characteristics
2. **Strategic Depth**: Different options suit different strategies (body-focused, zing-focused, support, etc.)
3. **Price-to-Power**: Higher tier items provide more powerful or unique effects
4. **System Integration**: Uses existing effect system without requiring new infrastructure (for Phase 1-2)
5. **Flavor Synergy**: Many drinks boost flavor stats, creating synergy with dish flavor profiles
6. **Sprite Availability**: Prioritize items with existing sprites in the spritesheet
7. **SAP Inspiration**: Mechanics feel familiar to SAP players but adapted to food theme
8. **Coordination**: Drinks and dishes work together seamlessly

## Testing Requirements

### Drink Testing
- Test each drink type applies correct effect
- Test effects trigger at correct battle phase
- Test team effects (White Wine, Red Wine with Next target)
- Test scaling effects (Red Soda, Blue Soda, Watermelon Juice, Yellow Soda)
- Test trade-off effects (Green Soda with negative Body)
- Test negative amounts work correctly (verify system supports negative values)
- Test sprite rendering for all drinks
- Test drink shop generation includes new drinks
- Test drink application to dishes works correctly
- Test multiple effects per drink (Red Wine, Green Soda)

### Dish Testing
- Test new dishes balance against existing dishes
- Test scaling effects work correctly (Miso Soup, Tempura)
- Test team support effects (Risotto)

### Effect Operation Testing
- Test stat swapping doesn't break combat calculations
- Test damage multipliers apply correctly and are consumed
- Test prevent defeat works at exactly 1 Body
- Test summoning creates valid battle entities
- Test effect copying reads and applies correctly
- Test status effects apply and persist correctly

### Integration Testing
- Test new dishes work correctly with drink pairings
- Test drinks that use new effect operations (if implemented)
- Test combined effects (drink + dish effects)

## Future Considerations

- Drink combinations (applying multiple drinks to one dish)
- Drink tiers/levels (upgrading drinks)
- Seasonal/special drinks and dishes
- Shop refresh mechanics
- Visual indicators for applied drinks on dishes
- Additional drink/dish types when sprites become available
- Integration with new dish types (e.g., drinks that enhance summoned dishes)
- Drinks that grant new effect operations to dishes (e.g., a drink that grants `MultiplyDamage` effect)
- Multi-hit attacks (attack multiple times per turn)
- Effect stacking (multiple copies of same effect)
- Effect duration (temporary vs permanent)
- Effect removal (ways to remove enemy effects)
- Battle phase modifiers (effects that change battle flow)
- RandomTarget scope implementation for more variety

## Outstanding Questions
1. **Content Release Cadence:** Do we ship drinks and dishes in separate waves (drinks first), or bundle them to avoid meta imbalance?
2. **Economy Impact:** How do new drink prices interact with existing gold flow—do we need reroll/price adjustments before launch?
3. **Effect Operation Ordering:** Which new operations (SwapStats, MultiplyDamage, PreventDefeat, SummonDish, etc.) are mandatory for MVP vs can slip?
4. **Telemetry:** What adoption + win-rate metrics do design need to quickly rebalance new content?
5. **Art & Localization:** Are all sprites/names/strings approved, or do we need art/localization passes before implementation begins?

