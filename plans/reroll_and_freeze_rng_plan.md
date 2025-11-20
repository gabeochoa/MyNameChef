# RerollCost, Freezeable, and Seeded RNG Cleanup Plan

## Overview

This plan documents the current state of reroll cost behavior, Freezeable component usage, and SeededRng usage in shop code. **Most functionality is already implemented.**

## Goal

- ✅ Implement deterministic `RerollCost` behavior
- ✅ Verify `Freezeable` component usage is correct
- ✅ Replace remaining `std::random_device` with `SeededRng` in shop code

## Current State

### ✅ Fully Implemented

1. **RerollCost Component** (`src/shop.h`)
   - Fields: `base`, `increment`, `current`
   - Methods: `get_cost()`, `apply_reroll()`, `reset()`
   - Current: `increment = 0`, so cost stays at base (1)
   - **Deterministic**: No RNG involved in cost calculation
   - Applied after successful reroll in `src/ui/ui_systems.cpp:595`

2. **Freezeable Component** (`src/shop.h`)
   - Field: `isFrozen`
   - Used correctly in reroll logic to preserve frozen items
   - Systems: `HandleFreezeIconClick`, `RenderFreezeIcon`
   - Tests: `ValidateShopFreezeTest.h`, `ValidateShopFreezeUnfreezeTest.h`, `ValidateShopFreezePurchaseTest.h`, `ValidateShopFreezeSwapTest.h`, `ValidateShopFreezeBattlePersistenceTest.h`

3. **Shop RNG Uses SeededRng** ✅
   - `get_random_dish()` in `src/shop.cpp:85` uses `SeededRng::get()`
   - `get_random_dish_for_tier()` in `src/shop.cpp:91` uses `SeededRng::get()`
   - `get_random_drink()` in `src/drink_types.cpp:39` uses `SeededRng::get()`
   - `get_random_drink_for_tier()` in `src/drink_types.cpp:45` uses `SeededRng::get()`
   - All shop generation systems use these functions (no direct `std::random_device` usage)

4. **SeededRng Initialization** (`src/main.cpp:134-139`)
   - Initialized on game start with `randomize_seed()`
   - Uses `std::random_device` for initial seed (intentional, for non-deterministic first run)
   - Can be set deterministically with `set_seed()` for testing/replay

5. **Reroll Button** (`src/ui/ui_systems.cpp:487-595`)
   - Shows cost: "Reroll (1 gold)"
   - Uses `RerollCost` singleton
   - Preserves frozen items during reroll
   - Applies cost increment after successful reroll

6. **Test Coverage** ✅
   - `ValidateRerollCostTest.h` - Tests reroll cost functionality
   - `ValidateSeededRngTest.h` - Tests deterministic shop generation
   - Multiple freeze tests verify freeze functionality

### ❌ Not Applicable (Server Code)

The only remaining `std::random_device` usage is in **server code**, which is a separate concern:
- `src/server/battle_api.cpp:195` - Battle seed generation
- `src/server/async/systems/ProcessCommandQueueSystem.h:213` - Command processing

**Note**: These are covered by `plans/server_rng_determinism_plan.md` and are not part of this plan.

### Intentional random_device Usage

`SeededRng::randomize_seed()` uses `std::random_device` intentionally:
- Purpose: Generate non-deterministic initial seed for first game run
- Location: `src/seeded_rng.h:36-40`
- This is correct behavior - initial seed should be random, but subsequent RNG should be deterministic

## Implementation Details

### 1. RerollCost Determinism ✅

**Current Implementation** (from `src/shop.h:43-48`):
```cpp
void apply_reroll() {
  current += increment;
}
```

**Analysis**:
- ✅ If `increment = 0`, cost stays at `base` (deterministic)
- ✅ If `increment > 0`, cost increases deterministically
- ✅ No RNG involved in cost calculation
- ✅ Applied after successful reroll in `ui_systems.cpp:595`

**Status**: ✅ **Complete** - Cost calculation is deterministic

### 2. Freezeable Usage ✅

**Current Usage**:
- ✅ Reroll preserves frozen items (`ui_systems.cpp:514-525`)
- ✅ Freeze icon click toggles freeze state (`HandleFreezeIconClick.h`)
- ✅ Freeze icon renders correctly (`RenderFreezeIcon.h`)
- ✅ Tests verify freeze functionality

**Status**: ✅ **Complete** - Freezeable component used correctly

### 3. Shop RNG Uses SeededRng ✅

**All shop generation functions use SeededRng**:

1. **Dish Generation** (`src/shop.cpp`):
   ```cpp
   DishType get_random_dish() {
     auto &rng = SeededRng::get();
     const auto &pool = get_default_dish_pool();
     return pool[rng.gen_index(pool.size())];
   }
   
   DishType get_random_dish_for_tier(int tier) {
     auto &rng = SeededRng::get();
     const auto &pool = get_dishes_for_tier(tier);
     return pool[rng.gen_index(pool.size())];
   }
   ```

