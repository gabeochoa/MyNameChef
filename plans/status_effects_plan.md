# Status Effects System Plan

## Overview

This plan details implementing a full status effects system. **Note**: Basic status effects infrastructure already exists. This plan focuses on design decisions and system completion.

## Goal

Implement complete status effects system with:
- Status effect types and definitions
- Application and tracking mechanisms
- Visual indicators
- Integration with combat stats

## Current State

### Existing Infrastructure

- ✅ `StatusEffects` component exists in `src/components/status_effects.h`
  - Contains `std::vector<StatusEffect>`
  - `StatusEffect` has: `zingDelta`, `bodyDelta`, `duration`

- ✅ `ApplyStatus` operation exists in `EffectResolutionSystem.h`
  - Can apply status effects via dish effects
  - Stores in `StatusEffects` component

- ✅ `ComputeCombatStatsSystem` reads status effects
  - Lines 155-162: Sums status effect deltas
  - Applies to combat stats

### Missing/Incomplete Features

- ❌ Status effect type definitions (what effects exist) - **NOT IMPLEMENTED** (no `StatusEffectType` enum)
- ⚠️ Status effect sources - **PARTIALLY IMPLEMENTED**:
  - ✅ Dish effects can apply status via `EffectOperation::ApplyStatus`
  - ✅ `ApplyDrinkPairingEffects` exists but applies `DishEffect`, not status effects
  - ❌ Synergies don't apply status effects yet
- ❌ Visual indicators (badges on dishes) - **NOT IMPLEMENTED** (no `RenderStatusEffectBadgesSystem`)
- ❌ Duration tracking (currently `duration = 0` means permanent) - **NOT IMPLEMENTED** (no `courseApplied` field)
- ❌ Status effect removal/expiration - **NOT IMPLEMENTED** (no expiration logic in `AdvanceCourseSystem`)

## Current Implementation Status

### What Exists:
- ✅ `StatusEffects` component with `std::vector<StatusEffect>`
- ✅ `StatusEffect` structure with `zingDelta`, `bodyDelta`, `duration` fields
- ✅ `EffectOperation::ApplyStatus` can apply status effects via dish effects
- ✅ `ComputeCombatStatsSystem` reads and applies status effect deltas (lines 155-162)

### What's Missing:
- ❌ `StatusEffectType` enum to define effect types
- ❌ `courseApplied` field to track when effect was applied
- ❌ Visual badge system (`RenderStatusEffectBadgesSystem`)
- ❌ Expiration logic in `AdvanceCourseSystem`
- ❌ Status effect application from drinks (currently drinks apply `DishEffect`, not status)
- ❌ Status effect application from synergies

## Design Decisions Needed (CRITICAL)

### 1. What Are Status Effects vs Dish Effects?

**Status Effects**: Temporary modifiers that can be applied/removed
- Examples: Heat (+1 Spice), Fatigue (-1 Body), PalateCleanser (reset flavor stats)

**Dish Effects**: Permanent effects from dish definitions
- Examples: OnServe effects, stat modifications

**Distinction**: Status effects are dynamic and can change during battle; dish effects are static.

### 2. Status Effect Sources

**Question**: Where do status effects come from?

**Option A: Drinks Only**
- Drinks apply status effects to paired dishes
- Example: Hot Sauce drink → Heat status effect

**Option B: Synergies Only**
- Set bonuses unlock status effects
- Example: 6-piece American → Patriotism status effect

**Option C: Both Drinks and Synergies**
- Drinks apply some status effects
- Synergies apply other status effects
- Both can stack

**Recommendation**: **Option C (Both)** - Most flexible and interesting.

### 3. Status Effect Types

**Proposed Effects**:

1. **Heat** (stat modifier)
   - Source: Drinks or synergies
   - Effect: +1 Spice (flavor stat)
   - Duration: Permanent or N courses

2. **PalateCleanser** (behavior modifier)
   - Source: Drinks
   - Effect: Reset flavor stats to base
   - Duration: One-time trigger

3. **SweetTooth** (stat modifier)
   - Source: Synergies
   - Effect: +1 Sweetness (flavor stat)
   - Duration: Permanent

4. **Fatigue** (stat modifier)
   - Source: Dish effects (debuff)
   - Effect: -1 Body per course
   - Duration: N courses

**Design**: Need to decide which effects exist and what they do.

### 4. Stat Modifiers vs Behavior Modifiers

