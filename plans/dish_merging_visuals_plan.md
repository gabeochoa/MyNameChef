# Visual Enhancements for Dish Merging Plan

## Overview

This plan details adding visual feedback for dish merging. **Note**: Dish merging system is already implemented. This task is purely visual polish.

## Goal

Add visual enhancements for dish merging:
- Merge animation showing dishes combining
- Level-up sound effect
- "Ready to merge!" indicator
- Level display badge on dishes

## Current State

### Existing Merge System

- ✅ Merge logic implemented in `src/systems/DropWhenNoLongerHeld.cpp::merge_dishes()`
- ✅ 2 level N dishes → 1 level N+1 dish
- ✅ Uses `DishLevel.contribution_value()` and `add_merge_value()`
- ✅ Level scaling: 2x per level (level 2 = 2x, level 3 = 4x, level 4 = 8x)

### Missing Visual Features

- ❌ Merge animation
- ❌ Level-up sound effect
- ❌ "Ready to merge!" indicator
- ❌ Level display badge

## Implementation Details

### 1. Merge Animation

**Visual Effect**:
- When merge occurs, show dishes combining
- Options:
  - Fade out donor, scale up target
  - Particles/sparkles during merge
  - Glow effect on target dish

**Implementation**:
- Detect merge in `DropWhenNoLongerHeld::merge_dishes()`
- Create animation event or trigger animation system
- Use existing animation infrastructure

**System**: Create `RenderMergeAnimationSystem` or use existing animation system

### 2. Level-Up Sound Effect

**Sound**:
- Play sound when merge completes
- Sound indicates level-up occurred
- Use existing sound system

**Implementation**:
- Trigger sound in `merge_dishes()` after merge completes
- Use `raylib::PlaySound()` or existing sound system

### 3. "Ready to Merge!" Indicator

**Visual Hint**:
- Show indicator when 2+ same-type dishes in inventory
- Position: Near dishes that can merge
- Style: Glow, outline, or icon

**Implementation**:
- System scans inventory for mergeable pairs
- Renders indicator near mergeable dishes
- Updates in real-time

**System**: Create `RenderMergeHintSystem`

**Logic**:
```cpp
void render_merge_hints() {
  // Find dishes that can merge
  for (Entity &dish1 : inventory_dishes) {
    for (Entity &dish2 : inventory_dishes) {
      if (can_merge(dish1, dish2)) {
        render_hint_near_dish(dish1);
        render_hint_near_dish(dish2);
      }
    }
  }
}
```

### 4. Level Display Badge

**Visual**:
- Small number badge showing dish level
- Position: Top-right or bottom-right corner of dish sprite
- Style: Small circle/rectangle with level number

**Implementation**:
- Check `DishLevel.level` for each dish
- Render badge if `level > 1`
- Update when level changes

**System**: Create `RenderDishLevelBadgeSystem` or extend existing overlay system

**Rendering**:
```cpp
void render_level_badge(Entity &dish, Transform &transform) {
  if (!dish.has<DishLevel>()) return;
  const auto &level = dish.get<DishLevel>();
  if (level.level <= 1) return; // Don't show for level 1
  
  // Calculate badge position
  float badge_x = transform.position.x + transform.size.x - 15;
  float badge_y = transform.position.y + 5;
  
  // Draw badge background
  DrawCircle(badge_x, badge_y, 10, GRAY);
  
  // Draw level number
  DrawText(std::to_string(level.level).c_str(), badge_x - 5, badge_y - 5, 10, WHITE);
}
```

## UI/UX Considerations

### Merge Animation Timing

- Animation should complete before merge logic finishes
- User should see visual feedback immediately
- Animation shouldn't block gameplay

### Level Badge Visibility

- Badge should be visible but not obtrusive
- Consider opacity/transparency
- Ensure readable on all dish sprites

### "Ready to Merge!" Indicator

- Should be subtle (not distracting)
- Only show when merge is actually possible
- Update immediately when dishes added/removed

## Test Cases

### Test 1: Merge Animation

**Setup**: Merge two dishes

**Expected**:
- Animation plays during merge
- Dishes visually combine
- Animation completes smoothly

### Test 2: Level-Up Sound

**Setup**: Merge two dishes

**Expected**:
- Sound plays when merge completes
- Sound is appropriate for level-up

### Test 3: "Ready to Merge!" Indicator

**Setup**: Place 2 same-type dishes in inventory

**Expected**:
- Indicator appears near both dishes
- Indicator disappears when dishes merge or one removed

### Test 4: Level Display Badge

**Setup**: Create level 2+ dish

**Expected**:
- Badge shows level number
- Badge updates when level changes
- Badge hidden for level 1 dishes

## Validation Steps

1. **Add Merge Animation**
   - Create animation system or use existing
   - Trigger animation on merge
   - Test animation plays correctly

2. **Add Level-Up Sound**
   - Add sound file (or use existing)
   - Trigger sound on merge
   - Test sound plays

3. **Add "Ready to Merge!" Indicator**
   - Create `RenderMergeHintSystem`
   - Detect mergeable pairs
   - Render hints

4. **Add Level Display Badge**
   - Create `RenderDishLevelBadgeSystem`
   - Render badge for level 2+ dishes
   - Update when level changes

5. **MANDATORY CHECKPOINT**: Build
   - Run `xmake` - code must compile successfully

6. **MANDATORY CHECKPOINT**: Run Headless Tests
   - Run `./scripts/run_all_tests.sh` (ALL must pass)

7. **MANDATORY CHECKPOINT**: Run Non-Headless Tests
   - Run `./scripts/run_all_tests.sh -v` (ALL must pass)

8. **Verify Visual Enhancements**
   - Test merge animation works
   - Test sound plays
   - Test indicators appear correctly
   - Test level badges display

9. **MANDATORY CHECKPOINT**: Commit
   - `git commit -m "ui - add visual enhancements for dish merging"`

## Edge Cases

1. **Rapid Merges**: Multiple merges in quick succession
2. **Merge During Animation**: Handle merge while animation playing
3. **Level 1 Dishes**: No badge shown (expected)
4. **Headless Mode**: Visuals should be disabled in headless

## Success Criteria

- ✅ Merge animation plays during merge
- ✅ Level-up sound plays on merge
- ✅ "Ready to merge!" indicator shows when applicable
- ✅ Level badges display on level 2+ dishes
- ✅ All tests pass in headless and non-headless modes

## Estimated Time

4-6 hours:
- 1.5 hours: Merge animation
- 30 min: Level-up sound
- 1.5 hours: "Ready to merge!" indicator
- 1 hour: Level display badge
- 1 hour: Testing and polish