2. **Drink Generation** (`src/drink_types.cpp`):
   ```cpp
   DrinkType get_random_drink() {
     auto all_drinks = magic_enum::enum_values<DrinkType>();
     auto &rng = SeededRng::get();
     return all_drinks[rng.gen_index(all_drinks.size())];
   }
   
   DrinkType get_random_drink_for_tier(int tier) {
     // ... tier logic ...
     auto &rng = SeededRng::get();
     return available_drinks[rng.gen_index(available_drinks.size())];
   }
   ```

3. **Shop Systems Use These Functions**:
   - `InitialShopFill.h` uses `get_random_dish_for_tier()`
   - `GenerateDrinkShop.h` uses `get_random_drink_for_tier()`
   - Reroll in `ui_systems.cpp` uses `get_random_dish_for_tier()` and `get_random_drink()`

**Status**: ✅ **Complete** - All shop RNG uses SeededRng

### 4. Shop Seed Initialization ✅

**Current Implementation** (`src/main.cpp:134-139`):
```cpp
// Initialize SeededRng singleton for deterministic randomness
{
  auto &rng = SeededRng::get();
  rng.randomize_seed();
  log_info("Initialized SeededRng with seed {}", rng.seed);
}
```

**Analysis**:
- ✅ SeededRng initialized before shop generation
- ✅ Uses `randomize_seed()` for non-deterministic first run (intentional)
- ✅ Can be set deterministically with `set_seed()` for testing/replay
- ✅ Test `ValidateSeededRngTest.h` verifies deterministic behavior

**Status**: ✅ **Complete** - Shop seed is initialized correctly

## Test Cases (All Implemented)

### Test 1: Deterministic Reroll Cost ✅

**Test**: `ValidateRerollCostTest.h`

**Setup**:
- Navigate to shop
- Get initial reroll cost (should be 1)
- Perform reroll

**Expected**:
- ✅ Cost stays at 1 (increment is 0)
- ✅ No randomness in cost calculation
- ✅ Cost is applied correctly

**Status**: ✅ **Passing**

### Test 2: Freezeable Persistence ✅

**Tests**: Multiple freeze tests

**Setup**:
- Freeze shop items
- Perform reroll

**Expected**:
- ✅ Frozen items remain
- ✅ Unfrozen items regenerate
- ✅ Freeze state preserved

**Status**: ✅ **Passing**

### Test 3: Deterministic Shop Generation ✅

**Test**: `ValidateSeededRngTest.h`

**Setup**:
- Set same seed
- Generate shop twice

**Expected**:
- ✅ Same seed produces identical shop items
- ✅ Order and types match exactly
- ✅ Different seed produces different items

**Status**: ✅ **Passing**

## Validation Steps

### ✅ Completed

1. ✅ **Verify RerollCost Determinism**
   - Reviewed `RerollCost` implementation
   - Tested cost calculation
   - Verified no RNG in cost logic

2. ✅ **Verify Freezeable Usage**
   - Reviewed freeze systems
   - Tested freeze functionality
   - Verified component usage is correct

3. ✅ **Search for random_device in Shop Code**
   - Found no `std::random_device` usage in shop code
   - All shop RNG uses `SeededRng::get()`
   - Only intentional usage is in `SeededRng::randomize_seed()`

4. ✅ **Test Deterministic Shop Generation**
   - Test exists: `ValidateSeededRngTest.h`
   - Verifies same seed produces same results
   - Verifies different seed produces different results

5. ✅ **Build**: Code compiles successfully

6. ✅ **Tests**: All tests pass in headless and non-headless modes

## Edge Cases (Handled)

1. ✅ **Reroll Cost Overflow**: Cost calculation is simple addition, no overflow issues
2. ✅ **Freeze All Items**: All items frozen, reroll preserves all (tested)
3. ✅ **Seed Not Set**: SeededRng initialized in `main.cpp` before shop generation
4. ✅ **Concurrent Rerolls**: Single-threaded game loop, no concurrency issues

## Success Criteria

- ✅ RerollCost calculation is deterministic
- ✅ Freezeable component used correctly
- ✅ All shop RNG uses SeededRng
- ✅ Shop generation is deterministic with same seed
- ✅ All tests pass in headless and non-headless modes

## Summary

**Status**: ✅ **COMPLETE**

All goals of this plan have been achieved:
1. ✅ RerollCost is deterministic (no RNG in cost calculation)
2. ✅ Freezeable component is used correctly throughout
3. ✅ All shop RNG uses SeededRng (no `std::random_device` in shop code)
4. ✅ Shop seed is initialized before generation
5. ✅ Comprehensive test coverage exists

**Remaining Work**: None - all functionality is implemented and tested.

**Note**: The only `std::random_device` usage remaining is in server code (`battle_api.cpp`, `ProcessCommandQueueSystem.h`), which is covered by a separate plan (`server_rng_determinism_plan.md`).

## Estimated Time

**Actual Time**: Already completed (implementation and tests exist)

**If Starting Fresh**: 2-3 hours:
- 30 min: Verify RerollCost determinism
- 30 min: Verify Freezeable usage
- 1 hour: Replace random_device in shop code (already done)
- 30 min: Testing and validation (already done)
