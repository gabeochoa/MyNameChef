# Survivor Carryover Test Plan

## Overview

This plan details the implementation of `ValidateSurvivorCarryoverTest.h` to test that dishes surviving a course correctly carry over to the next course with proper retargeting and position updates.

## Goal

Test that dishes that survive a course (have remaining Body after opponent finishes) correctly:
1. Retarget the next opponent in queue
2. Maintain correct slot position (visual and internal index)
3. Handle multiple survivors appropriately
4. Only complete battle when one team has no active dishes

## Current System Behavior

### How Survivor Carryover Works

1. **Course Completion**: When one dish in a course is defeated (Body <= 0), `ResolveCombatTickSystem` marks it as `Finished` and calls `reorganize_queues()`

2. **Queue Reorganization** (`ResolveCombatTickSystem::reorganize_queues()`):
   - Finds all active dishes (phase: `InQueue`, `Entering`, or `InCombat`)
   - Renumbers them sequentially starting from index 0
   - Updates `queue_index` for each dish
   - Updates `Transform.position` based on new `queue_index`:
     - Player side: `y = 150.0f`, `x = 120.0f + queue_index * 100.0f`
     - Opponent side: `y = 500.0f`, `x = 120.0f + queue_index * 100.0f`
   - Resets survivors from `InCombat` to `InQueue` phase
   - Clears combat state: `enter_progress = 0.0f`, `first_bite_decided = false`, resets cadence

3. **Next Course Start** (`StartCourseSystem`):
   - Finds dishes at `queue_index == 0` for both teams
   - Starts combat between them
   - Survivors from previous course (now at index 0) fight the next opponent

4. **Battle Completion** (`AdvanceCourseSystem`):
   - Checks if both tracked dishes for current course are finished
   - Counts active dishes (phase != `Finished`) for both teams
   - Battle completes only when one team has zero active dishes OR max courses reached

### Key Components

- `DishBattleState`: Tracks `queue_index`, `phase`, `team_side`, `enter_progress`, `bite_cadence`
- `CombatStats`: Tracks `currentBody`, `currentZing`
- `Transform`: Tracks visual position (`position.x`, `position.y`)
- `CombatQueue`: Tracks `current_index`, `total_courses`, `current_player_dish_id`, `current_opponent_dish_id`

## Test Cases

### Test Case 1: Single Survivor Retargets Next Opponent

**Setup**:
- Player: 1 dish (high Body, low Zing) at slot 0
- Opponent: 2 dishes (low Body, high Zing) at slots 0 and 1

**Expected Behavior**:
1. Course 1: Player dish defeats opponent dish 0, survives with remaining Body
2. Reorganization: Player dish stays at index 0, opponent dish 1 moves to index 0
3. Course 2: Player dish (survivor) fights opponent dish 1 (now at index 0)
4. Transform positions update correctly for both dishes
5. Battle completes after course 2 (opponent has no more dishes)

**Validation Points**:
- After course 1: Player dish has `currentBody > 0`, phase is reset to `InQueue`
- After reorganization: Player dish `queue_index == 0`, opponent dish 1 `queue_index == 0`
- Transform positions: Player at `(120, 150)`, Opponent at `(120, 500)`
- Course 2 starts: Both dishes enter `InCombat` phase
- Battle completes: Opponent team has 0 active dishes

### Test Case 2: Transform Position Updates (Visual and Index)

**Setup**:
- Player: 2 dishes at slots 0 and 1
- Opponent: 3 dishes at slots 0, 1, 2

**Expected Behavior**:
1. Course 1: Player dish 0 defeats opponent dish 0, survives
2. Reorganization: 
   - Player dish 0 stays at index 0
   - Player dish 1 stays at index 1
   - Opponent dish 1 moves from index 1 → 0
   - Opponent dish 2 moves from index 2 → 1
3. Transform positions update:
   - Player dish 0: `(120, 150)` (unchanged)
   - Player dish 1: `(220, 150)` (unchanged)
   - Opponent dish 1: `(120, 500)` (was `(220, 500)`)
   - Opponent dish 2: `(220, 500)` (was `(320, 500)`)

