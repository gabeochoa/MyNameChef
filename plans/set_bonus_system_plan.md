# Set Bonus Effects & Legend UI Plan

## Overview

This plan documents the current state of the set bonus system and identifies remaining work. **Most of the core functionality is already implemented.**

## Goal

Complete set bonus system that:
- ✅ Applies bonuses when synergy thresholds reached (2/4/6)
- ✅ Displays active synergies in battle legend box
- ❌ Provides visual feedback when thresholds achieved

## Current State

### ✅ Fully Implemented

1. **Set Bonus Definitions** (`src/components/set_bonus_definitions.h` & `.cpp`)
   - `SetBonusDefinition` struct with support for:
     - Stat bonuses (`zingDelta`, `bodyDelta`)
     - Effect-based bonuses (`bonusEffects`, `bonusEffectsCount`)
     - Target scope (`TargetScope::AllAllies`, `TargetScope::AllOpponents`)
   - Complete definitions for all 10 cuisine types:
     - American, Thai, Italian, Japanese, Mexican, French, Chinese, Indian, Korean, Vietnamese
   - Each cuisine has unique bonuses at 2/4/6 thresholds
   - Some bonuses include trigger effects (e.g., American 4-piece: OnBiteTaken +1 Body)

2. **Bonus Application System** (`src/systems/ApplySetBonusesSystem.h`)
   - Reads `BattleSynergyCounts` singleton
   - Checks thresholds (2, 4, 6) for each cuisine
   - Applies bonuses to `PersistentCombatModifiers` (not `PreBattleModifiers`)
   - Tracks applied bonuses in `AppliedSetBonuses` singleton to prevent duplicates
   - Applies stat bonuses and effect bonuses separately
   - Runs once per battle (when entering battle screen)

3. **Battle Synergy Counting** (`src/systems/BattleSynergyCountingSystem.h`)
   - Counts cuisine tags for player and opponent teams
   - Updates `BattleSynergyCounts` singleton
   - Only counts dishes in `InQueue` or `Entering` phase
   - Runs once per battle (uses `calculated` flag)

4. **Battle Synergy Legend UI** (`src/systems/RenderBattleSynergyLegend.h`)
   - Displays active synergies in top-left corner (x=20, y=170)
   - Shows format: "American: 4/6" (current count / next threshold)
   - Only displays synergies with count >= 2
   - Updates each frame (reads from `BattleSynergyCounts`)

5. **Effect Integration** (`src/systems/EffectResolutionSystem.h`)
   - Processes `SynergyBonusEffects` component
   - Applies trigger-based effects (e.g., OnBiteTaken)
   - Integrates with existing effect system

6. **Test Coverage** (`src/testing/tests/ValidateSetBonusSystemTest.h`)
   - Tests for 2-piece, 4-piece, 6-piece bonuses
   - Tests for no synergy case
   - Validates modifiers are applied correctly
   - All tests pass

### ❌ Missing Features

1. **Visual Feedback for Threshold Achievements**
   - No celebration animation when hitting 4-piece or 6-piece bonus
   - No particle effects or glow around dishes
   - No sound effects
   - No visual indication when threshold is crossed

2. **Real-Time Updates**
   - `BattleSynergyCountingSystem` only runs once per battle
   - `ApplySetBonusesSystem` only runs once per battle
   - Bonuses don't update if dishes leave combat mid-battle
   - Legend updates each frame but counts are static after initial calculation

3. **Other Tag Type Support**
   - Only `CuisineTag` is supported
   - `CourseTag`, `BrandTag`, `DishArchetypeTag` are counted but have no bonuses
   - Architecture could support other tag types but definitions don't exist

## Implementation Architecture

### Component Flow

1. **Battle Entry**:
   - `BattleSynergyCountingSystem` counts cuisine tags → `BattleSynergyCounts`
   - `ApplySetBonusesSystem` reads counts → applies bonuses → `PersistentCombatModifiers` + `SynergyBonusEffects`
   - `ComputeCombatStatsSystem` reads `PersistentCombatModifiers` → calculates final stats

2. **Effect Processing**:
   - `EffectResolutionSystem` processes `SynergyBonusEffects` on trigger events
   - Effects can modify stats mid-battle (e.g., OnBiteTaken +1 Body)

3. **UI Rendering**:
   - `RenderBattleSynergyLegend` reads `BattleSynergyCounts` each frame
   - Displays active synergies with current/next threshold format

