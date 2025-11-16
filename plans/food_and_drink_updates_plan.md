# Food and Drink Updates Plan

## Overview
Design and implement new drink types and dish types with advanced effects. This plan covers:
- **Drink pairings**: Drinks applied to dishes that enhance them during battle
- **New dish types**: Advanced dishes with complex mechanics
- **New effect operations**: System enhancements to support advanced mechanics

**Relationship to Other Plans:**
- `DISH_EFFECTS_IMPLEMENTATION.md` focuses on implementing effects for existing dishes
- All plans share the same effect system infrastructure

**Design Principles:**
- **Simplicity First**: Prefer less code and simpler implementations when possible, while maintaining robustness
- **Single Drink Per Dish**: Each dish can only have one drink applied (stored in `DrinkPairing` component)
- **Drink Synergies**: Drinks can care about other drinks on the team (synergy effects), but not for Tier 1 drinks
- **System Robustness First**: Focus on making systems and effects robust and interoperable. The codebase should support all TargetScopes, EffectOperations, and combinations, even if not all are used immediately. Once the game is complete, we can audit and tune specific dish configurations. The goal is to avoid changing code later when we want to add new dishes with different effect combinations.

## Reference: All TargetScopes and EffectOperations

### TargetScopes

**Current (Implemented):**
- `Self` - The dish with the effect
- `Opponent` - The opposing dish in the same slot
- `AllAllies` - All dishes on same team, excluding the source dish
- `AllOpponents` - All dishes on opponent team
- `DishesAfterSelf` - Dishes that come after this dish in queue
- `FutureAllies` - Future dishes on same team
- `FutureOpponents` - Future dishes on opponent team
- `Previous` - Previous dish in queue
- `Next` - Next dish in queue (if no next ally exists, effect does nothing)
- `SelfAndAdjacent` - Self and adjacent dishes

**Planned (From Q18):**
- `RandomAlly` - Random dish from same team, excluding self
- `RandomOpponent` - Random dish from opponent team
- `RandomDish` - Random dish from all dishes, excluding self
- `RandomOtherAlly` - Random ally excluding self (explicit version)

**Future Considerations:**
- `AllDishes` - All dishes on same team, including the source dish
- `AllOtherAllies` - All dishes on same team, excluding the source dish (explicit version of AllAllies)

### EffectOperations

**Current (Implemented):**
- `AddFlavorStat` - Add to flavor stat (Freshness, Sweetness, Richness, etc.)
- `AddCombatZing` - Add to combat Zing (attack)
- `AddCombatBody` - Add to combat Body (health)

**Planned (High Priority):**
- `SwapStats` - Exchange Zing and Body values
- `MultiplyDamage` - Modify next damage (multiplier, flat modifier, count)
- `PreventAllDamage` - Prevent next damage entirely (uses NextDamageEffect with multiplier 0.0)

**Planned (Medium Priority):**
- `SummonDish` - Create temporary dish on trigger
- `CopyEffect` - Copy all effects from target(s) to source dish
- `ApplyStatus` - Apply debuff to opponent

### Effect × TargetScope Usage Matrix

This table shows which dishes and drinks use each combination of EffectOperation and TargetScope. **Note**: Existing Tier 1 dishes already use diverse TargetScopes (SelfAndAdjacent, DishesAfterSelf, Opponent, FutureAllies, AllAllies) - this is fine. We can audit dish configurations later. **Focus**: Make the codebase robust to support all combinations - specific dish configurations can be tuned after the game is complete.

| TargetScope →<br>EffectOperation ↓ | Self | Opponent | AllAllies | Next | FutureAllies | DishesAfterSelf | SelfAndAdjacent | RandomAlly | Other |
|-------------------------------------|------|----------|-----------|------|--------------|-----------------|-----------------|------------|-------|
| **AddFlavorStat** | Orange Juice (+1 Freshness)<br>Hot Cocoa (+1 Sweetness, +1 Richness)<br>Red Wine (+1 Richness)<br>Watermelon Juice (+1 Freshness)<br>Risotto (+1 Richness)<br>Potato L3 (+1 Satiety)<br>Bagel L2-3 (+1 Richness)<br>GarlicBread L2-3 (+1 Spice) | | | | Red Wine (+1 Richness)<br>Risotto (+1 Richness) | GarlicBread (+1 Spice) | Bagel (+1 Richness) | Salmon (+1 Freshness, conditional) | | |
| **AddCombatZing** | Coffee (+2)<br>Red Soda (+1 per course)<br>Green Soda (+2)<br>Yellow Soda (+1 per bite)<br>Tempura (+1 per bite)<br>FrenchFries L2-3 (+1-2) | | White Wine (+1)<br>FriedEgg L2-3 (on finish) | | FrenchFries (+1-3) | | | | |
| **AddCombatBody** | Blue Soda (+1 per course)<br>Green Soda (-1)<br>Watermelon Juice (+1 per course)<br>Miso Soup (+1 per course)<br>Salmon L2-3 (+1-2 on course start)<br>Baguette L2-3 (+1-2 on course start)<br>FriedEgg L2-3 (+1-2) | | FriedEgg (+2-6 on finish) | | | | | | |
| **RemoveCombatZing** | | Baguette (-1 to -3) | | | | | | | |
| **SwapStats** | Churros | | | | | | | | |
| **MultiplyDamage** | Bouillabaisse (2x next attack) | | | | | | | | |
| **PreventAllDamage** | Foie Gras | | | | | | | | |
| **SummonDish** | Pho (Broth) | | | | | | | | |
| **CopyEffect** | | | | | | | | Paella | |
| **ApplyStatus** | | Wagyu Steak (-1 Zing) | | | | | | | |

**Existing Tier 1 Dishes (already implemented):**
- **Salmon**: `AddFlavorStat` (SelfAndAdjacent, +1-3 Freshness, conditional chain)
- **Bagel**: `AddFlavorStat` (DishesAfterSelf, +1-3 Richness) + (Self, +1-2 Richness at L2-3)
- **Baguette**: `RemoveCombatZing` (Opponent, -1 to -3) + `AddCombatBody` (Self, +1-2 on course start at L2-3)
- **GarlicBread**: `AddFlavorStat` (FutureAllies, +1-3 Spice) + (Self, +1-2 Spice at L2-3)
- **FriedEgg**: `AddCombatBody` (AllAllies, +2-6 on finish) + (Self, +1-2 at L2-3)
- **FrenchFries**: `AddCombatZing` (FutureAllies, +1-3) + (Self, +1-2 at L2-3)
- **Potato**: `AddFlavorStat` (Self, +1 Satiety at L3 only)

