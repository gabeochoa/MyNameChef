
first try is in stash "wip-survivor-carryover-all-dishes-appear-issue" 


# Test Issue Analysis: validate_effect_system - test_salmon_neighbor_freshness_persists_to_combat

## Problem
The test `test_salmon_neighbor_freshness_persists_to_combat` is failing because the Salmon's `onServe` handler cannot find `bagel0` when querying for adjacent dishes, even though the entity exists and is accessible.

## Root Cause
In production, the game loop automatically calls `EntityHelper::merge_entity_arrays()` after each system completes (see `vendor/afterhours/src/core/system.h:371`). This ensures that:

1. When `BattleTeamLoaderSystem` creates entities, they go to `temp_entities`
2. After the system finishes, entities are merged into the main array
3. When `TriggerDispatchSystem` runs, all entities are accessible
4. When the Salmon handler's `EQ()` queries run, they capture a complete entity list

In the test, we're calling `TriggerDispatchSystem` directly without going through the system loop, so we need to manually ensure entities are merged. However, there's a subtle timing issue: `EntityQuery` constructor captures `EntityHelper::get_entities()` at construction time (line 310-311 of `entity_query.h`). If entities aren't fully merged when the query is constructed, they won't be found.

## Current Test Setup
The test currently:
1. Creates entities via `add_dish_to_menu()` (entities go to temp)
2. Merges entities manually
3. Creates trigger queue and adds events
4. Merges again
5. Calls `TriggerDispatchSystem.for_each_with()` directly
6. Merges after dispatch
7. Re-queries bagel0 to verify it has `DeferredFlavorMods`

## Key Observations
- Entities ARE accessible before dispatch (pre-check query finds bagel0)
- Handler is being called (we see TRIGGER logs)
- Handler queries are NOT finding bagel0 during execution
- After dispatch, bagel0 still doesn't have `DeferredFlavorMods`

## Hypothesis
The handler's queries (`EQ()` calls on lines 46, 65, 93, 110 of `dish_types.cpp`) are constructed with entity lists that don't include bagel0, even though entities were merged before dispatch. This could be because:

1. **Query caching**: The `EntityQuery` might cache results, but this doesn't match the code
2. **Entity list capture timing**: The query captures entities at construction, but something happens between merge and query construction
3. **Component requirements**: bagel0 might be missing a component the query needs (but pre-check found it, so this is unlikely)

## Potential Solutions

### Option 1: Use QueryOptions with force_merge
We could modify the test to use `EQ({.force_merge = true})` when verifying, but this doesn't help the handler which doesn't use these options.

### Option 2: Ensure merge happens right before handler queries
Since the handler queries are constructed inside the handler (when `EQ()` is called), we need to ensure entities are merged BEFORE the handler is called. We're doing this, but maybe we need to merge right before EACH handler call, not just once before dispatch.

### Option 3: Match production system loop more closely
Instead of calling `TriggerDispatchSystem` directly, we could simulate the system loop behavior by:
1. Calling a system
2. Merging entities
3. Calling next system
4. Merging entities

This would more closely match production, but might be more complex.

### Option 4: Add logging to handler (temporary)
Add temporary logging to the Salmon handler to see exactly what queries find, then remove it. This would help diagnose the issue but requires modifying production code temporarily.

## Recommendation
The test should more closely simulate the production flow. Since production merges after each system, and the handler's queries are constructed during handler execution, we should ensure that:

1. All entities are created and merged BEFORE any queries run
2. The trigger dispatch happens in a way that mirrors the system loop
3. Consider if there are any other differences between test and production entity setup

Since the handler works in production, the issue is definitely in how the test is set up. The entities exist and are merged, but the handler's queries don't find them during execution.

---

# Battle Survivor Carryover Issue: All Dishes in Battle Simultaneously

## Problem
When survivor carryover occurs during battle, all dishes are appearing in battle at the same time instead of one course at a time. Dishes should fight head-to-head sequentially, with each new course starting when dishes from the next slot slide in to fight.

## Root Cause Analysis

### Issue 1: Transform Position Not Updated on Retargeting
When dishes are retargeted during survivor carryover (moved from their original slot to the current slot), we update `queue_index` but not `Transform.position`. This causes:
- Dishes to remain at their original visual positions (e.g., slot 1 = x=220, slot 2 = x=320)
- But logically they're all at slot 0 (queue_index = 0)
- Visually all dishes appear to be fighting simultaneously because they're at different X positions

**Location**: `StartCourseSystem.h` - survivor carryover retargeting logic (line ~175)