### Key Components

- `SetBonusDefinition`: Defines bonus structure (stats + effects)
- `BattleSynergyCounts`: Singleton tracking cuisine counts per team
- `AppliedSetBonuses`: Singleton tracking which bonuses have been applied
- `PersistentCombatModifiers`: Component storing stat bonuses
- `SynergyBonusEffects`: Component storing effect-based bonuses

### Key Systems

- `BattleSynergyCountingSystem`: Counts tags once per battle
- `ApplySetBonusesSystem`: Applies bonuses once per battle
- `RenderBattleSynergyLegend`: Renders legend each frame
- `EffectResolutionSystem`: Processes synergy effects during combat

## Remaining Work

### Priority 1: Visual Feedback (Optional Enhancement)

**Goal**: Add celebration when thresholds are achieved

**Implementation Options**:
1. **Animation Events**: Use existing `AnimationEvent` system
   - Create `AnimationEventType::SetBonusAchieved`
   - Trigger when threshold crossed (detect in `ApplySetBonusesSystem`)
   - Render celebration animation in `RenderAnimations`

2. **Particle Effects**: Add glow/particles around dishes
   - Create new particle system or extend existing
   - Trigger on threshold achievement
   - Visual indicator on affected dishes

3. **Sound Effects**: Play sound when threshold reached
   - Add to sound library
   - Trigger in `ApplySetBonusesSystem`

**Files to Modify**:
- `src/systems/ApplySetBonusesSystem.h` - Detect threshold crossing
- `src/systems/RenderAnimations.h` - Render celebration (if using animation events)
- `src/components/animation_event.h` - Add new event type (if needed)

**Estimated Time**: 2-4 hours

### Priority 2: Real-Time Updates (Future Enhancement)

**Goal**: Update bonuses as dishes enter/exit combat

**Current Limitation**: Systems run once per battle, so bonuses are static

**Implementation**:
1. Remove `calculated` flag from `BattleSynergyCountingSystem`
2. Make it run continuously (check `should_run` each frame)
3. Remove `applied` flag from `ApplySetBonusesSystem`
4. Track previous counts to detect threshold crossings
5. Re-apply bonuses when counts change

**Considerations**:
- Performance: Continuous counting may be expensive
- Edge cases: What happens when dish finishes mid-battle?
- Should bonuses be removed if count drops below threshold?

**Files to Modify**:
- `src/systems/BattleSynergyCountingSystem.h`
- `src/systems/ApplySetBonusesSystem.h`
- `src/components/applied_set_bonuses.h` - May need to track previous state

**Estimated Time**: 4-6 hours

### Priority 3: Other Tag Type Support (Future Enhancement)

**Goal**: Add set bonuses for CourseTag, BrandTag, DishArchetypeTag

**Implementation**:
1. Create `get_course_bonus_definitions()` similar to cuisine
2. Create `get_brand_bonus_definitions()`
3. Create `get_archetype_bonus_definitions()`
4. Extend `ApplySetBonusesSystem` to check all tag types
5. Extend `RenderBattleSynergyLegend` to display all tag types

**Files to Modify**:
- `src/components/set_bonus_definitions.cpp` - Add new definition functions
- `src/systems/ApplySetBonusesSystem.h` - Process all tag types
- `src/systems/RenderBattleSynergyLegend.h` - Display all tag types
- `src/components/battle_synergy_counts.h` - Already has counts for all types

**Estimated Time**: 6-8 hours

## Test Cases (Already Implemented)

All test cases in `ValidateSetBonusSystemTest.h` pass:

1. ✅ **2-Piece Bonus**: 2 American dishes → +1 Body each
2. ✅ **4-Piece Bonus**: 4 American dishes → +3 Body each (2-piece +1, 4-piece +2)
3. ✅ **6-Piece Bonus**: 6 American dishes → +6 Body each (2-piece +1, 4-piece +2, 6-piece +3)
4. ✅ **No Synergy**: Dishes without matching tags → no bonuses

**Note**: Tests validate that bonuses stack (2+4+6 = total), not replace each other.

## Validation Steps

### Current System Validation

1. ✅ **Build**: Code compiles successfully
2. ✅ **Headless Tests**: All tests pass (`./scripts/run_all_tests.sh`)
3. ✅ **Non-Headless Tests**: All tests pass (`./scripts/run_all_tests.sh -v`)
4. ✅ **Manual Testing**: Bonuses apply correctly in battle
5. ✅ **Legend Display**: Synergies show correctly in battle UI