**Suggested Additions for Tier 2+ Dishes (to fill empty spaces):**
- **Tier 2+ dishes currently have NO effects** - good opportunity to add diverse TargetScopes:
  - `AddCombatZing` (Opponent) - could debuff opponent
  - `AddCombatZing` (Next) - support adjacent ally
  - `AddCombatZing` (DishesAfterSelf) - buff dishes that come after
  - `AddCombatZing` (SelfAndAdjacent) - buff self and neighbors
  - `AddCombatBody` (Opponent) - unlikely, but possible
  - `AddCombatBody` (Next) - support adjacent ally
  - `AddCombatBody` (FutureAllies) - buff future dishes
  - `AddCombatBody` (DishesAfterSelf) - buff dishes that come after
  - `AddCombatBody` (SelfAndAdjacent) - buff self and neighbors
  - `AddFlavorStat` (Opponent) - debuff opponent flavor
  - `AddFlavorStat` (AllAllies) - team-wide flavor buff
  - `AddFlavorStat` (FutureAllies) - buff future dishes
  - `AddFlavorStat` (SelfAndAdjacent) - buff self and neighbors

**Notes:**
- **Green Soda** has two effects: `AddCombatZing` (Self, +2) and `AddCombatBody` (Self, -1)
- **Red Wine** has two effects: `AddFlavorStat` (Self, +1 Richness) and `AddFlavorStat` (Next, +1 Richness)
- **Watermelon Juice** has two effects: `AddFlavorStat` (Self, +1 Freshness) and `AddCombatBody` (Self, +1 per course)
- **Hot Cocoa** has two effects: `AddFlavorStat` (Self, +1 Sweetness) and `AddFlavorStat` (Self, +1 Richness)
- **Risotto** has two effects: `AddFlavorStat` (Self, +1 Richness) and `AddFlavorStat` (Next, +1 Richness)
- **Water** has no effect (baseline)
- **Tier 1 dishes** already use diverse TargetScopes (SelfAndAdjacent, DishesAfterSelf, Opponent, FutureAllies, AllAllies) - good variety!

**Design Mechanics**: This design incorporates various strategic mechanics, including:
- **Scaling effects**: Effects that trigger repeatedly over time (OnCourseComplete, OnBiteTaken)
- **Trade-off effects**: Strategic choices with negative stat modifications
- **Team support**: Buffing multiple allies or adjacent dishes
- **Reactive scaling**: Effects that trigger when taking damage
- **Protection effects**: Prevent defeat
- **Double damage**: Amplify next attack
- **Stat swapping**: Exchange stats
- **Summoning**: Create temporary dishes

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

### Battle Course System
A **course** is a single combat matchup between one player dish and one opponent dish at the same queue index (slot). Battle flow:
1. OnStartBattle - Fires once when battle begins
2. OnServe - Fires for all dishes before combat starts
3. OnCourseStart - Fires when a course begins (both dishes enter combat)
4. OnBiteTaken - Fires each time damage is dealt (alternating turns). **Clarification**: `OnBiteTaken` triggers when the dish with the effect takes damage (receives a bite), not when it deals damage.
5. OnDishFinished - Fires when a dish's Body reaches 0
6. OnCourseComplete - Fires when both dishes in a course are Finished

Battles can have multiple courses (one per slot). If a dish survives a course, it continues to the next course. `OnCourseComplete` fires once per course completion.

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
   - Use case: Strategic repositioning
   - Implementation: TODO - Need to determine which stats to swap (CombatStats Zing/Body), when it triggers, and how it affects combat calculations
   - Needed for: Churros (Tier 3 dish), potential advanced drinks

2. **`MultiplyDamage`** - Double/triple next attack(s)
   - Use case: Burst damage, multi-hit effects
   - Implementation: Add `NextDamageEffect` component system to support damage multipliers, flat modifiers, shields, and multi-use effects. Component stores: `float multiplier` (e.g., 2.0 for 2x damage, 0.5 for 50% damage, 0.0 for full block), `int flatModifier` (e.g., -5 for flat reduction, +3 for flat bonus), and `int count` (number of uses, default 1). In `ResolveCombatTickSystem`, apply multiplier first, then add flat modifier, then decrement count. Remove component when count reaches 0. Enables: "next attack deals 2x damage" (multiplier=2.0, count=1), "-5 damage for next 3 hits" (flatModifier=-5, count=3), "prevent next 3 damage" (multiplier=0.0, count=3), "50% damage reduction for 2 hits" (multiplier=0.5, count=2). System should be robust and support all effect types.
   - Needed for: Bouillabaisse (Tier 4 dish), potential advanced drinks

3. **`PreventAllDamage`** - Prevent next damage entirely (regardless of amount)
   - Use case: Protection from next attack(s) (shield/block)
   - Implementation: Uses `NextDamageEffect` component with multiplier 0.0 and count (default 1 for one-time, can be higher for multiple blocks). When effect triggers, add `NextDamageEffect` component with multiplier 0.0 to target. In `ResolveCombatTickSystem`, damage is multiplied by 0.0 (becomes 0), then count is decremented. Component is removed when count reaches 0. Works with existing `NextDamageEffect` system. Enables effects like "prevent next damage" (multiplier=0.0, count=1) or "prevent next 3 damage" (multiplier=0.0, count=3). System supports all effect types (multiplier, flat modifier, count combinations).
   - Needed for: Foie Gras (Tier 5 dish), potential advanced drinks

### Medium Priority (Requires New Systems)
4. **`SummonDish`** - Create temporary dish on trigger
   - Use case: Summoning temporary dishes
   - Implementation: TODO - Think about dish type for "Broth", battle entity spawning system, not needed for Tier 1
   - Needed for: Pho (Tier 3 dish) - **TODO: Defer to later phase, not for Tier 1**

5. **`CopyEffect`** - Copy effect from target dish
   - Use case: Mimic-style copying
   - Implementation: TODO - Think about which effects to copy (drink, dish, synergy), how to select random ally, effect persistence
   - Needed for: Paella (Tier 4 dish)

6. **`ApplyStatus`** - Apply debuff to opponent
   - Use case: Weakness, poison, etc.
   - Implementation: New status effect component system
   - Needed for: Wagyu Steak (Tier 5 dish)

7. **`RandomTarget`** - Target random allies/opponents
   - Use case: Random buffing
   - Implementation: TODO - Discuss enum variant vs parameter approach, RNG seeding strategy
   - Enables "give +X to 2 random allies" effects

### Low Priority (Complex Systems)
8. **`TransformDish`** - Temporarily change dish type
   - Use case: Transformation effects
   - Implementation: Complex state management

9. **`ShopModifier`** - Modify shop phase
   - Use case: Reduce costs, improve rolls
   - Implementation: New shop interaction system

10. **`GenerateGold`** - Economic effects
    - Use case: Resource generation
    - Implementation: Hook into shop/economy system
    - Outside combat scope, but could trigger OnStartBattle

**Implementation Priority Summary:**
- **High**: Negative amounts (already supported), Scaling effects (already supported via OnCourseComplete, OnBiteTaken)
- **Medium**: SwapStats, MultiplyDamage, PreventAllDamage, RandomTarget scope, SummonDish, CopyEffect, ApplyStatus
- **Low**: TransformDish, ShopModifier, GenerateGold