**Validation Points**:
- `queue_index` values match expected positions
- `Transform.position` matches calculated positions based on `queue_index`
- Visual positions (sprite transforms) are updated

### Test Case 3: Multiple Survivors All Retarget Correctly

**Setup**:
- Player: 3 dishes (all high Body, low Zing) at slots 0, 1, 2
- Opponent: 4 dishes (all low Body, high Zing) at slots 0, 1, 2, 3

**Expected Behavior**:
1. Course 1: All 3 player dishes defeat their opponents, all survive
2. Reorganization:
   - Player dishes 0, 1, 2 stay at indices 0, 1, 2
   - Opponent dish 3 moves from index 3 → 0
3. Course 2: Player dish 0 (survivor) fights opponent dish 3 (now at index 0)
4. Course 3: Player dish 1 (survivor) fights... (opponent has no more dishes, battle should complete)

**Validation Points**:
- After course 1: All 3 player dishes have `currentBody > 0`, all reset to `InQueue`
- After reorganization: All player dishes maintain correct indices, opponent dish 3 at index 0
- Course 2 starts correctly with player dish 0 vs opponent dish 3
- Battle completes when opponent has no active dishes

### Test Case 4: Battle Completion Only When Team Exhausted

**Setup**:
- Player: 2 dishes at slots 0 and 1
- Opponent: 2 dishes at slots 0 and 1

**Expected Behavior**:
1. Course 1: Player dish 0 defeats opponent dish 0, survives
2. Reorganization: Opponent dish 1 moves to index 0
3. Course 2: Player dish 0 (survivor) fights opponent dish 1
4. Battle should NOT complete after course 1 (opponent still has dish 1)
5. Battle completes after course 2 when opponent has no active dishes

**Validation Points**:
- After course 1: `CombatQueue.complete == false` (opponent still has active dishes)
- After course 2: `CombatQueue.complete == true` (opponent has 0 active dishes)
- Active dish counts are correct at each stage

### Test Case 5: Simultaneous Defeat (Both Dishes Die)

**Setup**:
- Player: 1 dish (medium Body, medium Zing) at slot 0
- Opponent: 2 dishes (medium Body, medium Zing) at slots 0 and 1

**Expected Behavior**:
1. Course 1: Both dishes defeat each other simultaneously (both Body <= 0 in same tick)
2. Both should be marked `Finished`
3. Reorganization: Both teams skip the finished dishes
4. Next course should start with next dishes in queue (if any)
5. Battle completes when one team has no active dishes

**Validation Points**:
- After course 1: Both dishes have `phase == Finished`
- Reorganization: Both dishes are skipped (not renumbered)
- Next course starts with next dishes in queue (if available)
- Battle completion logic handles simultaneous defeat correctly

**Setup**:
- Player: 2 dishes at slots 0 and 1
- Opponent: 2 dishes at slots 0 and 1

**Expected Behavior**:
1. Course 1: Player dish 0 defeats opponent dish 0, survives
2. Reorganization: Opponent dish 1 moves to index 0
3. Course 2: Player dish 0 (survivor) fights opponent dish 1
4. Battle should NOT complete after course 1 (opponent still has dish 1)
5. Battle completes after course 2 when opponent has no active dishes

**Validation Points**:
- After course 1: `CombatQueue.complete == false` (opponent still has active dishes)
- After course 2: `CombatQueue.complete == true` (opponent has 0 active dishes)
- Active dish counts are correct at each stage

## Implementation Details

### Test File Structure

**File**: `src/testing/tests/ValidateSurvivorCarryoverTest.h`

**Test Functions**: Create separate `TEST()` functions for each test case (not sequential):
- `TEST(validate_survivor_carryover_single)` - Test Case 1
- `TEST(validate_survivor_carryover_positions)` - Test Case 2
- `TEST(validate_survivor_carryover_multiple)` - Test Case 3
- `TEST(validate_survivor_carryover_battle_completion)` - Test Case 4
- `TEST(validate_survivor_carryover_simultaneous_defeat)` - Test Case 5 (new edge case)

