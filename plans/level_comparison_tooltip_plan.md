# Level Comparison & Progression Preview Tooltip Plan

## Overview

This plan details adding level comparison and progression preview to dish tooltips, showing players what stats and effects they'll gain when leveling up.

## Goal

Enhance tooltips to show:
- Current level stats and effects (already shown)
- Next level preview: what stats/effects will be at next level
- Highlight which effects unlock at higher levels
- Format: "Level 2: +2 Freshness (Level 3: +3 Freshness)"
- Show progression for all levels up to max level (level 4)

## Current State

### Existing Features

- ✅ Tooltip system exists (`src/tooltip.h`, `src/systems/TooltipSystem.h`)
- ✅ `generate_dish_tooltip_with_level()` shows current level info
- ✅ `get_dish_info(DishType type, int level)` supports level parameter
- ✅ Dish levels scale stats by 2x per level (level 2 = 2x, level 3 = 4x, level 4 = 8x)
- ✅ Effects can be different at different levels (some unlock at level 3, some scale)

### Missing Features

- ❌ Next level preview in tooltips
- ❌ Comparison between current and next level stats
- ❌ Highlighting of effects that unlock at higher levels
- ❌ Multi-level progression preview (showing all levels)

## Design Principles

### Display Format

**For Stats**:
- Show current level value, then next level value in parentheses
- Example: `Freshness: 2 (Level 3: 4)`
- If at max level, show only current: `Freshness: 8`

**For Effects**:
- Show current level effects normally
- Add preview line for next level if different: `[COLOR:TextMuted]Level 3: +1 Satiety to self`
- Highlight newly unlocked effects: `[COLOR:Success]Level 3: [NEW] +1 Satiety to self`
- If effects are the same but scaled, show: `Level 2: +1 Zing (Level 3: +2 Zing)`

**Section Organization**:
- Current level stats (as-is)
- Next level stat preview (new section)
- Current level effects (as-is)
- Next level effect preview (new section)

### Max Level Handling

- Max level is 4 (based on scaling: level 4 = 8x)
- If dish is at max level, don't show next level preview
- Show: `[COLOR:Warning]Max Level` indicator instead

## Implementation Details

### 1. Update `generate_dish_tooltip_with_level()` Function

**File**: `src/tooltip.h`

**Changes**:
- Add logic to fetch next level info using `get_dish_info(dishType, currentLevel + 1)`
- Compare current level vs next level stats
- Compare current level vs next level effects
- Generate comparison text for stats
- Generate preview text for effects

**Function Signature** (no change needed):
```cpp
inline std::string generate_dish_tooltip_with_level(
    DishType dishType,
    int level,
    int merge_progress,
    int merges_needed)
```

**New Helper Functions**:
- `format_stat_comparison(FlavorStats current, FlavorStats next, int currentLevel, int nextLevel)` - formats stat comparison
- `format_effect_comparison(const std::vector<DishEffect>& current, const std::vector<DishEffect>& next, int currentLevel, int nextLevel)` - formats effect comparison
- `effects_are_similar(const DishEffect& a, const DishEffect& b)` - checks if two effects are similar (same trigger/target, different amount)

### 2. Stat Comparison Logic

**Implementation**:
- Get current level stats: `get_dish_info(dishType, level).flavor`
- Get next level stats: `get_dish_info(dishType, level + 1).flavor`
- For each flavor stat that's > 0 in either level:
  - If current == next: Show only current (no change)
  - If current < next: Show `StatName: current (Level N+1: next)`
  - If at max level: Show only current

**Example Output**:
```
Flavor Stats:
  Satiety: 1 (Level 2: 2)
  Freshness: 2 (Level 3: 4)
```

**Note**: Stats scale by 2x per level, so level 2 = 2x base, level 3 = 4x base, level 4 = 8x base.

### 3. Effect Comparison Logic

