# Three-of-a-Kind Merge Plan

## Overview

This plan details implementing 3-of-a-kind merging (combining 3 duplicate dishes into 1 level-up dish), extending the existing 2-of-a-kind merge system.

## Goal

Implement merge system where:
- **3 dishes of same type** → combine into **1 dish of level +1**
- Extends existing 2-of-a-kind merge (which creates level progress)
- Provides alternative merge path for faster leveling

## Current State

### Existing Merge System

- **2-of-a-kind merge**: Already implemented
  - Location: `src/systems/DropWhenNoLongerHeld.cpp::merge_dishes()`
  - Logic: 2 level N dishes → 1 level N+1 dish
  - Uses `DishLevel.contribution_value()` and `add_merge_value()`
  - Merge trigger: User drags dish onto another dish of same type

### Current Merge Logic

**From `DropWhenNoLongerHeld.cpp`**:
- Checks: Same dish type, both have `DishLevel`, donor level <= target level
- Action: Adds donor's `contribution_value()` to target
- Result: Target level increases based on contribution

## Implementation Strategy

### Option 1: Extend Existing Merge (Recommended)

**Approach**: Detect when 3 dishes are in merge position, combine all 3 at once.

**Logic**:
1. User drags dish A onto dish B (both same type)
2. Check if dish C (same type) is adjacent or in merge range
3. If 3 dishes detected: combine all 3 → level +1
4. If only 2 dishes: use existing 2-of-a-kind merge

### Option 2: Separate 3-of-a-Kind Trigger

**Approach**: Require explicit 3-dish selection or special merge slot.

**Logic**:
1. Create "triple merge slot" or special area
2. User places 3 dishes in merge area
3. System detects 3-of-a-kind and merges

### Option 3: Auto-Detect Adjacent Triples

**Approach**: Automatically detect when 3 same-type dishes are adjacent in inventory.

**Logic**:
1. System scans inventory for 3 adjacent same-type dishes
2. Auto-merges when detected
3. User can disable auto-merge if desired

## Recommended Solution

**Use Option 1 (Extend Existing Merge)** - Most intuitive and builds on existing system.

### Implementation Details

#### 1. Detect 3-of-a-Kind During Merge

**In `DropWhenNoLongerHeld::merge_dishes()`**:

```cpp
void merge_dishes(Entity &entity, Entity *target_item, ...) {
  // Check if this is a 3-of-a-kind merge
  auto third_dish = find_third_same_type_dish(entity, target_item);
  
  if (third_dish) {
    // 3-of-a-kind merge: combine all 3 → level +1
    perform_triple_merge(entity, target_item, third_dish);
  } else {
    // 2-of-a-kind merge: existing logic
    perform_double_merge(entity, target_item);
  }
}
```

#### 2. Find Third Dish

**Logic**:
- Search inventory for third dish of same type
- Check if it's adjacent to target or in merge range
- Return entity if found, nullptr otherwise

#### 3. Triple Merge Logic

**Action**:
- Combine all 3 dishes
- Target dish gets level +1 (not just contribution)
- Remove donor dishes
- Free slots

**Level Calculation**:
- If target is level N, result is level N+1
- Donor levels don't matter (all 3 combine to +1 level)

#### 4. UI Feedback

**Visual Indicators**:
- Highlight when 3-of-a-kind merge is possible
- Show "Ready to merge 3!" indicator
- Animation when triple merge occurs

## Test Cases

### Test 1: Basic 3-of-a-Kind Merge

**Setup**:
- 3 level 1 Potato dishes in inventory
- Drag one onto another

**Expected**:
- System detects third dish
- All 3 combine into 1 level 2 Potato
- 2 slots freed

### Test 2: 3-of-a-Kind with Different Levels

**Setup**:
- 1 level 1 Potato, 1 level 2 Potato, 1 level 1 Potato
- Drag level 1 onto level 2

**Expected**:
- System detects third dish (level 1)
- All 3 combine into 1 level 3 Potato (level 2 + 1)
- 2 slots freed

### Test 3: 2-of-a-Kind Still Works

**Setup**:
- 2 level 1 Potato dishes (no third)
- Drag one onto another

**Expected**:
- Uses existing 2-of-a-kind merge
- Target gets contribution from donor
- No level jump (just progress)

### Test 4: Merge Range Detection

**Setup**:
- 3 Potato dishes, but third is far away (not adjacent)

**Expected**:
- System doesn't detect 3-of-a-kind
- Uses 2-of-a-kind merge instead

## Validation Steps

1. **Extend Merge Detection**
   - Add `find_third_same_type_dish()` function
   - Integrate with existing merge logic

2. **Implement Triple Merge**
   - Create `perform_triple_merge()` function
   - Handle level +1 calculation
   - Remove donor dishes

3. **Add UI Feedback**
   - Highlight when 3-of-a-kind possible
   - Add merge animation
   - Show "Ready to merge 3!" indicator

4. **MANDATORY CHECKPOINT**: Build
   - Run `xmake` - code must compile successfully

5. **MANDATORY CHECKPOINT**: Run Headless Tests
   - Run `./scripts/run_all_tests.sh` (ALL must pass)

6. **MANDATORY CHECKPOINT**: Run Non-Headless Tests
   - Run `./scripts/run_all_tests.sh -v` (ALL must pass)

7. **Verify Merge Functionality**
   - Test 3-of-a-kind merge works
   - Test 2-of-a-kind still works
   - Test UI feedback

8. **MANDATORY CHECKPOINT**: Commit
   - `git commit -m "implement three-of-a-kind merge system"`

## Edge Cases

1. **Partial Merge**: User drags away before completing 3-of-a-kind
2. **Multiple Triples**: 6 same-type dishes (2 potential triples)
3. **Mixed Types**: 3 Potatoes + 2 Salmons (shouldn't merge)
4. **Level Limits**: What happens at max level?

## Success Criteria

- ✅ 3-of-a-kind merge combines 3 dishes into level +1
- ✅ 2-of-a-kind merge still works correctly
- ✅ UI provides clear feedback for triple merge
- ✅ All tests pass in headless and non-headless modes

## Estimated Time

4-6 hours:
- 2 hours: Merge detection and logic
- 1.5 hours: UI feedback
- 1 hour: Testing
- 30 min: Edge case handling

