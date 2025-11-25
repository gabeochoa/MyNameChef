## Test Refactoring Plan

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