**Implementation**:
- Get current level effects: `get_dish_info(dishType, level).effects`
- Get next level effects: `get_dish_info(dishType, level + 1).effects`
- Compare effects:
  - **Same effects, scaled amounts**: Show current, then preview with scaled amount
  - **New effects at next level**: Show `[COLOR:Success][NEW]` prefix
  - **Effects removed**: Don't show (effects don't get removed, only added/changed)
  - **Different effects**: Show both current and preview

**Example Output**:
```
Effects:
  OnServe: +1 Zing to self
  [COLOR:TextMuted]Level 2: +2 Zing to self
  [COLOR:Success][NEW] Level 3: OnBiteTaken: +1 Body to self
```

**Effect Comparison Algorithm**:
1. For each current effect, try to find matching effect in next level (same trigger, target, operation)
2. If found and amount differs: Show scaled preview
3. If found and same: Don't show preview (no change)
4. For each next level effect, check if it exists in current
5. If not found: It's a new effect, show with `[NEW]` prefix

### 4. Max Level Detection

**Implementation**:
- Check if `level >= 4` (max level)
- If at max level:
  - Don't fetch next level info
  - Don't show comparison sections
  - Add `[COLOR:Warning]Max Level` indicator after level display

### 5. Multi-Level Preview (Optional Enhancement)

**Future Enhancement**: Show all levels in one tooltip
- Format: `Level 1: No effects | Level 2: +1 Zing | Level 3: +2 Zing + Chain`
- This is more complex and can be added later if needed
- For now, focus on current → next level comparison

## Implementation Steps

### Step 1: Add Helper Functions to `tooltip.h`

**File**: `src/tooltip.h`

**Add Functions**:
1. `format_stat_comparison()` - compares and formats stat differences
2. `format_effect_comparison()` - compares and formats effect differences
3. `effects_are_similar()` - checks if two effects are similar

**Implementation**:
- `format_stat_comparison()`:
  - Takes current and next `FlavorStats`, current and next level
  - Returns formatted string showing comparison
  - Only shows stats that are > 0 in either level
  - Format: `StatName: current (Level N+1: next)` or just `StatName: current` if same

- `format_effect_comparison()`:
  - Takes current and next effect vectors, current and next level
  - Returns formatted string showing effect previews
  - Identifies new effects, scaled effects, unchanged effects
  - Format: `[COLOR:TextMuted]Level N+1: effect description` or `[COLOR:Success][NEW] Level N+1: effect description`

- `effects_are_similar()`:
  - Compares two `DishEffect` objects
  - Returns true if same trigger, target, operation (amount can differ)
  - Used to identify scaled vs new effects

### Step 2: Update `generate_dish_tooltip_with_level()`

**File**: `src/tooltip.h`

**Changes**:
1. After showing current level stats, add next level stat preview section
2. After showing current level effects, add next level effect preview section
3. Check for max level and skip preview if at max
4. Use helper functions to generate comparison text

**Structure**:
```cpp
// Current level info (existing)
tooltip << "[COLOR:Gold]" << dishInfo.name << "\n";
tooltip << "[COLOR:Warning]Level: " << level;
if (level >= 4) {
  tooltip << " [COLOR:Warning](Max Level)";
}
tooltip << "\n";

// Current stats (existing)
tooltip << "Flavor Stats:\n";
// ... existing stat display ...

// Next level stat preview (NEW)
if (level < 4) {
  auto nextInfo = get_dish_info(dishType, level + 1);
  tooltip << format_stat_comparison(dishInfo.flavor, nextInfo.flavor, level, level + 1);
}

// Current effects (existing)
if (!dishInfo.effects.empty()) {
  tooltip << "\n[COLOR:Gold]Effects:\n";
  // ... existing effect display ...
}

// Next level effect preview (NEW)
if (level < 4) {
  auto nextInfo = get_dish_info(dishType, level + 1);
  if (!nextInfo.effects.empty()) {
    tooltip << format_effect_comparison(dishInfo.effects, nextInfo.effects, level, level + 1);
  }
}
```