## Proposed Items by Tier

### Tier 1 (Basic - 1-2 gold)

#### Drinks

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

4. **Soda** (use red variant, column 5, row 5) - *Scaling effect*
   - Price: 2 gold
   - Effect: OnCourseComplete - Add +1 Combat Zing to Self (scales each course)
   - Theme: Bubbly, per-course boost that accumulates
   - Sprite: i=5, j=5
   - Note: Provides scaling value over time. Fires once per course completion.

#### Dishes
- No Tier 1 dishes in this plan

### Tier 2 (Moderate - 2-3 gold)

#### Drinks

5. **Hot Cocoa** (column 1, row 5)
   - Price: 2 gold
   - Effect: OnServe - Add +1 Sweetness and +1 Richness (flavor stats)
   - Theme: Warm, comforting, body-focused
   - Sprite: i=1, j=5

6. **Red Wine** (column 10, row 5) - *Team support*
   - Price: 3 gold
   - Effect: OnServe - Add +1 Richness to Self and +1 Richness to Next Ally
   - Theme: Sophisticated, body-focused, supports adjacent dishes
   - Sprite: i=10, j=5
   - Note: Uses `TargetScope::Next` for adjacent support. If no next ally exists, effect does nothing (no error, silent skip).

7. **Green Soda** (column 4, row 5) - *Trade-off effect*
   - Price: 2 gold
   - Effect: OnServe - Add +2 Combat Zing, Add -1 Combat Body to Self
   - Theme: Energy drink, glass cannon build
   - Sprite: i=4, j=5
   - Note: Offensive trade-off effect. Negative amounts are allowed and clamped to 0 (Body and Zing cannot go below zero). If a dish has 1 Body and gets -1 Body, it becomes 0 (defeated). This is intentional for trade-off effects.

8. **Blue Soda** (column 3, row 5) - *Scaling durability*
   - Price: 2 gold
   - Effect: OnCourseComplete - Add +1 Combat Body to Self
   - Theme: Refreshing, scales durability over time
   - Sprite: i=3, j=5
   - Note: Gradual health scaling over time. Fires once per course completion.

#### Dishes

**Miso Soup** (Tier 2)
- Effect: OnCourseComplete - Add +1 Body to Self (scaling durability)
- Theme: Nourishing, builds over time
- **Uses existing operations**: AddCombatBody

**Tempura** (Tier 2)
- Effect: OnBiteTaken - Add +1 Zing to Self (reactive scaling)
- Theme: Crispy, gets stronger when hit
- **Uses existing operations**: AddCombatZing
- Note: `OnBiteTaken` triggers when this dish takes damage (receives a bite), not when it deals damage.

### Tier 3 (Advanced - 3-4 gold)

#### Drinks

9. **White Wine** (column 11, row 5) - *Team-wide buff*
   - Price: 3 gold
   - Effect: OnStartBattle - Add +1 Combat Zing to All Allies
   - Theme: Celebratory, team boost
   - Sprite: i=11, j=5
   - Note: Team-wide support effect. Uses `TargetScope::AllAllies` which excludes the source dish.

10. **Watermelon Juice** (use column 8, row 5) - *Multi-stat scaling*
    - Price: 3 gold
    - Effect: OnCourseComplete - Add +1 Freshness and +1 Combat Body to Self
    - Theme: Refreshing, maximum freshness with scaling durability
    - Sprite: i=8, j=5
    - Note: Multi-stat scaling effect. Fires once per course completion.

11. **Yellow Soda** (column 6, row 5) - *Per-bite scaling*
    - Price: 3 gold
    - Effect: OnBiteTaken - Add +1 Combat Zing to Self (when this dish takes damage)
    - Theme: Adrenaline rush, scales with combat intensity
    - Sprite: i=6, j=5
    - Note: Reactive growth that scales with combat. `OnBiteTaken` triggers when this dish takes damage (receives a bite), not when it deals damage.

#### Dishes

**Risotto** (Tier 3)
- Effect: OnServe - Add +1 Richness to Self and Next Ally
- Theme: Creamy, supports adjacent dishes
- **Uses existing operations**: AddFlavorStat with TargetScope::Next
- Note: If no next ally exists, effect does nothing (no error, silent skip).

**Pho** (Tier 3)
- Effect: OnDishFinished - Summon 1/1 Broth (weak dish) to same slot
- Theme: Nourishing broth remains after main ingredient
- **Requires**: `SummonDish` operation (Medium priority) - **TODO: Defer to later phase, not for Tier 1**

**Churros** (Tier 3)
- Effect: OnServe - Swap Zing and Body values
- Theme: Sweet but fragile, strategic repositioning
- **Requires**: `SwapStats` operation (High priority)
- Note: Swap happens instantly when OnServe triggers, after all other effects (dish → drink → synergy) are applied. Directly swaps both baseZing/baseBody and currentZing/currentBody in CombatStats component.

### Tier 4 (Complex - 4-5 gold)

#### Drinks
- No Tier 4 drinks in this plan

#### Dishes

**Paella** (Tier 4)
- Effect: OnStartBattle - Copy effect from random ally
- Theme: Complex dish that adapts to team
- **Requires**: `CopyEffect` operation (Medium priority) - **TODO: Need to determine which effects to copy and selection logic**

**Bouillabaisse** (Tier 4)
- Effect: OnServe - Next attack deals 2x damage
- Theme: Rich, powerful first strike
- **Requires**: `MultiplyDamage` operation (High priority)
- Note: Uses `NextDamageEffect` component with multiplier 2.0, flatModifier 0, and count 1. Component count is decremented when damage is dealt, removed when count reaches 0. This robust system enables: shields (multiplier 0.0), multi-hit effects (count > 1), flat damage reduction (flatModifier < 0), and combinations of all types. Fun config of dish values and effects can be designed later.

### Tier 5 (Premium - 5+ gold)

#### Drinks
- No Tier 5 drinks in this plan

#### Dishes

**Foie Gras** (Tier 5)
- Effect: OnStartBattle - Prevent next damage entirely (one-time)
- Theme: Luxurious, protected
- **Requires**: `PreventAllDamage` operation (High priority)
- Note: Uses `NextDamageEffect` component with multiplier 0.0, flatModifier 0, and count 1. Prevents the next damage taken regardless of amount. Component count is decremented when damage is dealt (damage becomes 0), removed when count reaches 0. Can be extended to "prevent next 3 damage" (multiplier=0.0, count=3). Works with existing robust `NextDamageEffect` system that supports all effect types.

**Wagyu Steak** (Tier 5)
- Effect: OnBiteTaken - Apply -1 Zing to Opponent (debuff)
- Theme: Premium, weakens opponents
- **Requires**: `ApplyStatus` operation (Medium priority)

### Future Expansion (when sprites available)

