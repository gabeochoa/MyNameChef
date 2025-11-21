# Combat Level Scaling Correction Plan

## Overview

This plan details fixing level scaling to use strict 2x per level instead of the current exponential calculation.

## Goal

Fix combat stat level scaling to use **strict 2x per level**:
- Level 1: 1x (base)
- Level 2: 2x
- Level 3: 4x (2x from level 2)
- Level 4: 8x (2x from level 3)

**Current bug**: Uses `2^(level-1)` which is incorrect.

## Current Implementation

### ComputeCombatStatsSystem.h (lines 95-104)

**Current Code**:
```cpp
// Level scaling: multiply by 2 for each level above 1
if (lvl.level > 1) {
  int level_multiplier = 2;
  for (int i = 2; i < lvl.level; ++i) {
    level_multiplier *= 2;
  }
  zing *= level_multiplier;
  body *= level_multiplier;
}
```

**Analysis**:
- Level 2: `i = 2; i < 2` → loop doesn't run → `multiplier = 2` ✅ Correct
- Level 3: `i = 2; i < 3` → loop runs once → `multiplier = 2 * 2 = 4` ✅ Correct
- Level 4: `i = 2; i < 4` → loop runs twice → `multiplier = 2 * 2 * 2 = 8` ✅ Correct

**Wait**: The current code appears correct! Let me verify the bug description.

**From next_todo.md line 12**: "Level Scaling Bug Fix: ✅ Complete (commit 0c0aa8e - fixed to use 2x per level instead of 2^(level-1))"

**Conclusion**: This bug is already fixed. However, there may be other places with incorrect scaling.

## Verification Tasks

### 1. Check All Level Scaling Locations

**Files to Check**:
- `src/systems/ComputeCombatStatsSystem.h` - ✅ Already correct
- `src/systems/RenderZingBodyOverlay.h` - Check lines 153-159
- Any other systems that calculate level scaling

### 2. RenderZingBodyOverlay.h (lines 153-159)

**Current Code**:
```cpp
if (entity.has<DishLevel>()) {
  const auto &lvl = entity.get<DishLevel>();
  if (lvl.level > 1) {
    int level_multiplier = 1 << (lvl.level - 1);
    zing *= level_multiplier;
    body *= level_multiplier;
  }
}
```

**Analysis**:
- Level 2: `1 << (2-1) = 1 << 1 = 2` ✅ Correct
- Level 3: `1 << (3-1) = 1 << 2 = 4` ✅ Correct
- Level 4: `1 << (4-1) = 1 << 3 = 8` ✅ Correct

**Conclusion**: This is also correct! `1 << (level-1)` is equivalent to `2^(level-1)`, which equals the desired 2x per level.

## Potential Issues

If the bug is already fixed, the plan should focus on:

1. **Regression Testing**
   - Ensure all level scaling tests pass
   - Verify no regressions introduced

2. **Consistency Check**
   - Ensure all systems use same scaling formula
   - Document the correct formula

3. **Test Coverage**
   - Add tests for level scaling if missing
   - Verify edge cases (level 1, very high levels)

## Correct Formula

**Strict 2x per level**:
- Level 1: `multiplier = 1` (base, no scaling)
- Level 2: `multiplier = 2` (2x)
- Level 3: `multiplier = 4` (2x from level 2)
- Level 4: `multiplier = 8` (2x from level 3)

**Implementation**:
```cpp
int level_multiplier = 1;
for (int i = 1; i < lvl.level; ++i) {
  level_multiplier *= 2;
}
// Or: int level_multiplier = 1 << (lvl.level - 1);
```

## Validation Steps

1. **Review Current Implementation**
   - Check `ComputeCombatStatsSystem.h`
   - Check `RenderZingBodyOverlay.h`
   - Check any other level scaling code

2. **Verify Formula Correctness**
   - Test level 1: should be 1x
   - Test level 2: should be 2x
   - Test level 3: should be 4x
   - Test level 4: should be 8x

3. **Add Regression Tests** (if missing)
   - Test level scaling in combat stats
   - Test level scaling in UI overlay
   - Verify consistency across systems

4. **MANDATORY CHECKPOINT**: Build
   - Run `make` - code must compile successfully

5. **MANDATORY CHECKPOINT**: Run Headless Tests
   - Run `./scripts/run_all_tests.sh` (ALL must pass)

6. **MANDATORY CHECKPOINT**: Run Non-Headless Tests
   - Run `./scripts/run_all_tests.sh -v` (ALL must pass)

7. **MANDATORY CHECKPOINT**: Commit
   - `git commit -m "be - verify and document level scaling formula"`

## Success Criteria

- ✅ All level scaling uses correct 2x per level formula
- ✅ All systems use consistent formula
- ✅ Tests verify level scaling correctness
- ✅ No regressions introduced

## Estimated Time

1-2 hours (verification and documentation):
- 30 min: Code review
- 30 min: Testing
- 30 min: Documentation
- 30 min: Test creation (if needed)

