# Remove merge_entity_arrays Code Smell - Updated Plan

## Root Cause Analysis

After analyzing the codebase and comparing with `kart-afterhours`:

1. **ECS Design**: Entities created via `EntityHelper::createEntity()` go into `temp_entities` array
2. **Automatic Merging**: `SystemManager::tick()` automatically calls `merge_entity_arrays()` after each system's `after(dt)` method (see `vendor/afterhours/src/core/system.h:371`)
3. **The Problem**: Code is manually calling `merge_entity_arrays()` 110+ times, which is a code smell

4. **Why kart-afterhours doesn't need it**: Systems automatically merge after each system runs, so entities created during one system are available to the next system automatically.

## Key Insights

1. **Use references directly**: When we create an entity with `createEntity()`, we get a reference - use it directly, don't query by ID
2. **Wait a frame when querying**: If we need to query for entities (don't have reference), wait a frame - system loop merges automatically
3. **force_merge only when necessary**: Only use `EntityQuery({.force_merge = true})` when we absolutely must query immediately in the same frame

## Patterns to Fix

1. **Unnecessary ID queries**: Create entity → store ID → merge → query by ID → use entity
   - **Better**: Create entity → use reference directly (no merge/query needed)

2. **Immediate queries after creation**: Create entity → immediately query for it
   - **Better**: Use reference directly, OR wait a frame (`app.pump_once(dt)` or `app.wait_for_frames(1)`)

3. **Cross-system queries**: System A creates entity → System B queries for it
   - **Better**: System loop merges automatically between systems, so next system can query normally

4. **Test patterns**: Create entity → merge → query
   - **Better**: Use reference directly, OR wait a frame, OR use `force_merge` only if truly needed

## Task 0: Create Validation Tests (BEFORE Removing Merges)

**Goal**: Create tests that validate current functionality works correctly, so we can ensure nothing breaks when we remove manual merges.

**Test File**: Create `src/testing/tests/ValidateEntityQueryWithoutManualMergeTest.h`

**Test Cases to Create**:

1. **Test: Use Entity Reference Directly (No Query Needed)**
   - Create entity with `createEntity()`
   - Use reference directly to add components/check state
   - Verify no merge or query needed
   - Expected: Reference works immediately, no merge required

2. **Test: Entities Queryable After One Frame**
   - Create entities directly in test
   - Call `app.wait_for_frames(1)` or `app.pump_once(dt)` to let system loop run
   - Query entities using normal `EntityQuery()` (no force_merge)
   - Verify entities are found
   - Expected: System loop merges automatically, entities queryable next frame

3. **Test: Battle Team Entities Queryable After System Loop**
   - Create battle team data (player + opponent)
   - Transition to Battle screen
   - Let `InstantiateBattleTeamSystem` create entities
   - Wait one frame (`app.wait_for_frames(1)`)
   - Verify `BattleTeamLoaderSystem` can query and tag those entities
   - Expected: Entities queryable after system loop merges them

4. **Test: Inventory Slots Queryable After Generation**
   - Transition to Shop screen
   - Let `GenerateInventorySlots` create slots
   - Wait one frame
   - Query for slots using normal `EntityQuery()`
   - Verify all 7 inventory slots + 1 sell slot exist
   - Expected: Slots queryable after system loop runs

5. **Test: Shop Slots Queryable After Generation**
   - Transition to Shop screen
   - Let `GenerateShopSlots` create slots
   - Wait one frame
   - Query for shop slots using normal `EntityQuery()`
   - Verify all 7 shop slots exist
   - Expected: Slots queryable after system loop runs

6. **Test: force_merge Works When Needed**
   - Create entities directly in test
   - Query them immediately using `EntityQuery({.force_merge = true})`
   - Verify entities are found
   - Expected: force_merge works for immediate queries (rare case)

7. **Test: Shop Items Queryable After System Runs**
   - Transition to Shop screen
   - Let `InitialShopFill` or `ProcessBattleRewards` create shop items
   - Wait one frame
   - Query for shop items using normal `EntityQuery()`
   - Verify items are findable
   - Expected: Items queryable after system loop runs

**Implementation Strategy**:
- These tests should pass WITH current code (with manual merges)
- After we remove manual merges, these tests should STILL pass (proving functionality preserved)
- Prefer: Use references directly > Wait a frame > Use force_merge (last resort)

**Validation**:
- Run tests: `./scripts/run_all_tests.sh` (especially the new validation test)
- All tests should pass with current code
- After removing merges, all tests should still pass

## Task 1: Remove Manual merge_entity_arrays Calls from Production Code

**Goal**: Remove all manual `merge_entity_arrays()` calls from production systems - systems should handle merging automatically.

**Files to fix**:
- `src/systems/GenerateInventorySlots.h` (line 65)
- `src/systems/InitCombatState.h` (lines 138, 181)
- `src/systems/ProcessBattleRewards.h` (line 109)
- `src/systems/InitialShopFill.h` (line 45)
- `src/systems/GameStateLoadSystem.h` (line 284)
- `src/systems/ServerBattleRequestSystem.h` (line 138)
- `src/systems/ReplayControllerSystem.h` (line 122)
- `src/systems/BattleTeamLoaderSystem.h` (line 50) - Review: uses `force_merge` in queries, merge might not be needed
- `src/systems/InstantiateBattleTeamSystem.h` (line 67)
- `src/systems/GenerateDishesGallery.h` (line 68)
- `src/systems/GenerateShopSlots.h` (line 49)
- `src/ui/ui_systems.cpp` (line 549)

**Strategy**:
- Systems that create entities should NOT manually merge - the system loop handles it
- If a system needs to query entities it just created in the same `once()` or `for_each()` call, use `EntityQuery({.force_merge = true})` instead
- Remove merge calls and verify systems still work (they should, since merge happens automatically)

## Task 2: Fix Tests to Use References/Wait Frames

**Goal**: Tests should use references directly or wait a frame instead of manually merging.

**Files to fix**:
- `src/testing/tests/ValidateEffectSystemTest.h` - Use references directly or wait frames
- `src/testing/tests/ValidateDishLevelContributionTest.h` - Remove 20+ merge calls, use references
- `src/testing/test_app.cpp` - Remove merge calls from helper methods (lines 348, 440, 475, etc.)
- All other test files with merge calls

**Strategy**:
- When tests create entities: Use the reference directly (no merge/query needed)
- When tests need to query entities they didn't create: Wait a frame (`app.wait_for_frames(1)`)
- Only use `EntityQuery({.force_merge = true})` when absolutely necessary (rare)

**Specific Fix for salmon test** (`test_salmon_neighbor_freshness_persists_to_combat`):
- The test creates entities (bagel0, salmon1, salmon2, bagel3) - use references directly
- Instead of merging and querying by ID, use the references directly
- If handlers need to query entities, use `force_merge` or wait a frame

## Task 3: Add Survivor Carryover Test

**File**: Create `src/testing/tests/ValidateSurvivorCarryoverTest.h`

**Goal**: Test that dishes surviving a course carry over to the next course correctly.

**Test Cases**:
1. Dish with remaining Body after opponent finishes should retarget next opponent
2. Transform position should update correctly on retarget
3. Multiple survivors should handle correctly
4. Battle should only complete when one team has no active dishes

**Implementation**: Use references directly or wait frames - no manual merges needed.

## Validation Steps

1. Build: `make`
2. Run headless tests: `./scripts/run_all_tests.sh`
3. Run non-headless tests: `./scripts/run_all_tests.sh -v`
4. Verify all tests pass, especially validation tests and `validate_effect_system`
5. Verify no warnings about temp entities in query logs
6. Commit: `git commit -m "refactor - remove manual merge_entity_arrays calls, use references and system loop"`

## Estimated Time

- Task 0: 2-3 hours (creating validation tests)
- Task 1: 2-3 hours (reviewing each production system)
- Task 2: 3-4 hours (fixing all tests)
- Task 3: 2-3 hours (creating new test)
- **Total**: 9-13 hours