#### Drinks
- Milk - Could be: OnServe - Add +1 Body to Self (nourishing)
- Tea - Could be: OnCourseStart - Add +1 Zing to Self (energizing)
- Energy Drink - Could be: OnStartBattle - Add +3 Zing, -2 Body (extreme trade-off)
- Smoothie - Could be: OnServe - Add +1 to all flavor stats (balanced boost)
- Honey - Could be: OnServe - Add +2 Sweetness (flavor-focused)
- Vinegar - Could be: OnServe - Add +2 Acidity, -1 Body (sour trade-off)
- Maple Syrup - Could be: OnServe - Add +3 Sweetness (sweet-focused)
- Lemonade - Could be: OnCourseComplete - Add +1 Freshness and +1 Zing (scaling refresh)

## System Execution Order

**CRITICAL: These systems run in a specific order. Order matters for correctness.**

During battle initialization, when dishes enter `InQueue` phase, the following systems run in this exact order:

1. **`ApplyPairingsAndClashesSystem`** - Computes pairing bonuses and clash penalties for dishes
   - Runs when: `dbs.phase == DishBattleState::Phase::InQueue`
   - Adds: `PairingClashModifiers` component
   - **MUST run first** to establish base modifiers

2. **`ApplyDrinkPairingEffects`** (NEW) - Applies drink effects to dishes
   - Runs when: `dbs.phase == DishBattleState::Phase::InQueue`
   - Adds: `DrinkEffects` component
   - **MUST run after** `ApplyPairingsAndClashesSystem`
   - **MUST run before** `OnServe` triggers (which happens in `SimplifiedOnServeSystem`)

3. **`SimplifiedOnServeSystem`** - Fires `OnServe` trigger for all dishes
   - Runs after dishes are initialized
   - Triggers: `TriggerHook::OnServe` events
   - Effects from drink effects (and dish effects) will fire here

**Registration Order in `battle_system_registry.cpp`:**
```cpp
systems.register_update_system(std::make_unique<ApplyPairingsAndClashesSystem>());
systems.register_update_system(std::make_unique<ApplyDrinkPairingEffects>());  // NEW - register immediately after
systems.register_update_system(std::make_unique<StartCourseSystem>());
// ... other systems ...
systems.register_update_system(std::make_unique<SimplifiedOnServeSystem>());
```

**Why order matters:**
- Pairings/clashes establish base combat modifiers
- Drink effects may depend on or interact with these modifiers
- All effects must be ready before `OnServe` triggers fire
- Deterministic order ensures consistent, testable behavior

## Implementation Strategy

### Phase 0: SpriteLocation Refactoring (Separate Commit)
**CRITICAL: This must be done in a separate commit, tested separately, before Phase 1**

1. Create shared `SpriteLocation` struct (or use existing one from `dish_types.h`)
2. Update `DrinkInfo` to use `SpriteLocation` instead of nested struct
3. Update `DishInfo` to use same `SpriteLocation` format (if not already)
4. Update all systems that use sprite locations to use shared struct
5. Test sprite rendering for all existing drinks and dishes
6. Verify no regressions in sprite display

**Rationale**: Prefer less code by sharing sprite location structure across all classes. This should be done separately to ensure it's tested and doesn't break existing functionality.

### Phase 1: Core Drink System (Can be done independently)

**Design Decisions:**
- **Effect Storage**: Create separate `DrinkEffects` component (similar to `SynergyBonusEffects`, stores `std::vector<DishEffect>`) for drink effects. This provides clear separation and deterministic ordering.
- **Effect Resolution Order**: Deterministic order: **dish effects → drink effects → synergy effects**. Update `EffectResolutionSystem::process_trigger_event` to process: 1) dish effects from `get_dish_info().effects`, 2) drink effects from `DrinkEffects` component, 3) synergy effects from `SynergyBonusEffects` component. **Order matters** because effects are applied sequentially - a dish with 1 Body getting -1 Body (drink effect) goes to 0, then if it gets +2 Body (synergy effect), it goes to 2.
- **System Timing**: Run `ApplyDrinkPairingEffects` **after** `ApplyPairingsAndClashesSystem` in system registry. Both check `dbs.phase == DishBattleState::Phase::InQueue` and run once per dish. See "System Execution Order" section below for details.
- **Error Handling**: Use `log_error()` for missing drink effect mappings, null `DrinkPairing.drink`, or invalid drink types. Fail fast with clear error messages.
1. Extend `DrinkType` enum (`src/drink_types.h`)
   - Add new drink types: Coffee, OrangeJuice, Soda (Red), GreenSoda, BlueSoda, YellowSoda, HotCocoa, RedWine, WhiteWine, WatermelonJuice
   - Total: 11 drink types (including Water)

2. Update `DrinkInfo` structure (`src/drink_types.h`)
   - Use `SpriteLocation` struct (from Phase 0)
   - Update `get_drink_info()` to return sprite locations for each drink

3. Implement drink info (`src/drink_types.cpp`)
   - Implement `get_drink_info()` for all new drinks with name, price, sprite location
   - Add error handling: `log_error()` if drink type not found

4. Create Drink Effect Component (`src/components/drink_effects.h`)
   - New component similar to `SynergyBonusEffects`
   - Stores `std::vector<DishEffect>` for drink effects
   - Reuses existing infrastructure pattern

5. Create Drink Effect System (`src/systems/ApplyDrinkPairingEffects.h`)
   - New system that processes `DrinkPairing` components
   - Converts drink pairings into `DrinkEffects` component (stores vector of `DishEffect`)
   - Runs during battle initialization (similar to `ApplyPairingsAndClashesSystem`)
   - Maps drink types to their corresponding effects
   - Runs when: `dbs.phase == DishBattleState::Phase::InQueue` (same condition as `ApplyPairingsAndClashesSystem`)
   - Runs each battle start - effects are reapplied fresh each battle (battles are fully separate)
   - Only runs once per dish per battle (check if `DrinkEffects` already exists to prevent duplicates)
   - Error handling: `log_error()` if drink type has no effect mapping or `DrinkPairing.drink` is null
   - **CRITICAL**: Register in `battle_system_registry.cpp` immediately after `ApplyPairingsAndClashesSystem` (see System Execution Order section)

6. Integrate with Effect Resolution
   - Update `EffectResolutionSystem::process_trigger_event` to process effects in deterministic order:
     1. Dish effects from `get_dish_info().effects` (existing)
     2. Drink effects from `DrinkEffects` component (NEW)
     3. Synergy effects from `SynergyBonusEffects` component (existing)
   - Effects will trigger at appropriate `TriggerHook` points automatically
   - Order is deterministic and testable
   - **Note**: For `SwapStats` operation, swap happens **after** all other effects in this order are applied, then stats are swapped directly in `CombatStats` component

7. Update Shop Integration
   - Update `GenerateDrinkShop` to use sprite locations from `DrinkInfo`
   - Update `RenderDrinkIcon` to use sprite locations from `DrinkInfo`
   - **Shop Generation Logic**: For now, only generate Tier 1 drinks. TODO: Match drink tier to dish tier in shop (defer to later)

