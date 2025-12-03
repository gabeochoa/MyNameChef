# Test Refactoring Plan

## At-a-Glance
- **Sequence:** 06 / 21 — prerequisites for reliable combat/effect automation.
- **Goal:** Centralize repeated battle/test helpers so new suites are trivial to write and maintain.
- **Status:** Analysis done; implementation waiting on engineering bandwidth (no code changes yet).
- **Dependencies:** Must align with Plan `07_COMBAT_IMPLEMENTATION_STATUS.md` (shared battle setup expectations) and `03_TEMP_ENTITIES...` for eventual API shifts.
- **KPI:** Reduce duplicated helper LOC by ≥70% and cut average combat test authoring time in half.

## Work Breakdown
|Step|Description|Deliverables|Exit Criteria|
|---|---|---|---|
|1. Scaffold Shared Helpers|Create `src/testing/test_battle_helpers.h/.cpp` with singleton + trigger utilities|Helper files, namespace contract, initial docs|Unit tests for helpers + adoption guide|
|2. Mass Refactor High-usage Tests|Update listed suites (Paella → TriggerOrdering) to consume new helpers|Per-file diff + before/after LOC stats|`./scripts/run_all_tests.sh` passes, no regressions|
|3. Optional TestApp APIs|Add thin wrappers (`TestApp::push_trigger_event`, etc.) if repeated patterns remain|New methods + docs|Only merge if ≥3 suites benefit|
|4. Enforce via Lint|Add CI check (clang-tidy or custom script) that flags local copies of helper patterns|CI script + documentation|CI fails when prohibited helpers added|

## Key Findings

Many tests in `src/testing/tests/` duplicate helper functions and battle setup patterns. Centralizing these helpers will reduce maintenance overhead and keep future test additions consistent.

### Key Findings
- `get_or_create_trigger_queue()` and `ensure_battle_load_request_exists()` are copy-pasted across a dozen test files.
- Battle setup boilerplate (`ensure...`, `GameStateManager::get().to_battle()`, `app.wait_for_frames(1)`) is repeated everywhere.
- Trigger events are manually pushed in slightly different ways instead of using a consistent helper.

## Current Implementation Status

### What Exists:
- ✅ `src/testing/test_server_helpers.h` exists with server-specific helpers (`server_integration_test_setup`, `read_json_file`)
- ✅ Some test files have local helper namespaces (e.g., `ValidateSetBonusSystemTestHelpers`, `ValidateDebugDishTestHelpers`)

### What's Missing:
- ❌ Shared `src/testing/test_battle_helpers.h` file does not exist
- ❌ Common helpers like `get_or_create_trigger_queue()` are duplicated across test files
- ❌ `setup_battle_for_test()` function does not exist
- ❌ Shared trigger event helpers do not exist

### Implementation Steps
1. **Create shared helpers (`src/testing/test_battle_helpers.h`):**
   - Provide singleton helpers for trigger queue and battle load request.
   - Add `setup_battle_for_test(TestApp &app)` to encapsulate common battle bootstrapping.
   - Offer trigger-event helper functions (`push_trigger_event`, `push_trigger_event_with_slot`).
   - Include optional query helpers (`find_entity_by_id`, `find_dish_by_id`) if needed by tests.
2. **Refactor duplicated tests (Paella, Wagyu, Pho, SummonDish, CopyEffect, RandomTargetScopes, NewEffectOperations, DrinkEffects, EffectSystem, DebugDish, TriggerSystemComponents, TriggerOrdering):**
   - Remove local helper definitions.
   - `#include "../test_battle_helpers.h"` (and `using namespace test_battle_helpers;` if convenient).
   - Replace manual trigger queue manipulations with shared helper calls.
   - Ensure files rely on the centralized battle setup helpers.
3. **Optional TestApp conveniences:**
   - Extend `TestApp` with thin wrappers around the shared helpers (e.g., `push_trigger_event`).
   - Only pursue if it meaningfully reduces code in multiple files.
4. **Validation:**
   - Run `./scripts/run_all_tests.sh` to ensure no regressions after refactoring.

### Expected Benefits
- One canonical definition for frequently used helpers.
- Less boilerplate for future ECS/combat tests.
- Easier to update trigger or battle setup logic without touching dozens of files.

## Outstanding Questions
1. **Ownership:** Who maintains the shared helper module once created (QA tooling squad vs gameplay engineers)?
2. **Language/Namespace Choice:** Should helpers live in `testing::battle` or remain in the global namespace for brevity?
3. **Enforcement:** Do we add a clang-tidy style check to block new ad-hoc helpers, or rely on code review discipline?
4. **Future-proofing:** How will these helpers adapt once combat systems switch fully to SOA (Plan 03) — do we need abstraction layers now?
5. **TestApp Surface Area:** Should we expose convenience methods on `TestApp`, or keep helpers separate to avoid bloating the class?
