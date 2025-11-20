# RerollCost, Freezeable, and Seeded RNG Cleanup Plan

## Overview

This plan details implementing deterministic reroll cost behavior, ensuring Freezeable component is used correctly, and cleaning up remaining `std::random_device` usage in shop code.

## Goal

- Implement deterministic `RerollCost` behavior
- Verify `Freezeable` component usage is correct
- Replace remaining `std::random_device` with `SeededRng` in shop code

## Current State

### Existing Components

- ✅ `RerollCost` component exists in `src/shop.h`
  - Fields: `base`, `increment`, `current`
  - Methods: `get_cost()`, `apply_reroll()`, `reset()`
  - Current: `increment = 0`, so cost stays at base (1)

- ✅ `Freezeable` component exists in `src/shop.h`
  - Field: `isFrozen`
  - Used in reroll logic to preserve frozen items

### Existing Systems

- ✅ Reroll button exists in `src/ui/ui_systems.cpp`
  - Shows cost: "Reroll (1)"
  - Uses `RerollCost` singleton
  - Preserves frozen items during reroll

- ✅ Freeze system works
  - `HandleFreezeIconClick` system
  - `RenderFreezeIcon` system
  - Tests exist: `ValidateShopFreezeTest.h`

### Potential Issues

- ❌ May have `std::random_device` usage in shop generation
- ❌ Reroll cost increment may not be deterministic
- ❌ Need to verify all shop RNG uses `SeededRng`

## Implementation Details

### 1. Verify RerollCost Determinism

**Current Implementation** (from `src/shop.h`):
```cpp
void apply_reroll() {
  current += increment;
}
```

**Analysis**:
- If `increment = 0`, cost stays at `base` (deterministic) ✅
- If `increment > 0`, cost increases deterministically ✅
- No RNG involved in cost calculation ✅

**Action**: Verify cost calculation is deterministic (should already be).

### 2. Verify Freezeable Usage

**Current Usage** (from `src/ui/ui_systems.cpp`):
- Reroll preserves frozen items ✅
- Freeze icon click toggles freeze state ✅
- Tests verify freeze functionality ✅

**Action**: Verify all freeze logic uses `Freezeable` component correctly.

### 3. Replace random_device in Shop Code

**Search for**: `std::random_device` in shop-related files

**Files to Check**:
- `src/systems/GenerateShopSlotsSystem.h` (or similar)
- `src/systems/InitialShopFillSystem.h` (or similar)
- Any shop generation systems

**Replace with**: `SeededRng::get()`

**Example**:
```cpp
// OLD
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(0, count - 1);

// NEW
SeededRng &rng = SeededRng::get();
std::uniform_int_distribution<> dis(0, count - 1);
int index = dis(rng.gen);
```

### 4. Ensure Shop Seed is Set

**Requirement**: Shop seed must be set before shop generation

**Location**: Check where shop seed is initialized

**Action**: Verify `SeededRng::get().set_seed(shop_seed)` is called before shop generation

## Test Cases

### Test 1: Deterministic Reroll Cost

**Setup**:
- Set `RerollCost{base=1, increment=1}`
- Perform multiple rerolls

**Expected**:
- Cost increases deterministically: 1, 2, 3, 4...
- No randomness in cost calculation

### Test 2: Freezeable Persistence

**Setup**:
- Freeze shop items
- Perform reroll

**Expected**:
- Frozen items remain
- Unfrozen items regenerate
- Freeze state preserved

### Test 3: Deterministic Shop Generation

**Setup**:
- Set same seed
- Generate shop twice

**Expected**:
- Same seed produces identical shop items
- Order and types match exactly

## Validation Steps

1. **Verify RerollCost Determinism**
   - Review `RerollCost` implementation
   - Test cost calculation
   - Verify no RNG in cost logic

2. **Verify Freezeable Usage**
   - Review freeze systems
   - Test freeze functionality
   - Verify component usage is correct

3. **Search for random_device in Shop Code**
   - Find all `std::random_device` usage
   - Replace with `SeededRng::get()`
   - Verify shop seed is set

4. **Test Deterministic Shop Generation**
   - Set same seed
   - Generate shop multiple times
   - Verify identical results

5. **MANDATORY CHECKPOINT**: Build
   - Run `xmake` - code must compile successfully

6. **MANDATORY CHECKPOINT**: Run Headless Tests
   - Run `./scripts/run_all_tests.sh` (ALL must pass)

7. **MANDATORY CHECKPOINT**: Run Non-Headless Tests
   - Run `./scripts/run_all_tests.sh -v` (ALL must pass)

8. **Verify Determinism**
   - Test reroll cost is deterministic
   - Test shop generation is deterministic
   - Test freeze functionality works

9. **MANDATORY CHECKPOINT**: Commit
   - `git commit -m "be - ensure deterministic reroll cost and shop RNG"`

## Edge Cases

1. **Reroll Cost Overflow**: Handle very high costs gracefully
2. **Freeze All Items**: All items frozen, reroll does nothing
3. **Seed Not Set**: Handle case where seed not initialized
4. **Concurrent Rerolls**: Ensure thread safety if needed

## Success Criteria

- ✅ RerollCost calculation is deterministic
- ✅ Freezeable component used correctly
- ✅ All shop RNG uses SeededRng
- ✅ Shop generation is deterministic with same seed
- ✅ All tests pass in headless and non-headless modes

## Estimated Time

2-3 hours:
- 30 min: Verify RerollCost determinism
- 30 min: Verify Freezeable usage
- 1 hour: Replace random_device in shop code
- 30 min: Testing and validation