8. Test Core Drink System
   - **CRITICAL**: Tests must use production code only - no test-specific code paths
   - Test setup: Only affect what shows up in store (set up initial shop state)
   - Test execution: Buy drinks from store using production code (`app.purchase_drink()` or similar)
   - Test execution: Apply drinks to dishes using production code (HandleDrinkDrop system)
   - Test execution: Run battles using production code
   - Test verification: Verify effects work using production systems
   - Test basic drink effects (Water, Orange Juice, Coffee)
   - Test scaling effects (Red Soda, Blue Soda, Watermelon Juice, Yellow Soda) - verify fires once per course
   - Test trade-off effects (Green Soda with negative Body)
   - Test team effects (White Wine with AllAllies, Red Wine with Next target)
   - Test edge cases: Next target when no next ally exists (should do nothing, no error)
   - Verify negative amounts work correctly in effect system
   - Test negative amounts: Body and Zing are clamped at 0. Test dish with 1 Body + Green Soda (-1 Body) = 0 Body (defeated). Test Body at 0 + positive effect = correct value.
   - Test sprite rendering for all drinks
   - Test drink shop generation includes new drinks (Tier 1 only for now)
   - Test drink application to dishes works correctly
   - Test multiple effects per drink (Red Wine, Green Soda)
   - Test OnBiteTaken triggers when dish takes damage (not when it deals damage)
   - Test integration with existing dish effects - ensure deterministic order

### Phase 2: New Dish Types (Using Existing Operations)
1. Add Miso Soup, Tempura, Risotto to `DishType` enum
2. Implement dish info in `dish_types.cpp`
3. Use existing `OnCourseComplete`, `OnBiteTaken`, `TargetScope::Next`
4. Test scaling effects work correctly
5. Test new dishes balance against existing dishes

### Phase 3: New Effect Operations (High Priority)
**Note**: These operations benefit both drinks and new dishes. Implement together for maximum value.

1. Add `SwapStats` to `EffectOperation` enum (`src/components/dish_effect.h`)
2. Implement swap logic in `EffectResolutionSystem::apply_to_target()`
   - When `EffectOperation::SwapStats` is processed, directly swap values in target's `CombatStats` component
   - Swap both `baseZing`/`baseBody` and `currentZing`/`currentBody`
   - Swap is instant when effect triggers (after all other effects in resolution order are applied)
   - If trigger is `OnBiteTaken`, swap happens every time `OnBiteTaken` fires
3. Add `MultiplyDamage` operation to `EffectOperation` enum
4. Create `NextDamageEffect` component (`src/components/next_damage_effect.h`)
   - Stores `float multiplier` (e.g., 2.0 for 2x damage, 0.5 for 50% damage, 0.0 for full block, 1.0 for no multiplier)
   - Stores `int flatModifier` (e.g., -5 for flat reduction, +3 for flat bonus, 0 for no flat modifier)
   - Stores `int count` (number of uses, default 1)
   - Enables multi-use effects: "next 3 hits deal 2x damage" (multiplier=2.0, count=3), "-5 damage for next 3 hits" (flatModifier=-5, count=3), "prevent next 3 damage" (multiplier=0.0, count=3), "50% damage reduction for 2 hits" (multiplier=0.5, count=2)
   - System should be robust and support all effect type combinations
   - Can support shields (multiplier 0.0 for full block, or fractional for partial reduction)
5. Implement damage effect in `ResolveCombatTickSystem`
   - Before applying damage, check if target has `NextDamageEffect`
   - If present, **apply multiplier FIRST**: `damage *= multiplier`
   - **Then apply flat modifier**: `damage += flatModifier`
   - **Then clamp to minimum 0**: `damage = std::max(0, damage)` (cannot go negative)
   - Decrement count after applying damage
   - Remove component when count reaches 0 (consumed after all uses)
   - Works for both player and opponent attacks
   - **Calculation order is critical and must be documented**: multiplier → flat modifier → clamp. Example: damage=20, multiplier=0.5, flatModifier=-10 → (20 * 0.5) - 10 = 0 (not (20 - 10) * 0.5 = 5)
   - System supports all effect type combinations (multiplier only, flat modifier only, or both)
6. Add `PreventAllDamage` operation to `EffectOperation` enum
7. Implement `PreventAllDamage` in `EffectResolutionSystem::apply_to_target()`
   - When `EffectOperation::PreventAllDamage` is processed, add `NextDamageEffect` component with multiplier 0.0 and count (default 1, can be specified in effect)
   - Uses existing `NextDamageEffect` system - no separate implementation needed
   - Component count is decremented when damage is dealt (damage becomes 0), removed when count reaches 0
8. Test stat swapping doesn't break combat calculations
9. Test damage multipliers apply correctly and are consumed
10. Test prevent all damage works (damage becomes 0, component consumed)

### Phase 4: Advanced Dishes (Requires Phase 3)
1. Add Churros (uses `SwapStats`)
2. Add Bouillabaisse (uses `MultiplyDamage`)
3. Add Foie Gras (uses `PreventAllDamage`)
4. Test new dishes with new operations
5. Test integration with drink pairings

### Phase 5: Advanced Mechanics (Medium Priority)
1. Implement `SummonDish` operation
   - TODO: Think about dish type for "Broth", battle entity spawning system
2. Create battle entity spawning system
3. Add Pho dish with summoning
4. Implement `CopyEffect` operation
   - In `EffectResolutionSystem`, when processing `EffectOperation::CopyEffect`:
     - Validate that TargetScope results in exactly one target (log_error and skip if multiple targets like AllAllies, AllOpponents)
     - Collect all effects from that single target: 1) dish effects from `get_dish_info().effects`, 2) drink effects from `DrinkEffects` component, 3) synergy effects from `SynergyBonusEffects` component
     - Store all collected effects on the source dish (add to `DrinkEffects` component or create separate component for copied effects)
     - Copied effects trigger at their normal trigger hooks - they are stored and will fire when their trigger conditions are met
     - If CopyEffect fires after a trigger has already passed (e.g., OnServe), those copied effects won't trigger until next battle (this is fine)
     - System should be robust to support all single-target scopes (Self, Opponent, Next, Previous, RandomAlly, etc.)
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
- Effects stored in `DrinkEffects` component (vector of `DishEffect`) on dish entities
- **Effects are reapplied fresh each battle** - battles are fully separate, no side effects except health and wins
- Same team will have same stats each battle (barring permanent effects)

**Implementation Approach:**
- Create separate `DrinkEffects` component (similar to `SynergyBonusEffects`, stores `std::vector<DishEffect>`)
- `EffectResolutionSystem` processes effects in deterministic order: dish → drink → synergy
- This approach provides clear separation and deterministic ordering

