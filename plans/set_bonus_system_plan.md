# Set Bonus Effects & Legend UI Plan

## Overview

This plan details implementing set bonus effects that activate when synergy thresholds (2/4/6) are reached, and a battle legend UI to display active synergies.

## Goal

Implement set bonus system that:
- Applies bonuses when synergy thresholds reached (2/4/6)
- Displays active synergies in battle legend box
- Provides visual feedback when thresholds achieved

## Current State

### Existing Infrastructure

- ✅ `SynergyCounts` singleton exists
- ✅ `SynergyCountingSystem` counts tags in shop
- ✅ `BattleSynergyCountingSystem` counts tags in battle
- ✅ `ApplyPairingsAndClashesSystem` exists (can be extended)
- ✅ `PreBattleModifiers` component exists for stat bonuses

### Missing Features

- ❌ Set bonus definitions (what bonuses are)
- ❌ Threshold checking and bonus application
- ❌ Battle synergy legend box UI
- ❌ Visual feedback for threshold achievements

## Design Decisions Needed

### Bonus Design (CRITICAL - Needs Design Planning)

**Question**: What are set bonuses?

**Option 1: Stat Bonuses Only**
- 2-piece: +1 Zing, +1 Body
- 4-piece: +2 Zing, +2 Body
- 6-piece: +3 Zing, +3 Body

**Option 2: Effect Unlocks**
- 2-piece: Stat bonus
- 4-piece: Stat bonus + new effect
- 6-piece: Stat bonus + powerful effect

**Option 3: Hybrid**
- 2-piece: Stat bonus
- 4-piece: Larger stat bonus
- 6-piece: Stat bonus + effect unlock

**Recommendation**: Start with **Option 1 (Stat Bonuses Only)** for proof of concept, then expand.

## Implementation Details

### 1. Set Bonus Definitions

**File**: Create `src/components/set_bonus_definitions.h` or hardcode in system

**Structure**:
```cpp
struct SetBonusDefinition {
  CuisineTagType cuisine; // Or other tag type
  int threshold; // 2, 4, or 6
  int zingBonus;
  int bodyBonus;
};

// Hardcoded definitions (proof of concept with CuisineTag)
std::vector<SetBonusDefinition> set_bonus_definitions = {
  {CuisineTagType::American, 2, 1, 1},  // 2-piece: +1 Zing, +1 Body
  {CuisineTagType::American, 4, 2, 2},  // 4-piece: +2 Zing, +2 Body
  {CuisineTagType::American, 6, 3, 3},  // 6-piece: +3 Zing, +3 Body
  // Add more cuisines...
};
```

### 2. Extend ApplyPairingsAndClashesSystem

**File**: `src/systems/ApplyPairingsAndClashesSystem.h`

