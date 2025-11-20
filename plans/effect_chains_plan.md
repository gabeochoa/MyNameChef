# Effect Chains and Dependencies Plan

## Overview

This plan details implementing effect chains where one effect can trigger another effect, creating complex effect interactions.

## Goal

Implement effect chain system:
- Effect A can trigger effect B
- Handle effect dependencies
- Support conditional effect chains

## Current State

### Existing Infrastructure

- ✅ `EffectResolutionSystem` processes effects
- ✅ `DishEffect` structure exists
- ✅ Effects can target other dishes
- ✅ Trigger system exists (`TriggerHook`, `TriggerQueue`)

### Missing Features

- ❌ Effect dependency tracking
- ❌ Effect chain resolution
- ❌ Conditional effect chains

## Design Approach

### Option 1: Implicit Chains (Recommended)

**Approach**: Effects naturally chain through trigger system.

**Example**:
- Effect A triggers `OnBiteTaken` → applies stat change
- Stat change triggers `OnStatChange` → applies effect B
- Effect B triggers `OnServe` → applies effect C

**Pros**: Uses existing trigger system, natural flow
**Cons**: Less explicit control

### Option 2: Explicit Chain Definitions

**Approach**: Define chains explicitly in effect definitions.

**Example**:
```cpp
struct DishEffect {
  // ... existing fields ...
  std::optional<DishEffect> chainedEffect; // Effect to trigger after this one
  bool chainCondition = false; // Condition for chaining
};
```

**Pros**: Explicit control, clear dependencies
**Cons**: More complex, requires new data structures

### Option 3: Hybrid Approach

**Approach**: Support both implicit (via triggers) and explicit (via chain definitions).

**Recommendation**: **Option 1 (Implicit Chains)** - Builds on existing system.

## Implementation Strategy

### 1. Effect Dependency Tracking

**Track Effect Sources**:
- When effect A triggers effect B, track that B depends on A
- Store dependency graph for debugging/validation

**Structure**:
```cpp
struct EffectDependency {
  int sourceEffectId; // Effect that triggered this
  int targetEffectId; // Effect that was triggered
  TriggerHook trigger; // Trigger that caused chain
};
```

### 2. Effect Chain Resolution

**Logic**:
1. Process effect A (normal resolution)
2. Effect A applies stat change or creates entity
3. Stat change/entity creation triggers new event
4. New event processes effect B (if conditions met)
5. Repeat for effect C, etc.

**Implementation**: Use existing trigger system - effects naturally chain through events.

### 3. Conditional Effect Chains

**Conditions**:
- Effect chains only if condition met
- Examples:
  - "If target has Heat status: apply Burn effect"
  - "If adjacent dish has Freshness >= 2: apply Chain effect"

**Implementation**: Extend `DishEffect.conditional` to support chain conditions.

### 4. Chain Reaction Detection

**Prevent Infinite Loops**:
- Track effects processed in current chain
- Limit chain depth (e.g., max 10 effects in chain)
- Log warnings for deep chains

**Implementation**:
```cpp
void resolve_effect_chain(Entity &source, DishEffect &effect, 
                          std::set<int> &processed_effects, int depth = 0) {
  if (depth > 10) {
    log_warn("EFFECT_CHAIN: Max depth reached, stopping chain");
    return;
  }
  
  if (processed_effects.count(effect.id)) {
    log_warn("EFFECT_CHAIN: Circular dependency detected");
    return;
  }
  
  processed_effects.insert(effect.id);
  
  // Process effect
  apply_effect(source, effect);
  
  // Check if effect triggers new events
  // (handled by existing trigger system)
}
```

## Test Cases

### Test 1: Simple Effect Chain

**Setup**:
- Effect A: OnServe → +1 Freshness to Self
- Effect B: OnFreshnessChange → +1 Spice to Adjacent

**Expected**:
- Effect A triggers
- Freshness change detected
- Effect B triggers on adjacent dish

### Test 2: Conditional Chain

**Setup**:
- Effect A: OnBiteTaken → apply Heat status
- Effect B: If target has Heat → apply Burn effect

**Expected**:
- Effect A applies Heat
- Effect B checks condition (Heat exists)
- Effect B applies Burn

### Test 3: Chain Depth Limit

**Setup**:
- Create chain of 15 effects (each triggers next)

**Expected**:
- Chain processes up to depth limit (10)
- Warning logged for exceeding limit
- Chain stops safely

## Validation Steps

1. **Design Effect Chain System**
   - Decide on approach (implicit vs explicit)
   - Define dependency tracking structure

2. **Implement Dependency Tracking**
   - Track effect sources
   - Build dependency graph
   - Log chains for debugging

3. **Implement Chain Resolution**
   - Use existing trigger system
   - Process effects in chain order
   - Handle conditional chains

4. **Add Chain Safety**
   - Prevent infinite loops
   - Limit chain depth
   - Log warnings

5. **MANDATORY CHECKPOINT**: Build
   - Run `xmake` - code must compile successfully

6. **MANDATORY CHECKPOINT**: Run Headless Tests
   - Run `./scripts/run_all_tests.sh` (ALL must pass)

7. **MANDATORY CHECKPOINT**: Run Non-Headless Tests
   - Run `./scripts/run_all_tests.sh -v` (ALL must pass)

8. **Verify Effect Chains**
   - Test simple chains work
   - Test conditional chains work
   - Test chain depth limits

9. **MANDATORY CHECKPOINT**: Commit
   - `git commit -m "implement effect chains and dependencies"`

## Edge Cases

1. **Circular Dependencies**: Effect A triggers B, B triggers A
2. **Very Long Chains**: 20+ effects in chain
3. **Conditional Failure**: Chain stops if condition not met
4. **Multiple Chains**: Multiple effects trigger simultaneously

## Success Criteria

- ✅ Effects can trigger other effects
- ✅ Dependency tracking works
- ✅ Conditional chains work correctly
- ✅ Chain depth limits prevent infinite loops
- ✅ All tests pass in headless and non-headless modes

## Estimated Time

6-8 hours:
- 2 hours: Design and dependency tracking
- 2 hours: Chain resolution implementation
- 1.5 hours: Conditional chains
- 1 hour: Safety and edge cases
- 1.5 hours: Testing and validation