**Target Scope Clarifications:**
- `TargetScope::AllAllies` - All dishes on same team, excluding the source dish
- `TargetScope::AllDishes` - All dishes on same team, including the source dish (TODO: Add if needed for clarity)
- `TargetScope::AllOtherAllies` - All dishes on same team, excluding the source dish (same as AllAllies, but more explicit - TODO: Consider adding)
- `TargetScope::Next` - Next dish in queue. If no next ally exists, effect does nothing (no error, silent skip)

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

// White Wine: OnStartBattle - Add +1 Combat Zing to All Allies (excludes self)
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
// Triggers when this dish takes damage
DishEffect yellow_soda_effect(
    TriggerHook::OnBiteTaken,
    EffectOperation::AddCombatZing,
    TargetScope::Self,
    1
);
```

## Files to Modify

### New Files:
- `src/components/drink_effects.h` - New component to store drink effects (similar to SynergyBonusEffects)
- `src/systems/ApplyDrinkPairingEffects.h` - New system to apply drink effects

### Existing Files to Modify:
1. **`src/drink_types.h`** - Add new drink types to enum, add SpriteLocation to DrinkInfo
2. **`src/drink_types.cpp`** - Implement `get_drink_info()` for all drinks with sprite locations
3. **`src/dish_types.h`** - Add new dish types to enum (Miso Soup, Tempura, Risotto, Pho, Churros, Paella, Bouillabaisse, Foie Gras, Wagyu Steak)
4. **`src/dish_types.cpp`** - Implement new dish info
5. **`src/components/dish_effect.h`** - Add new `EffectOperation` values (SwapStats, MultiplyDamage, PreventAllDamage, SummonDish, CopyEffect, ApplyStatus)
6. **`src/systems/EffectResolutionSystem.h`** - Update `process_trigger_event` to process drink effects in deterministic order (dish → drink → synergy), implement new operations (SwapStats, MultiplyDamage, PreventAllDamage)
7. **`src/systems/ResolveCombatTickSystem.h`** - Check for `NextDamageEffect` component before applying damage, multiply damage by multiplier, consume component
8. **`src/systems/GenerateDrinkShop.h`** - Update to use sprite locations from DrinkInfo
9. **`src/systems/RenderDrinkIcon.h`** - Update to use sprite locations from DrinkInfo
10. **`src/systems/system_registry.cpp`** - Register new `ApplyDrinkPairingEffects` system **immediately after** `ApplyPairingsAndClashesSystem` (order matters - see System Execution Order section)
11. **`src/components/next_damage_effect.h`** - New component for next damage effects (multipliers, shields, one-time effects)
12. **`src/components/status_effects.h`** - New file for status effects (if Phase 6 implemented)

## Design Principles

1. **Simplicity First**: Prefer less code and simpler implementations when possible, while maintaining robustness
2. **Thematic Consistency**: Drink and dish names and effects match real-world characteristics
3. **Strategic Depth**: Different options suit different strategies (body-focused, zing-focused, support, etc.)
4. **Price-to-Power**: Higher tier items provide more powerful or unique effects
5. **System Integration**: Uses existing effect system without requiring new infrastructure (for Phase 1-2)
6. **Flavor Synergy**: Many drinks boost flavor stats, creating synergy with dish flavor profiles
7. **Sprite Availability**: Prioritize items with existing sprites in the spritesheet
8. **Coordination**: Drinks and dishes work together seamlessly
9. **Error Handling**: Fail fast with `log_error()` for invalid states
10. **Deterministic Behavior**: Effect resolution order must be deterministic for reliable testing

## Testing Requirements

### Drink Testing
- Test each drink type applies correct effect
- Test effects trigger at correct battle phase
- Test team effects (White Wine with AllAllies, Red Wine with Next target)
- Test scaling effects (Red Soda, Blue Soda, Watermelon Juice, Yellow Soda) - verify fires once per course
- Test trade-off effects (Green Soda with negative Body)
- Test negative amounts work correctly (verify system supports negative values)
- TODO: Validate negative amounts don't break combat (e.g., Body going to 0 or negative)
- Test edge cases: Next target when no next ally exists (should do nothing, no error)
- Test sprite rendering for all drinks
- Test drink shop generation includes new drinks (Tier 1 only for now)
- Test drink application to dishes works correctly
- Test multiple effects per drink (Red Wine, Green Soda)
- Test OnBiteTaken triggers when dish takes damage (not when it deals damage)
- Test integration with existing dish effects - ensure deterministic order

### Dish Testing
- Test new dishes balance against existing dishes
- Test scaling effects work correctly (Miso Soup, Tempura)
- Test team support effects (Risotto)
- Test OnBiteTaken triggers correctly (when dish takes damage)

### Effect Operation Testing
- Test stat swapping doesn't break combat calculations
- Test damage multipliers apply correctly and are consumed
- Test prevent all damage works (damage becomes 0, component consumed)
- Test summoning creates valid battle entities
- Test effect copying reads and applies correctly
- Test status effects apply and persist correctly

### Integration Testing
- Test new dishes work correctly with drink pairings
- Test drinks that use new effect operations (if implemented)
- Test combined effects (drink + dish effects) - ensure deterministic resolution order
- Test effect resolution order is consistent across multiple runs

## Questions and Answers

This section documents questions raised during plan review and their answers:

### Q1: How should drink effects be stored on entities?
**A**: Use `SynergyBonusEffects` component pattern (stores `std::vector<DishEffect>`) to reuse existing infrastructure. This minimizes new code while being robust. `EffectResolutionSystem` already processes this component.

### Q2: When does OnBiteTaken trigger?
**A**: `OnBiteTaken` triggers when the dish with the effect takes damage (receives a bite), not when it deals damage. This applies to Yellow Soda and Tempura.

### Q3: What order should effects resolve in?
**A**: Deterministic order: **dish effects → drink effects → synergy effects**. Create a separate `DrinkEffects` component (similar to `SynergyBonusEffects`) to store drink effects. Update `EffectResolutionSystem::process_trigger_event` to process effects in this order: 1) dish effects from `get_dish_info().effects`, 2) drink effects from `DrinkEffects` component, 3) synergy effects from `SynergyBonusEffects` component. This ensures deterministic, testable behavior.

### Q4: Can a dish have multiple drinks?
**A**: No, each dish can only have one drink applied (stored in `DrinkPairing` component as `std::optional<DrinkType>`). However, drinks can care about other drinks on the team (synergy effects), but not for Tier 1 drinks.

### Q5: How should SwapStats work?
**A**: **Swap directly** - modify `CombatStats` component directly (swap both `baseZing`/`baseBody` and `currentZing`/`currentBody`). Swap is **instant** when the effect triggers. If effect says `OnBiteTaken`, swap every time `OnBiteTaken` runs. Example: if "OnPurchase swap stats", it would swap in the store. In battle, swap runs **after** all other effects in resolution order (DishEffect → DrinkEffect → SynergyEffect), so other effects are applied first, then stats are swapped. Implementation: In `EffectResolutionSystem::apply_to_target()`, when `EffectOperation::SwapStats` is processed, directly swap the values in the target's `CombatStats` component.

### Q6: How should MultiplyDamage be implemented?
**A**: Add **`NextDamageEffect` component system** to support damage multipliers, flat modifiers, shields, and multi-use effects. Component stores: `float multiplier` (e.g., 2.0 for 2x damage, 0.5 for 50% damage, 0.0 for full block), `int flatModifier` (e.g., -5 for flat reduction, +3 for flat bonus), and `int count` (number of uses, default 1). In `ResolveCombatTickSystem`, before applying damage, check if target has `NextDamageEffect`. If present, **apply multiplier FIRST** (damage *= multiplier), **then add flat modifier** (damage += flatModifier), **then clamp to minimum 0** (damage cannot go negative), then decrement count. Remove component when count reaches 0. **Calculation order is critical**: multiplier first, then flat modifier, then clamp. Example: damage=20, multiplier=0.5, flatModifier=-10 → (20 * 0.5) - 10 = 0 (not (20 - 10) * 0.5 = 5). This enables: "next attack deals 2x damage" (multiplier=2.0, count=1), "-5 damage for next 3 hits" (flatModifier=-5, count=3), "prevent next 3 damage" (multiplier=0.0, count=3), "50% damage reduction for 2 hits" (multiplier=0.5, count=2). System should be robust and support all effect types - fun config of dish values and effects can be designed later. Consider also `NextAttackEffect` for one-time attacks if needed.

### Q7: How should PreventAllDamage be implemented?
**A**: Use **`NextDamageEffect` component system** with multiplier 0.0, flatModifier 0, and count (default 1 for one-time, can be higher). When `PreventAllDamage` effect triggers, add `NextDamageEffect` component with multiplier 0.0 and count to target. In `ResolveCombatTickSystem`, damage is multiplied by 0.0 (becomes 0), then count is decremented. Component is removed when count reaches 0. This prevents damage entirely regardless of amount. Enables "prevent next damage" (multiplier=0.0, count=1) or "prevent next 3 damage" (multiplier=0.0, count=3). Works seamlessly with existing robust `NextDamageEffect` system that supports all effect types - no separate implementation needed, just use multiplier 0.0.

### Q8: What dish type should SummonDish create for Pho?
**A**: TODO - Think about dish type for "Broth", battle entity spawning system. Defer to later phase, not needed for Tier 1.

### Q9: How should drink shop generation work?
**A**: For now, only generate Tier 1 drinks. TODO: Match drink tier to dish tier in shop (defer to later).

### Q10: Should we use a shared SpriteLocation struct?
**A**: Yes, prefer less code by sharing sprite location structure across all classes. This should be done in a separate commit (Phase 0), tested separately, before Phase 1.

### Q11: When should ApplyDrinkPairingEffects run?
**A**: Run **after** `ApplyPairingsAndClashesSystem` in the system registry. Both systems check `dbs.phase == DishBattleState::Phase::InQueue` and run once per dish. Register `ApplyDrinkPairingEffects` immediately after `ApplyPairingsAndClashesSystem` in `battle_system_registry.cpp` to ensure pairings/clashes are computed first, then drink effects are applied. This ensures effects are ready before OnServe triggers.

### Q12: How should negative amounts be validated?
**A**: **Body and Zing cannot go below zero** - they are clamped at 0. Negative amounts in effects are allowed and will be clamped. Examples: Body at 3 taking 4 damage → 0 (not -1). Body at 0 doubled → 0 (0 * 2 = 0). Body at 0 + 2 → 2. The deterministic effect resolution order (dish → drink → synergy) matters because effects are applied sequentially, so a dish with 1 Body getting -1 Body from Green Soda will go to 0, and if it later gets +2 Body, it will go to 2. The system already handles this with `std::max(0, ...)` clamping in `ComputeCombatStatsSystem`.

### Q13: What does AllAllies target scope mean?
**A**: `TargetScope::AllAllies` targets all dishes on the same team, excluding the source dish. Consider adding `AllDishes` (includes self) and `AllOtherAllies` (explicit version) for clarity.

### Q14: What happens if Next target doesn't exist?
**A**: If no next ally exists, the effect does nothing (no error, silent skip). This applies to Red Wine and Risotto.

### Q15: What is a course?
**A**: A course is a single combat matchup between one player dish and one opponent dish at the same queue index (slot). `OnCourseComplete` fires once per course completion. Battles can have multiple courses.

### Q16: How long do drink effects persist?
**A**: Effects are **reapplied fresh each battle start** when dishes enter InQueue phase. Battles are fully separate with no side effects except health and number of wins. Same team should have same stats each battle (barring permanent effects). The `ApplyDrinkPairingEffects` system runs each battle and applies effects once per battle (checks if `DrinkEffects` already exists to prevent duplicates). Effects persist for the duration of that battle only - they do not carry over to the next battle.

### Q17: How should CopyEffect work?
**A**: `CopyEffect` is an `EffectOperation` that supports **all `TargetScope` values** (Self, Opponent, Next, AllAllies, etc.). When `CopyEffect` triggers, it **pretends to be that dish** - it copies **all effects** from the target specified by its `TargetScope` (dish effects + drink effects + synergy effects, from any/all sources). The copied effects are **always applied to the source dish** (the one with the `CopyEffect`). **Important constraints**: 1) CopyEffect with multiple targets (e.g., `AllAllies`, `AllOpponents`) is **not valid** - use `log_error()` and block/ignore the effect if TargetScope would result in multiple targets. For now, only single-target scopes are supported (Self, Opponent, Next, Previous, RandomAlly, etc.). 2) Copied effects trigger at their **normal trigger hooks** - they are stored on the source dish and will fire when their trigger conditions are met. If `CopyEffect` fires after `OnServe` has already passed, any copied `OnServe` effects will not trigger (this is fine, they'll trigger in the next battle). Implementation: In `EffectResolutionSystem`, when processing `EffectOperation::CopyEffect`, validate that TargetScope results in exactly one target (log_error and skip if multiple). Then collect all effects from that target: 1) dish effects from `get_dish_info().effects`, 2) drink effects from `DrinkEffects` component, 3) synergy effects from `SynergyBonusEffects` component. Store all collected effects on the source dish (add to `DrinkEffects` or create a new component for copied effects). The system should be robust and support copying from any single target so we can configure the most fun way and experiment. Focus on making the codebase support all options - specific dish configurations can be tuned later.

### Q18: How should RandomTarget scope be implemented?
**A**: Use **separate `TargetScope` enum values for different random pools**. Add enum variants like: `RandomAlly` (random dish from same team, excluding self), `RandomOpponent` (random dish from opponent team), `RandomDish` (random dish from all dishes, excluding self), `RandomOtherAlly` (random ally excluding self, explicit version). When processing these scopes, use seeded RNG for deterministic behavior in tests. Implementation: In `EffectResolutionSystem::get_targets()`, when encountering `TargetScope::RandomAlly`, select **exactly one random dish** from allies (excluding source). For `RandomOpponent`, select one random dish from opponents. For `RandomDish`, select one random dish from all dishes (excluding source). Use battle RNG seed to ensure determinism. If no valid target exists (e.g., no allies for `RandomAlly`), effect does nothing (silent skip, same as `Next` when no next ally exists). **TODO**: Consider if selecting multiple random targets is valuable (e.g., "give +X to 2 random allies") - defer to later, focus on robust single-target implementation first.

### Q19: What testing approach should we use?
**A**: **Tests must use production code only** - no test-specific code paths. Tests should: 1) Only affect what shows up in the store (set up initial shop state), 2) Buy items from the store using production shop code (`app.purchase_item()`, `app.purchase_drink()`), 3) Apply drinks to dishes using production code (HandleDrinkDrop system), 4) Run battles using production code, 5) Verify effects work using production systems. Follow existing patterns in `ValidateEffectSystemTest.h` and `ValidateShopPurchaseTest.h`. If tests use different code than production, there's no point to testing - must validate the same code paths players use.

### Q20: Should we test integration with existing dish effects?
**A**: Yes, test with existing dish effects to ensure deterministic order and no conflicts.

### Q21: How should we handle errors?
**A**: Use `log_error()` for missing drink effect mappings, null `DrinkPairing.drink`, or invalid drink types. Fail fast with clear error messages.

### Q22: Should we add visual feedback for applied drinks?
**A**: TODO for later - visual indicators for applied drinks (similar to freshness animation). Don't worry about it for now, just write it down.

## TODOs and Open Questions

### Implementation TODOs
- [ ] **SpriteLocation Refactoring**: Create shared SpriteLocation struct, separate commit, tested separately (Phase 0)
- [x] **Effect Resolution Order**: Deterministic order: dish → drink → synergy. Use separate `DrinkEffects` component, update `EffectResolutionSystem` to process in this order.
- [x] **ApplyDrinkPairingEffects Timing**: Run after `ApplyPairingsAndClashesSystem` in system registry. Both check `InQueue` phase. Register immediately after pairings system.
- [x] **Negative Amounts Validation**: Body and Zing clamped at 0. Negative amounts allowed, clamped at application. Order matters (dish → drink → synergy) because effects apply sequentially. System already handles with `std::max(0, ...)` in `ComputeCombatStatsSystem`.
- [ ] **Shop Tier Matching**: Support matching drink tier to dish tier in shop (currently Tier 1 only)
- [x] **SwapStats Implementation**: Swap directly in `CombatStats` component (both base and current). Swap is instant when effect triggers, after all other effects (dish → drink → synergy) are applied. If trigger is `OnBiteTaken`, swap every time it fires. Implement in `EffectResolutionSystem::apply_to_target()`.
- [x] **MultiplyDamage Component Design**: Use `NextDamageEffect` component system. Stores multiplier (e.g., 2.0 for 2x damage), flatModifier (e.g., -5 for flat reduction), and count (number of uses, default 1). Check in `ResolveCombatTickSystem` before applying damage: apply multiplier first (damage *= multiplier), then flat modifier (damage += flatModifier), clamp to 0, decrement count, remove when count reaches 0. System should be robust and support all effect types (multiplier only, flat modifier only, or both). Enables: "next 3 hits deal 2x damage" (multiplier=2.0, count=3), "-5 damage for next 3 hits" (flatModifier=-5, count=3), "prevent next 3 damage" (multiplier=0.0, count=3), "50% damage reduction for 2 hits" (multiplier=0.5, count=2). Fun config of dish values and effects can be designed later. Consider `NextAttackEffect` for one-time attacks if needed.
- [x] **PreventAllDamage Implementation**: Use `NextDamageEffect` component with multiplier 0.0, flatModifier 0, and count (default 1, can be higher). When `PreventAllDamage` effect triggers, add `NextDamageEffect` with multiplier 0.0 and count. Decrement count when damage is dealt, remove when count reaches 0. Enables "prevent next damage" (multiplier=0.0, count=1) or "prevent next 3 damage" (multiplier=0.0, count=3). Works with existing robust system - no separate implementation needed.
- [ ] **SummonDish Details**: Think about dish type for "Broth", battle entity spawning system (defer to later phase)
- [x] **CopyEffect Logic**: Copy all effects (dish + drink + synergy) from single target. Copied effects trigger at their normal trigger hooks. Multiple targets (AllAllies, AllOpponents) are invalid - log_error and block. System should be robust to support all single-target scopes (Self, Opponent, Next, RandomAlly, etc.). Focus on making codebase support all options - specific dish configurations can be tuned later.
- [x] **RandomTarget Scope**: Use separate enum values (RandomAlly, RandomOpponent, RandomDish, RandomOtherAlly). Select exactly one random target using seeded RNG for determinism. If no valid target exists, effect does nothing (silent skip). TODO: Consider if selecting multiple random targets is valuable - defer to later, focus on robust single-target implementation first.
- [x] **Testing Approach**: Tests must use production code only - no test-specific code paths. Tests only affect initial shop state, then buy items/apply drinks/run battles using production code. Follow patterns in `ValidateEffectSystemTest.h` and `ValidateShopPurchaseTest.h`. Must validate same code paths players use.
- [x] **Drink Effect Persistence**: Effects reapplied fresh each battle start when dishes enter InQueue. Battles are fully separate - no side effects except health/wins. Same team has same stats each battle (barring permanent effects). System checks if `DrinkEffects` exists to prevent duplicates within a battle.
- [ ] **Visual Feedback**: TODO for later - visual indicators for applied drinks (similar to freshness animation)

### Design Discussions Needed
- Effect resolution order and determinism
- SwapStats implementation details
- MultiplyDamage component design
- PreventAllDamage implementation (uses NextDamageEffect with multiplier 0.0)
- CopyEffect selection and persistence logic
- RandomTarget scope implementation
- Negative amounts validation and bounds

## Future Considerations

- Drink combinations (applying multiple drinks to one dish) - **Not supported, single drink per dish only**
- Drink synergies (drinks that care about other drinks on team) - **Supported, but not for Tier 1**
- Drink tiers/levels (upgrading drinks)
- Seasonal/special drinks and dishes
- Shop refresh mechanics
- Visual indicators for applied drinks on dishes - **TODO for later**
- Additional drink/dish types when sprites become available
- Integration with new dish types (e.g., drinks that enhance summoned dishes)
- Drinks that grant new effect operations to dishes (e.g., a drink that grants `MultiplyDamage` effect)
- Multi-hit attacks (attack multiple times per turn)
- Effect stacking (multiple copies of same effect)
- Effect duration (temporary vs permanent)
- Effect removal (ways to remove enemy effects)
- Battle phase modifiers (effects that change battle flow)
- RandomTarget scope implementation for more variety
- New TargetScope variants: AllDishes (includes self), AllOtherAllies (explicit version of AllAllies)