### Future Enhancement Validation

If implementing visual feedback or real-time updates:

1. **Build**: Run `xmake` - code must compile successfully
2. **Headless Tests**: Run `./scripts/run_all_tests.sh` (ALL must pass)
3. **Non-Headless Tests**: Run `./scripts/run_all_tests.sh -v` (ALL must pass)
4. **Manual Testing**: Verify visual feedback appears correctly
5. **Commit**: `git commit -m "add visual feedback for set bonus thresholds"`

## Edge Cases (Handled)

1. ✅ **Partial Teams**: Bonuses apply only to dishes with matching tags
2. ✅ **Multiple Thresholds**: Bonuses stack (2-piece + 4-piece + 6-piece all apply)
3. ✅ **Multiple Cuisines**: Each cuisine tracked separately
4. ✅ **Effect Bonuses**: Trigger-based effects work correctly (e.g., OnBiteTaken)
5. ✅ **Opponent Bonuses**: Opponent team also gets bonuses (e.g., Indian cuisine debuffs)

## Edge Cases (Not Handled - Future Work)

1. ❌ **Dishes Leave Combat**: Bonuses don't update if dish finishes mid-battle
2. ❌ **Threshold Crossing**: No visual feedback when crossing threshold
3. ❌ **Count Drops Below Threshold**: Bonuses remain even if count drops (by design - once applied, stays)

## Success Criteria

### ✅ Completed

- ✅ Set bonuses apply when thresholds reached
- ✅ Battle synergy legend displays active synergies
- ✅ All tests pass in headless and non-headless modes
- ✅ Effect-based bonuses work correctly
- ✅ Multiple cuisines supported

### ❌ Remaining (Optional)

- ❌ Visual feedback for threshold achievements
- ❌ Real-time updates as dishes enter/exit combat
- ❌ Support for other tag types (CourseTag, BrandTag, etc.)

## Design Decisions (Already Made)

### Bonus Design: Hybrid Approach ✅

The system uses **Option 3 (Hybrid)** from the original plan:
- 2-piece: Stat bonus only
- 4-piece: Stat bonus + effect unlock (some cuisines)
- 6-piece: Stat bonus + powerful effect (some cuisines)

**Examples**:
- **American**: 2-piece = +1 Body, 4-piece = +2 Body + OnBiteTaken +1 Body, 6-piece = +3 Body + OnBiteTaken +2 Body
- **Thai**: 2-piece = +1 Zing, 4-piece = +2 Zing, 6-piece = +3 Zing (stats only)
- **Indian**: 2-piece = +1 Zing (allies), 4-piece = -1 Zing (opponents), 6-piece = -2 Zing (opponents)

### Component Choice: PersistentCombatModifiers ✅

Bonuses are applied to `PersistentCombatModifiers`, not `PreBattleModifiers`:
- `PreBattleModifiers` is now a write-only mirror (for backward compatibility)
- `PersistentCombatModifiers` is the source of truth for combat stat modifiers
- `ComputeCombatStatsSystem` reads from `PersistentCombatModifiers`

### System Architecture: Once-Per-Battle ✅

Both counting and application systems run once per battle:
- `BattleSynergyCountingSystem` uses `calculated` flag
- `ApplySetBonusesSystem` uses `applied` flag
- This is by design - bonuses are calculated at battle start and remain static

## Estimated Time for Remaining Work

**Visual Feedback Only**: 2-4 hours
- Detect threshold crossing: 1 hour
- Add animation/particles: 1-2 hours
- Testing: 1 hour

**Real-Time Updates**: 4-6 hours
- Remove once-per-battle flags: 1 hour
- Add threshold crossing detection: 2 hours
- Handle edge cases: 1-2 hours
- Testing: 1 hour

**Other Tag Types**: 6-8 hours
- Create definitions: 2-3 hours
- Extend systems: 2-3 hours
- Testing: 2 hours

## Notes

- The "Herb Garden" example mentioned in the original plan is not implemented and appears to be a future feature idea
- The system is production-ready for CuisineTag bonuses
- Visual feedback is a nice-to-have enhancement, not required for core functionality
- Real-time updates may not be necessary if bonuses are meant to be static per battle
