# Server RNG Determinism Plan

## Overview

This plan details replacing `std::random_device` with `SeededRng` in server code to ensure deterministic battle simulation for server-authoritative gameplay.

## Goal

Replace all `std::random_device` usage in server code with `SeededRng::get()` calls to ensure:
- Same seed produces identical battle outcomes
- Server and client can reproduce identical battles
- Deterministic testing and debugging

## Current State

### Locations Using `std::random_device`

1. **`src/server/battle_api.cpp:195`** - Battle seed generation
   - Currently: `std::random_device rd; uint64_t seed = static_cast<uint64_t>(rd()) << 32 | rd();`
   - Comment says: "Explicitly not using SeededRng here because this is the real seed generation for the battle"
   - **Issue**: This generates a non-deterministic seed, which breaks server determinism

2. **`src/server/async/systems/ProcessCommandQueueSystem.h:213`** - Command processing
   - Currently: `std::random_device rd; uint64_t battle_seed = static_cast<uint64_t>(rd()) << 32 | rd();`
   - Then sets: `SeededRng::get().set_seed(battle_seed);`
   - **Issue**: Generates non-deterministic seed before setting SeededRng

### Current SeededRng Usage

- `src/server/battle_simulator.cpp:35` - Already uses `SeededRng::get().set_seed(seed);`
- `src/server/team_manager.cpp` - Already uses `SeededRng::get()` for team generation
- `src/server/server_context.cpp:42` - Initializes SeededRng singleton

## Implementation Strategy

### Option 1: Use SeededRng for Seed Generation (Recommended)

**Approach**: Use SeededRng itself to generate the initial seed, but seed it with a deterministic source (e.g., timestamp or request ID).

**Pros**:
- Maintains determinism
- Can use request metadata (timestamp, request ID) as seed source
- Consistent with existing SeededRng usage

**Cons**:
- Need a deterministic seed source (not truly random)

### Option 2: Accept Non-Deterministic Seed Generation (Current Behavior)

**Approach**: Keep `std::random_device` for initial seed generation, but ensure all subsequent RNG uses SeededRng.

**Pros**:
- Truly random initial seeds
- Each battle is unique

**Cons**:
- Breaks determinism requirement
- Cannot reproduce battles for debugging
- Server and client may diverge

### Option 3: Hybrid Approach (Recommended for Production)

**Approach**: 
- Use `std::random_device` to generate initial seed (for uniqueness)
- Store the seed in battle results
- Use SeededRng with that seed for all battle RNG
- Client receives seed and can replay deterministically

**Implementation**:
1. Generate seed with `std::random_device` (one-time, non-deterministic)
2. Store seed in battle results/response
3. Set `SeededRng::get().set_seed(seed)` immediately after generation
4. All subsequent RNG uses `SeededRng::get()`

## Recommended Solution

**Use Option 3 (Hybrid)** - Generate seed once with `std::random_device`, then use SeededRng for all battle logic.

### Changes Required

#### 1. `src/server/battle_api.cpp:195`

**Current**:
```cpp
// Explicitly not using SeededRng here because
// this is the real seed generation for the battle
std::random_device rd;
uint64_t seed = static_cast<uint64_t>(rd()) << 32 | rd();
```

**New**:
```cpp
// Generate unique seed for this battle (non-deterministic, one-time)
std::random_device rd;
uint64_t seed = static_cast<uint64_t>(rd()) << 32 | rd();

// Set seed for deterministic battle simulation
SeededRng::get().set_seed(seed);

// Store seed in battle results for client replay
// (seed will be included in JSON response)
```

#### 2. `src/server/async/systems/ProcessCommandQueueSystem.h:213`

**Current**:
```cpp
// Generate battle seed
std::random_device rd;
uint64_t battle_seed = static_cast<uint64_t>(rd()) << 32 | rd();

// Set seed for deterministic battle simulation
SeededRng::get().set_seed(battle_seed);
```

**New**:
```cpp
// Generate unique seed for this battle (non-deterministic, one-time)
std::random_device rd;
uint64_t battle_seed = static_cast<uint64_t>(rd()) << 32 | rd();

// Set seed for deterministic battle simulation
SeededRng::get().set_seed(battle_seed);

// Note: Seed should be included in battle response for client replay
```

### Verification

After changes, verify:
1. Seed is generated once per battle
2. `SeededRng::get().set_seed()` is called immediately after generation
3. All battle RNG uses `SeededRng::get()` (not `std::random_device`)
4. Same seed produces identical battle outcomes

## Test Cases

### Test 1: Deterministic Battle with Same Seed

**Setup**:
- Run same battle twice with manually set seed (e.g., `SeededRng::get().set_seed(12345)`)
- Compare battle outcomes

**Expected**:
- Order of effects is identical
- Order of dishes entering combat is identical
- All RNG-dependent behavior is identical
- Battle results match exactly

### Test 2: Seed Persistence

**Setup**:
- Generate battle seed
- Store seed in battle results
- Load battle from stored seed
- Replay battle

**Expected**:
- Replayed battle matches original exactly

### Test 3: Multiple Battles with Different Seeds

**Setup**:
- Run multiple battles with different seeds
- Verify each battle is deterministic with its seed
- Verify battles differ from each other

**Expected**:
- Each battle is internally deterministic
- Different seeds produce different outcomes

## Validation Steps

1. **Replace random_device in battle_api.cpp**
   - Generate seed with `std::random_device`
   - Immediately call `SeededRng::get().set_seed(seed)`
   - Ensure seed is stored in battle response

2. **Replace random_device in ProcessCommandQueueSystem.h**
   - Generate seed with `std::random_device`
   - Immediately call `SeededRng::get().set_seed(battle_seed)`
   - Ensure seed is included in battle info

3. **Verify all battle RNG uses SeededRng**
   - Search for remaining `std::random_device` usage in server code
   - Ensure all battle simulation uses `SeededRng::get()`

4. **Create determinism test**
   - Test that same seed produces identical results
   - Test that different seeds produce different results

5. **MANDATORY CHECKPOINT**: Build
   - Run `make` - code must compile successfully

6. **MANDATORY CHECKPOINT**: Run Headless Tests
   - Run `./scripts/run_all_tests.sh` (ALL must pass)

7. **MANDATORY CHECKPOINT**: Run Non-Headless Tests
   - Run `./scripts/run_all_tests.sh -v` (ALL must pass)

8. **Verify server determinism**
   - Run same battle twice with same seed
   - Compare outputs (logs, results, event order)
   - Verify identical behavior

9. **MANDATORY CHECKPOINT**: Commit
   - `git commit -m "be - replace random_device with SeededRng in server code"`

10. **Only after successful commit, proceed to Task 3**

## Edge Cases

1. **Concurrent Battles**: Each battle should have its own seed. SeededRng is singleton, so battles must be sequential or use separate RNG instances.

2. **Seed Collision**: Very unlikely with 64-bit seeds, but if it occurs, battles should still be deterministic.

3. **Server Restart**: Seeds are not persisted across restarts (by design - each battle gets fresh seed).

## Success Criteria

- ✅ All `std::random_device` usage in server battle code replaced with SeededRng
- ✅ Seed generation happens once per battle
- ✅ Seed is stored in battle results for client replay
- ✅ Same seed produces identical battle outcomes
- ✅ All tests pass in headless and non-headless modes
- ✅ Server determinism verified with test cases

## Estimated Time

1-2 hours:
- 30 min: Code changes
- 30 min: Testing and verification
- 30 min: Test creation and validation