### Helper Functions Needed

**Step 0: Check Existing Helpers** (Do this first, as separate commit if needed)

Check if these helper methods already exist in `src/testing/test_app.h`:
- `wait_for_course_complete(course_index, timeout)`
- `expect_dish_at_index(entity_id, expected_index, side)`
- `expect_dish_position(entity_id, expected_x, expected_y)` 
- `expect_dish_body(entity_id, expected_body_min)`
- `expect_battle_not_complete()`
- `expect_battle_complete()`
- `expect_active_dish_count(side, expected_count)`

**If helpers don't exist**: Create them in `test_app.h`/`test_app.cpp` as a **separate commit** before implementing the test.

**Course Completion Detection**:
- Investigate how to detect course completion (check `CombatQueue.current_index` and dish phases)
- Consider adding helper method or `OnCourseComplete` event that tests can subscribe to
- Look at existing battle completion detection patterns

1. **Set Dish Stats Helper**:
   - `app.set_dish_combat_stats(dish_id, body, zing)` - Helper to set Body/Zing values without branching
   - **TODO**: Add badge system so tests can request permission to write (makes it easier to track who's using it)

2. **Wait for Course Completion**:
   - Investigate course completion detection (monitor `CombatQueue.current_index` and dish state)
   - Can add helper method or consider `OnCourseComplete` event subscription approach
   - Look at how `wait_for_battle_complete()` works for patterns

3. **Validate Dish State**:
   - `expect_dish_at_index(entity_id, expected_index, side)` - Validates `queue_index` matches expected
   - `expect_dish_position(entity_id, expected_x, expected_y, epsilon)` - Validates `Transform.position` with tolerance (resolution-based strategy needed)
   - `expect_dish_phase(entity_id, expected_phase)` - Already exists: `expect_dish_phase()`
   - `expect_dish_body(entity_id, expected_body_min)` - Validates `CombatStats.currentBody >= min`

4. **Validate Battle State**:
   - `expect_battle_not_complete()` - Validates `CombatQueue.complete == false` OR check active dish counts
   - `expect_battle_complete()` - Validates `CombatQueue.complete == true` OR check active dish counts (see how game detects it in `ServerContext::is_battle_complete()`)
   - `expect_active_dish_count(side, expected_count)` - Counts active dishes (phase != `Finished`)

**Position Validation Strategy**:
- Position validation is resolution-based, so need better strategy than exact equality
- Consider using epsilon tolerance (e.g., `abs(actual - expected) < 1.0f`) or relative tolerance
- May need to account for screen resolution differences

**Reorganization Timing**:
- Use `app.wait_for_frames(2)` after course completion (what other tests do, usually enough)
- **TODO**: Setup app API for `wait_until()` or similar conditional wait mechanism

### Test Implementation Pattern

Following PROJECT_RULES.md test structure requirements:
- **NO branching logic** - Use assertions only
- **NO early returns** - Tests run to completion
- Use `app.wait_for_*()` methods for sequential operations
- Use `app.expect_*()` methods for validation

### Example Test Flow (Test Case 1)

**Note**: Use helper method `app.set_dish_combat_stats(dish_id, body, zing)` to set Body/Zing values (avoids branching).

**Body/Zing Math**: Calculate values to ensure:
- Player dishes survive course 1 (high Body, low Zing = slow but durable)
- Opponent dishes die quickly (low Body, high Zing = fast but fragile)
- Example: Player Body=20, Zing=2 vs Opponent Body=5, Zing=10
  - Opponent deals 10 damage per tick, player has 20 Body = 2 ticks to kill player
  - Player deals 2 damage per tick, opponent has 5 Body = 3 ticks to kill opponent
  - Opponent dies first, player survives

```cpp
TEST(validate_survivor_carryover_single) {
  app.wait_for_frames(1);
  
  // Setup: Create battle with specific dish configurations
  int player_dish_id = app.create_dish(DishType::Potato)
    .on_team(DishBattleState::TeamSide::Player)
    .at_slot(0)
    .with_combat_stats()
    .commit();
  
  // Use helper to set Body/Zing (no branching)
  app.set_dish_combat_stats(player_dish_id, 20, 2);  // High body, low zing
  
  int opponent_dish_0_id = app.create_dish(DishType::Potato)
    .on_team(DishBattleState::TeamSide::Opponent)
    .at_slot(0)
    .with_combat_stats()
    .commit();
  
  app.set_dish_combat_stats(opponent_dish_0_id, 5, 10);  // Low body, high zing
  
  int opponent_dish_1_id = app.create_dish(DishType::Potato)
    .on_team(DishBattleState::TeamSide::Opponent)
    .at_slot(1)
    .with_combat_stats()
    .commit();
  
  app.set_dish_combat_stats(opponent_dish_1_id, 5, 10);
  
  app.wait_for_frames(1);  // Let entities merge
  
  // Start battle - investigate how dishes created with .on_team()/.at_slot() 
  // are recognized by battle systems (may need manual setup)
  app.start_battle();
  app.wait_for_battle_initialized(10.0f);
  
  // Wait for course 1 to complete (use helper or detect via CombatQueue.current_index)
  app.wait_for_course_complete(0, 30.0f);
  
  // Validate: Player dish survived
  app.expect_dish_body(player_dish_id, 1, "Player dish should have remaining Body");
  app.expect_dish_phase(player_dish_id, DishBattleState::Phase::InQueue, "Survivor should be reset to InQueue");
  
  // Wait for reorganization (2 frames is what other tests do)
  app.wait_for_frames(2);
  
  // Validate: Positions updated correctly (use epsilon for position validation)
  app.expect_dish_at_index(player_dish_id, 0, DishBattleState::TeamSide::Player, "Player dish should stay at index 0");
  app.expect_dish_at_index(opponent_dish_1_id, 0, DishBattleState::TeamSide::Opponent, "Opponent dish 1 should move to index 0");
  app.expect_dish_position(player_dish_id, 120.0f, 150.0f, 1.0f, "Player dish position");  // epsilon = 1.0f
  app.expect_dish_position(opponent_dish_1_id, 120.0f, 500.0f, 1.0f, "Opponent dish 1 position");
  
  // Validate: Battle not complete yet (check CombatQueue.complete or active dish counts)
  app.expect_battle_not_complete("Battle should continue with opponent dish remaining");
  
  // Wait for course 2 to start
  app.wait_for_dish_phase(player_dish_id, DishBattleState::Phase::InCombat, 10.0f, "Survivor should enter combat with next opponent");
  
  // Wait for course 2 to complete
  app.wait_for_course_complete(1, 30.0f);
  
  // Validate: Battle complete (see how game detects it - check CombatQueue.complete or active dish counts)
  app.expect_battle_complete("Battle should complete when opponent has no dishes");
  app.expect_active_dish_count(DishBattleState::TeamSide::Opponent, 0, "Opponent should have no active dishes");
}
```

**Reference Other Tests**: Look at `ValidateFullBattleFlowTest.h` and other passing tests for patterns on:
- Battle initialization flow
- How dishes are set up for battle
- Waiting patterns
- Validation patterns

### Headless Mode Considerations

From memories and code analysis:
- Slide-in animations complete instantly in headless mode (`slide_in == 1.0`)
- `enter_progress` is set to `1.0` immediately in headless
- Use `app.wait_for_frames(1)` after entity creation for system processing
- Tests should work in both headless and non-headless modes

### Logging for Debugging

**Only add logging if needed to debug the test** - not required for final implementation.

If debugging is needed, use existing log patterns:
- `log_info("COMBAT: ...")` for combat events
- `log_info("COMBAT_ADVANCE: ...")` for course advancement
- `log_info("TEST_SURVIVOR: ...")` for test-specific events

## Validation Steps

**Step 0: Check and Create Helper Methods** (Separate Commit)
1. Check if helper methods exist in `test_app.h`/`test_app.cpp`
2. If missing, implement:
   - `set_dish_combat_stats(dish_id, body, zing)` - Set Body/Zing without branching
   - `wait_for_course_complete(course_index, timeout)` - Wait for course completion
   - `expect_dish_at_index(entity_id, expected_index, side)` - Validate queue_index
   - `expect_dish_position(entity_id, expected_x, expected_y, epsilon)` - Validate position with tolerance
   - `expect_dish_body(entity_id, expected_body_min)` - Validate Body >= min
   - `expect_battle_not_complete()` - Validate battle not complete
   - `expect_battle_complete()` - Validate battle complete
   - `expect_active_dish_count(side, expected_count)` - Count active dishes
3. **MANDATORY CHECKPOINT**: Build and commit helpers separately
   - `git commit -m "be - add test helpers for survivor carryover test"`

1. **Create Test File**: `src/testing/tests/ValidateSurvivorCarryoverTest.h`
   - Implement all 5 test cases as separate `TEST()` functions
   - Use helper functions for setup and validation
   - Follow PROJECT_RULES.md test structure (no branching, assertions only)
   - Reference other passing tests for patterns (`ValidateFullBattleFlowTest.h`, etc.)

2. **MANDATORY CHECKPOINT**: Build
   - Run `make` - code must compile successfully
   - Fix any compilation errors

3. **MANDATORY CHECKPOINT**: Run Headless Tests
   - Run `./scripts/run_all_tests.sh` (default headless mode)
   - ALL tests must pass (including new survivor carryover test)
   - Verify test output shows all 4 test cases passing

4. **MANDATORY CHECKPOINT**: Run Non-Headless Tests
   - Run `./scripts/run_all_tests.sh -v` (non-headless mode)
   - ALL tests must pass
   - Verify visual behavior matches expected (dishes move to correct positions)

5. **Verify Test Coverage**:
   - Test Case 1: Single survivor retargets correctly ✓
   - Test Case 2: Transform positions update correctly ✓
   - Test Case 3: Multiple survivors handle correctly ✓
   - Test Case 4: Battle completion only when team exhausted ✓
   - Test Case 5: Simultaneous defeat handled correctly ✓

6. **MANDATORY CHECKPOINT**: Commit
   - `git commit -m "add survivor carryover test coverage"`
   - Only commit after all tests pass in both modes

7. **Only after successful commit, proceed to Task 2**

## Edge Cases to Consider

1. **Tie Scenario**: Both dishes defeat each other simultaneously
   - Both should be marked `Finished`
   - Reorganization should skip both
   - Next course should start with next dishes in queue

2. **Survivor with Very Low Body**: Dish survives with 1 Body
   - Should still carry over correctly
   - Should be able to fight next opponent (may die quickly)

3. **All Dishes Survive**: Unlikely but possible if both teams have very high Body
   - All dishes should reorganize correctly
   - Next course should start with dishes at index 0

4. **Empty Queue After Reorganization**: One team has no dishes left
   - Battle should complete immediately
   - `CombatQueue.complete` should be `true`

## Success Criteria

- All 5 test cases pass in both headless and non-headless modes
- Test validates:
  - ✅ Single survivor retargets next opponent correctly
  - ✅ Transform position (visual and index) updates correctly on retarget
  - ✅ Multiple survivors all retarget correctly
  - ✅ Battle completion only occurs when one team exhausted
- Test file follows PROJECT_RULES.md structure (no branching, assertions only)
- Code compiles without warnings
- All existing tests still pass

## Estimated Time

3-4 hours:
- 30 min: Check existing helpers, create missing ones (separate commit)
- 1 hour: Test file creation and test case implementation
- 30 min: Investigate battle initialization flow and course completion detection
- 30 min: Debugging and validation
- 30 min: Headless/non-headless testing and fixes

## TODOs for Future Improvements

1. **Badge System for Test Write Permissions**: Add system so tests can request permission to write (makes it easier to track who's using write operations)

2. **Wait Until API**: Setup app API for `wait_until()` or similar conditional wait mechanism (better than fixed frame delays)

3. **Position Validation Strategy**: Improve position validation to handle resolution differences better