**Logic**:
- Read `BattleSynergyCounts` singleton (or `SynergyCounts` for shop)
- Check each tag type for threshold achievements
- Apply bonuses to `PreBattleModifiers` when thresholds reached
- Only apply once per threshold (don't stack multiple times)

**Implementation**:
```cpp
void apply_set_bonuses(afterhours::Entity &dish, const BattleSynergyCounts &counts) {
  if (!dish.has<CuisineTag>()) return;
  
  const auto &tag = dish.get<CuisineTag>();
  auto &pre = dish.addComponentIfMissing<PreBattleModifiers>();
  
  for (auto cuisine : magic_enum::enum_values<CuisineTagType>()) {
    if (!tag.has(cuisine)) continue;
    
    int count = counts.player_cuisine_counts[cuisine];
    
    // Check thresholds and apply bonuses
    if (count >= 6) {
      pre.zingDelta += 3;
      pre.bodyDelta += 3;
    } else if (count >= 4) {
      pre.zingDelta += 2;
      pre.bodyDelta += 2;
    } else if (count >= 2) {
      pre.zingDelta += 1;
      pre.bodyDelta += 1;
    }
  }
}
```

### 3. Battle Synergy Legend Box

**File**: `src/systems/RenderBattleSynergyLegendSystem.h` (new)

**Purpose**: Display active synergies in battle

**Display Rules**:
- Only show synergies that have reached at least 2/4/6 threshold
- Format: "American: 4/6" or "Bread: 2/4"
- Position: Top-left or top-right corner of battle screen

**UI Layout**:
```
┌─────────────────┐
│ Synergies:       │
│ American: 4/6   │ ← Active (reached 4)
│ Bread: 2/4      │ ← Active (reached 2)
└─────────────────┘
```

**Implementation**:
- Read `BattleSynergyCounts` singleton
- Filter to only show tags with count >= 2
- Display count and next threshold
- Update in real-time as dishes enter/exit

### 4. Visual Feedback

**Threshold Achievements**:
- Celebration animation when hitting 4-piece or 6-piece bonus
- Particle effects or glow around dishes
- Sound effect (optional)

**Implementation**:
- Detect threshold crossing (count goes from <threshold to >=threshold)
- Trigger animation/effect
- Log achievement

## Scope

**Start with**: One tag type (CuisineTag) as proof of concept

**Future**: Extend to other tag types (CourseTag, BrandTag, etc.)

**Skip for now**: "Herb Garden" example - focus on stat bonuses first

## Test Cases

### Test 1: 2-Piece Bonus

**Setup**:
- 2 American dishes in team
- Enter battle

**Expected**:
- Each dish gets +1 Zing, +1 Body from set bonus
- Legend shows "American: 2/4"

### Test 2: 4-Piece Bonus

**Setup**:
- 4 American dishes in team
- Enter battle

**Expected**:
- Each dish gets +2 Zing, +2 Body (4-piece bonus, not 2-piece)
- Legend shows "American: 4/6"

### Test 3: 6-Piece Bonus

**Setup**:
- 6 American dishes in team
- Enter battle

**Expected**:
- Each dish gets +3 Zing, +3 Body (6-piece bonus)
- Legend shows "American: 6/6" or "American: 6"

### Test 4: Multiple Synergies

**Setup**:
- 4 American dishes, 2 Bread dishes

**Expected**:
- American dishes get 4-piece bonus
- Bread dishes get 2-piece bonus
- Legend shows both synergies

## Validation Steps

1. **Design Phase**: Decide on bonus design (stats vs effects vs both)

2. **Create Set Bonus Definitions**
   - Define data structure for bonuses
   - Hardcode definitions for one tag type (CuisineTag)

3. **Extend ApplyPairingsAndClashesSystem**
   - Read `BattleSynergyCounts`
   - Check thresholds
   - Apply bonuses to `PreBattleModifiers`

4. **Add Battle Synergy Legend Box**
   - Create `RenderBattleSynergyLegendSystem`
   - Display active synergies
   - Update in real-time

5. **Add Visual Feedback**
   - Celebration animation for thresholds
   - Particle effects (optional)

6. **MANDATORY CHECKPOINT**: Build
   - Run `xmake` - code must compile successfully

7. **MANDATORY CHECKPOINT**: Run Headless Tests
   - Run `./scripts/run_all_tests.sh` (ALL must pass)

8. **MANDATORY CHECKPOINT**: Run Non-Headless Tests
   - Run `./scripts/run_all_tests.sh -v` (ALL must pass)

9. **Verify Set Bonuses**
   - Test bonuses apply correctly
   - Test legend displays correctly
   - Test visual feedback works

10. **MANDATORY CHECKPOINT**: Commit
    - `git commit -m "implement set bonus effects with threshold system"`

## Edge Cases

1. **Partial Teams**: Some dishes don't have tag (bonus still applies to those with tag)
2. **Dishes Leave Combat**: Bonuses should update if dishes finish
3. **Multiple Tag Types**: Dish has multiple tags (gets bonuses from all)
4. **Threshold Crossing**: Animation triggers when crossing threshold

## Success Criteria

- ✅ Set bonuses apply when thresholds reached
- ✅ Battle synergy legend displays active synergies
- ✅ Visual feedback for threshold achievements
- ✅ All tests pass in headless and non-headless modes

## Estimated Time

6-8 hours:
- 1 hour: Design phase (decide bonus structure)
- 2 hours: Set bonus definitions and application
- 2 hours: Battle synergy legend UI
- 1 hour: Visual feedback
- 2 hours: Testing and validation