### Step 3: Handle Edge Cases

**Edge Cases**:
1. **Level 1 → Level 2**: Show preview (most common case)
2. **Level 2 → Level 3**: Show preview
3. **Level 3 → Level 4**: Show preview
4. **Level 4 (max)**: Don't show preview, show "Max Level"
5. **No effects at current level, effects at next**: Show all next level effects as `[NEW]`
6. **Effects at current, no effects at next**: Don't show preview (shouldn't happen, but handle gracefully)
7. **Same effects, same amounts**: Don't show preview (no change)

### Step 4: Testing

**Test Cases**:
1. **Potato (Level 1)**: 
   - Current: No effects
   - Next: Should show Level 3 unlocks +1 Satiety effect
   - Stats: Should show stat scaling preview
2. **Salmon (Level 1)**:
   - Current: OnServe chain effect
   - Next: Should show scaled effect amounts
3. **Any dish at Level 4**:
   - Should show "Max Level" indicator
   - Should not show next level preview
4. **Dish with progressive effects**:
   - Verify new effects show with `[NEW]` prefix
   - Verify scaled effects show preview

**Validation**:
- Build: `xmake`
- Run headless tests: `./scripts/run_all_tests.sh`
- Run non-headless tests: `./scripts/run_all_tests.sh -v`
- Manual test: Hover over dishes at different levels in shop/inventory
- Verify tooltip shows correct comparisons

## Files to Modify

1. **`src/tooltip.h`**:
   - Add helper functions: `format_stat_comparison()`, `format_effect_comparison()`, `effects_are_similar()`
   - Update `generate_dish_tooltip_with_level()` to include next level preview

## Example Output

### Example 1: Potato at Level 1
```
[COLOR:Gold]Potato
[COLOR:Info]Price: 3 coins
[COLOR:Warning]Level: 1
Flavor Stats:
  Satiety: 1 (Level 2: 2)
[COLOR:Gold]Effects:
  (no effects)
[COLOR:TextMuted]Level 3: OnServe: +1 Satiety to self
```

### Example 2: Salmon at Level 2
```
[COLOR:Gold]Salmon
[COLOR:Info]Price: 3 coins
[COLOR:Warning]Level: 2
Flavor Stats:
  Freshness: 2 (Level 3: 4)
[COLOR:Gold]Effects:
  OnServe: +2 Freshness to self and adjacent (if adjacent has Freshness, chain)
  [COLOR:TextMuted]Level 3: +3 Freshness to self and adjacent (if adjacent has Freshness, chain)
  [COLOR:TextMuted]Level 3: OnCourseStart: +2 Body to self
```

### Example 3: Any dish at Level 4
```
[COLOR:Gold]Steak
[COLOR:Info]Price: 3 coins
[COLOR:Warning]Level: 4 (Max Level)
Flavor Stats:
  Satiety: 8
  Umami: 8
[COLOR:Gold]Effects:
  OnServe: +5 Zing to self
  OnBiteTaken: +4 Body to self
  ...
```

## Success Criteria

- ✅ Tooltip shows current level stats and effects (existing functionality preserved)
- ✅ Tooltip shows next level stat preview when not at max level
- ✅ Tooltip shows next level effect preview when not at max level
- ✅ New effects are highlighted with `[NEW]` prefix
- ✅ Scaled effects show preview with next level amount
- ✅ Max level dishes show "Max Level" indicator
- ✅ All tests pass in headless and non-headless modes
- ✅ Tooltip formatting is readable and not cluttered

## Estimated Time

**2-3 hours** (as specified in `next_todo.md`)

## Notes

- This enhancement helps players understand progression and plan their merges
- Shows value of leveling up dishes
- Highlights when effects unlock (important for strategic planning)
- Can be extended later to show all levels in one tooltip if needed