**Stat Modifiers**: Change flavor stats or combat stats
- Examples: Heat (+Spice), SweetTooth (+Sweetness), Fatigue (-Body)

**Behavior Modifiers**: Change dish behavior
- Examples: PalateCleanser (reset stats), Immunity (prevent debuffs)

**Implementation**: Stat modifiers use existing `StatusEffect` structure. Behavior modifiers may need new system.

## Implementation Strategy

### Phase 1: Design (CRITICAL)

**Before coding, decide**:
1. What status effects exist?
2. What do they do?
3. Where do they come from (drinks, synergies, dish effects)?
4. How long do they last?
5. How do they stack?

### Phase 2: Extend StatusEffect Structure

**Enhance `StatusEffect`**:
```cpp
struct StatusEffect {
  StatusEffectType type; // NEW: Enum for effect types
  int zingDelta = 0;
  int bodyDelta = 0;
  int duration = 0; // 0 = permanent, >0 = courses remaining
  int courseApplied = 0; // NEW: Track when applied
};
```

### Phase 3: Status Effect Application

**Sources**:
1. **Drinks**: Apply via `ApplyDrinkPairingEffects` system
2. **Synergies**: Apply via set bonus system (Task 5)
3. **Dish Effects**: Apply via `EffectOperation::ApplyStatus`

**Logic**: Each source can add status effects to dishes.

### Phase 4: Visual Indicators

**Badges on Dishes**:
- Small icon/badge showing active status effects
- Position: Top-right corner of dish sprite
- Color-coded by effect type
- Tooltip shows effect description

**Implementation**: Create `RenderStatusEffectBadgesSystem`

### Phase 5: Duration Tracking

**Course-Based Duration**:
- Track which course effect was applied
- Decrement duration each course
- Remove when duration expires

**Implementation**: Extend `AdvanceCourseSystem` to decrement durations.

## Test Cases

### Test 1: Apply Status Effect

**Setup**: Apply Heat status effect to dish

**Expected**:
- `StatusEffects` component added
- Effect appears in vector
- Combat stats updated (+1 Spice affects Zing)

### Test 2: Multiple Status Effects

**Setup**: Apply Heat and SweetTooth to same dish

**Expected**:
- Both effects in `StatusEffects.effects`
- Both stat modifiers apply
- Visual shows both badges

### Test 3: Duration Expiration

**Setup**: Apply effect with duration=2 courses

**Expected**:
- Effect applied in course 1
- Still active in course 2
- Removed after course 2 completes

## Validation Steps

1. **DESIGN PHASE** (CRITICAL):
   - Decide status effect types
   - Decide sources (drinks, synergies, effects)
   - Decide mechanics (stat vs behavior modifiers)
   - Document design decisions

2. **Extend StatusEffect Structure**
   - Add `StatusEffectType` enum
   - Add course tracking
   - Update serialization if needed

3. **Implement Status Effect Application**
   - Extend drink system to apply effects
   - Extend synergy system to apply effects
   - Verify dish effects can apply status

4. **Add Visual Indicators**
   - Create `RenderStatusEffectBadgesSystem`
   - Display badges on dishes
   - Add tooltips

5. **Implement Duration Tracking**
   - Track course applied
   - Decrement on course completion
   - Remove expired effects

6. **MANDATORY CHECKPOINT**: Build
   - Run `make` - code must compile successfully

7. **MANDATORY CHECKPOINT**: Run Headless Tests
   - Run `./scripts/run_all_tests.sh` (ALL must pass)

8. **MANDATORY CHECKPOINT**: Run Non-Headless Tests
   - Run `./scripts/run_all_tests.sh -v` (ALL must pass)

9. **Verify Status Effects**
   - Test application works
   - Test visual indicators
   - Test duration tracking

10. **MANDATORY CHECKPOINT**: Commit
    - `git commit -m "implement status effects system"`

## Success Criteria

- ✅ Status effect types defined
- ✅ Status effects can be applied from multiple sources
- ✅ Visual indicators show active effects
- ✅ Duration tracking works correctly
- ✅ All tests pass in headless and non-headless modes

## Estimated Time

6-8 hours:
- 2 hours: Design phase (CRITICAL - must complete first)
- 2 hours: Status effect structure and application
- 1.5 hours: Visual indicators
- 1 hour: Duration tracking
- 1.5 hours: Testing and validation

