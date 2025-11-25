# Duration-Based Effects Plan

## Overview

This plan details implementing effects that last N courses (duration-based effects), extending the existing status effects system.

## Goal

Implement duration tracking for effects:
- Effects expire after N courses
- Track which course an effect was applied
- Remove effects when duration expires

## Current State

### Existing Infrastructure

- ✅ `StatusEffect` has `duration` field (currently `duration = 0` means permanent)
- ✅ `StatusEffects` component exists
- ✅ `AdvanceCourseSystem` exists (can track course completion)

### Missing Features

- ❌ Course tracking for effects (when was effect applied?) - **NOT IMPLEMENTED** (no `courseApplied` field in `StatusEffect`)
- ❌ Duration decrement logic - **NOT IMPLEMENTED** (no expiration logic in `AdvanceCourseSystem`)
- ❌ Effect expiration/removal - **NOT IMPLEMENTED** (no `expire_status_effects()` function)

## Current Implementation Status

### What Exists:
- ✅ `StatusEffect` has `duration` field (0 = permanent, >0 = courses remaining)
- ✅ `StatusEffects` component exists
- ✅ `AdvanceCourseSystem` exists and tracks course completion

### What's Missing:
- ❌ `courseApplied` field in `StatusEffect` structure
- ❌ Logic to set `courseApplied` when effect is applied
- ❌ Expiration logic in `AdvanceCourseSystem` to remove expired effects
- ❌ `expire_status_effects()` function to calculate and remove expired effects

## Implementation Details

### 1. Track Course Applied

**Enhance `StatusEffect`**:
```cpp
struct StatusEffect {
  int zingDelta = 0;
  int bodyDelta = 0;
  int duration = 0; // 0 = permanent, >0 = courses remaining
  int courseApplied = 0; // NEW: Course number when applied
};
```

**When Applying Effect**:
- Get current course from `CombatQueue.current_index`
- Set `courseApplied = currentCourse`
- Set `duration = N` (desired duration)

### 2. Track Course Completion

**In `AdvanceCourseSystem`**:
- When course completes, decrement durations
- Check all dishes with `StatusEffects`
- For each effect with `duration > 0`:
  - Calculate courses elapsed: `currentCourse - effect.courseApplied`
  - If `coursesElapsed >= duration`: Remove effect
  - Otherwise: Keep effect (duration still valid)

### 3. Remove Expired Effects

**Logic**:
```cpp
void expire_status_effects(int currentCourse) {
  for (Entity &dish : dishes_with_status_effects) {
    StatusEffects &se = dish.get<StatusEffects>();
    
    // Remove expired effects
    se.effects.erase(
      std::remove_if(se.effects.begin(), se.effects.end(),
        [currentCourse](const StatusEffect &effect) {
          if (effect.duration == 0) return false; // Permanent
          int coursesElapsed = currentCourse - effect.courseApplied;
          return coursesElapsed >= effect.duration;
        }),
      se.effects.end()
    );
    
    // Remove component if empty
    if (se.effects.empty()) {
      dish.removeComponent<StatusEffects>();
    }
  }
}
```

### 4. Integration Points

**Apply Effect** (in `EffectResolutionSystem`):
```cpp
case EffectOperation::ApplyStatus: {
  auto &status_effects = target.addComponentIfMissing<StatusEffects>();
  StatusEffect status;
  status.zingDelta = effect.amount;
  status.bodyDelta = effect.statusBodyDelta;
  status.duration = effect.duration; // NEW: From effect definition
  status.courseApplied = get_current_course(); // NEW: Track course
  status_effects.effects.push_back(status);
  break;
}
```

**Expire Effects** (in `AdvanceCourseSystem`):
- After course completes, before advancing
- Call `expire_status_effects(currentCourse)`

## Test Cases

### Test 1: Effect Expires After Duration

**Setup**:
- Apply effect with `duration = 2` in course 1
- Complete course 1
- Complete course 2

**Expected**:
- Effect active in course 1
- Effect active in course 2
- Effect removed after course 2 completes

### Test 2: Permanent Effect Doesn't Expire

**Setup**:
- Apply effect with `duration = 0` (permanent)
- Complete multiple courses

**Expected**:
- Effect remains active through all courses
- Never expires

### Test 3: Multiple Effects with Different Durations

**Setup**:
- Apply effect A with `duration = 1` in course 1
- Apply effect B with `duration = 3` in course 1
- Complete courses

**Expected**:
- Effect A expires after course 1
- Effect B expires after course 3
- Effects expire independently

## Validation Steps

1. **Extend StatusEffect Structure**
   - Add `courseApplied` field
   - Update effect application to set course

2. **Implement Course Tracking**
   - Get current course when applying effect
   - Store in `StatusEffect.courseApplied`

3. **Implement Expiration Logic**
   - Add `expire_status_effects()` function
   - Calculate courses elapsed
   - Remove expired effects

4. **Integrate with AdvanceCourseSystem**
   - Call expiration logic after course completes
   - Before advancing to next course

5. **MANDATORY CHECKPOINT**: Build
   - Run `make` - code must compile successfully

6. **MANDATORY CHECKPOINT**: Run Headless Tests
   - Run `./scripts/run_all_tests.sh` (ALL must pass)

7. **MANDATORY CHECKPOINT**: Run Non-Headless Tests
   - Run `./scripts/run_all_tests.sh -v` (ALL must pass)

8. **Verify Duration Tracking**
   - Test effects expire correctly
   - Test permanent effects don't expire
   - Test multiple effects expire independently

9. **MANDATORY CHECKPOINT**: Commit
   - `git commit -m "implement duration-based effects"`

## Edge Cases

1. **Effect Applied in Last Course**: Duration may extend beyond battle
2. **Battle Ends Early**: Effects expire when battle ends
3. **Effect Reapplied**: Same effect applied multiple times (stack or replace?)
4. **Course 0 vs Course 1**: Ensure course numbering is consistent

## Success Criteria

- ✅ Effects track which course they were applied
- ✅ Effects expire after N courses
- ✅ Permanent effects (duration=0) never expire
- ✅ Multiple effects expire independently
- ✅ All tests pass in headless and non-headless modes

## Estimated Time

4-6 hours:
- 1 hour: Extend StatusEffect structure
- 1.5 hours: Course tracking implementation
- 1.5 hours: Expiration logic
- 1 hour: Testing and validation