### Issue 2: Dishes Not Restored After Course Completion
When a course finishes and advances to the next course:
- Dishes that were retargeted to slot 0 remain at slot 0 (as Finished)
- When course 1 tries to start, it looks for dishes at slot 1
- But dishes from slot 1 were retargeted to slot 0, so they're not found
- Battle progression stalls or behaves incorrectly

**Location**: `AdvanceCourseSystem.h` - course advancement logic

### Issue 3: Visual vs Logical Position Mismatch
- `queue_index` controls logical battle slot (which dishes fight together)
- `Transform.position` controls visual position (where dish appears on screen)
- When retargeting, we change logical position but not visual position
- This breaks the intended visual flow where dishes slide in from their queue positions

## Current Behavior
1. Course 0 starts: dishes at slot 0 fight head-to-head ✓
2. Survivor carryover: dishes from slot 1, 2, 3, etc. get retargeted to slot 0
   - `queue_index` changes to 0 ✓
   - `Transform.position` stays at original position (slot 1, 2, 3...) ✗
   - Visually all dishes appear at once ✗
3. Course 0 finishes: all dishes at slot 0 are Finished
4. Course 1 should start: looks for dishes at slot 1
   - But slot 1 dishes were retargeted to slot 0 ✗
   - No dishes found at slot 1 ✗
   - Battle can't continue properly ✗

## Expected Behavior
1. Course 0: dishes at slot 0 fight
2. Survivor carryover: ONE dish at a time is retargeted to slot 0 (as opponents are defeated)
   - Visual position should update to slot 0's position (x=120)
   - Only one dish should be visible fighting at a time
3. Course 0 finishes: all dishes at slot 0 are Finished
4. Course 1 starts: dishes at slot 1 slide in from their queue positions
   - Dishes that were retargeted should be restored to original slots OR
   - Only the current course's dishes should be active

## Proposed Solutions

### Solution 1: Update Transform Position on Retargeting
**Implemented**: When retargeting a dish, update `Transform.position` to match the current slot's position:
- X position: `120.0f + current_index * 100.0f` (all retargeted dishes at slot 0 = x=120)
- Y position: `150.0f` for player, `500.0f` for opponent
- This ensures retargeted dishes appear at the correct visual position

### Solution 2: Restore Dishes After Course Completion
**Partially Implemented**: When a course finishes, restore retargeted dishes to their original slots:
- Find Finished dishes at the finished slot with `original_queue_index >= 0`
- Restore `queue_index` to `original_queue_index`
- Restore `Transform.position` to original slot position
- This ensures dishes are available at correct slots for next course

**Issue with current implementation**: Only restores Finished dishes. But if a dish survived and is still active, it needs different handling.

### Solution 3: Alternative Approach - Don't Retarget, Use Combat Matching
Instead of changing `queue_index`, we could:
- Keep dishes at their original slots
- Match survivor dishes with next available opponent by searching all slots
- Only change visual position/state, not logical slot
- This avoids the restore step entirely

**Tradeoff**: More complex matching logic, but cleaner separation of concerns

## Implementation Status

### Completed
- ✅ Added `original_queue_index` field to track original slot before retargeting
- ✅ Modified `find_dish_for_slot` and `find_opponent_dish` to use queue order (not entity ID)
- ✅ Fixed `both_dishes_finished` to check all dishes at slot (handle retargeted dishes)
- ✅ Added Transform position update when retargeting (moves dish to slot 0 position)
- ⚠️ Added restore logic in `AdvanceCourseSystem` (may need refinement)

### Pending/Issues
- ⚠️ Restore logic only handles Finished dishes - need to handle active survivors
- ⚠️ Visual flow: when retargeted, dishes should still animate/slide into battle center
- ❓ Should retargeted dishes visually appear at slot 0 position immediately, or animate from original position?
- ❓ When course finishes, should we restore ALL retargeted dishes, or only the ones needed for next course?

## Key Files Modified
- `src/components/dish_battle_state.h` - Added `original_queue_index` field
- `src/systems/StartCourseSystem.h` - Survivor carryover retargeting logic, Transform position update
- `src/systems/ResolveCombatTickSystem.h` - Opponent finding uses queue order
- `src/systems/AdvanceCourseSystem.h` - Course completion detection and dish restoration

## Testing
- `validate_survivor_carryover` test passes (headless mode)
- Normal gameplay: dishes appear all at once instead of sequentially
- Need to verify course advancement works correctly after fixes
