# Temp Entities Refactoring - Comprehensive Plan

## Executive Summary

This document consolidates analysis and implementation plans for refactoring the `temp_entities` pattern in the afterhours ECS library. The refactoring would **eliminate 2 library improvements entirely**, **simplify 5 others**, and **reduce overall complexity** across the codebase. This is a high-value refactoring that should be done **before** implementing many of the planned library improvements.

**Key Decision**: **SOA (Structure of Arrays) Architecture** - Selected for superior component fingerprint filtering performance.

**Key Decision Points**:
1. **Approach**: ✅ **SOA Architecture** (selected for performance benefits)
2. **Timing**: Before library improvements (enables better optimizations)
3. **Risk**: High (major architectural change) but high reward (significant performance gains)

---

## Table of Contents

1. [Root Cause Analysis](#root-cause-analysis)
2. [Alternative Approaches](#alternative-approaches)
3. [Impact on Library Improvements](#impact-on-library-improvements)
4. [Implementation Plans](#implementation-plans)
5. [Library Improvements Context](#library-improvements-context)

---

## Root Cause Analysis

### Current State

After analyzing the codebase and comparing with `kart-afterhours`:

1. **ECS Design**: Entities created via `EntityHelper::createEntity()` go into `temp_entities` array
2. **Automatic Merging**: `SystemManager::tick()` automatically calls `merge_entity_arrays()` after each system's `after(dt)` method (see `vendor/afterhours/src/core/system.h:371`)
3. **The Problem**: Code is manually calling `merge_entity_arrays()` in multiple places, which is a code smell (current count: 4 matches in 2 files: `test_server_helpers.h` and `battle_simulator.cpp`)
4. **Why kart-afterhours doesn't need it**: Systems automatically merge after each system runs, so entities created during one system are available to the next system automatically.

### Why temp_entities Exists

1. **Iterator Safety**: Prevents modifying `entities` vector while iterating over it
2. **Frame Isolation**: New entities created in System A don't appear to System B in the same frame
3. **Batch Merging**: Entities are merged after each system completes

### Current Problems

1. **Query Complexity**: `EntityQuery` must check `temp_entities` and warn if not merged
2. **Manual Merges**: Code must call `merge_entity_arrays()` in tests and special cases
3. **Inconsistent Merging**: `tick()` merges after each system, but `fixed_tick()` and `render()` don't
4. **Confusing API**: Developers must understand when entities are "visible" to queries

### Key Insights

1. **Use references directly**: When we create an entity with `createEntity()`, we get a reference - use it directly, don't query by ID
2. **Wait a frame when querying**: If we need to query for entities (don't have reference), wait a frame - system loop merges automatically
3. **force_merge only when necessary**: Only use `EntityQuery({.force_merge = true})` when we absolutely must query immediately in the same frame

---

## Alternative Approaches

### Approach 1: Queries Check Both Arrays (No Merging)

#### Concept
Instead of merging `temp_entities` into `entities`, queries iterate over both arrays when building results.

#### Implementation
```cpp
RefEntities run_query(const UnderlyingOptions) const {
  RefEntities out;
  const auto& main_entities = EntityHelper::get_entities();
  const auto& temp_entities = EntityHelper::get_temp();
  
  // Reserve space for both arrays
  out.reserve(main_entities.size() + temp_entities.size());
  
  // Iterate main entities
  for (const auto &e_ptr : main_entities) {
    if (!e_ptr) continue;
    Entity &e = *e_ptr;
    out.push_back(e);
  }
  
  // Iterate temp entities
  for (const auto &e_ptr : temp_entities) {
    if (!e_ptr) continue;
    if (e_ptr->cleanup) continue; // Skip cleanup-marked entities
    Entity &e = *e_ptr;
    out.push_back(e);
  }
  
  // Apply filters, sorting, etc. (same as before)
  // ...
}
```

#### Pros
1. **No merge overhead**: Eliminates `merge_entity_arrays()` calls
2. **Frame isolation preserved**: Temp entities stay separate until merge (or never merge)
3. **Simpler than merging**: Just iterate two arrays instead of copying
4. **No manual merges needed**: Queries automatically see both arrays
5. **Threading-friendly**: Can read both arrays without modification

#### Cons
1. **Query overhead**: Every query iterates two arrays instead of one
2. **Duplicate iteration**: If temp_entities is usually empty, still checking it
3. **Cache complexity**: Cache invalidation must account for both arrays
4. **Still has temp_entities**: Doesn't eliminate the pattern, just changes how it's used

#### Performance Impact
- **Query cost**: O(n + m) where n = main entities, m = temp entities
- **Current cost**: O(n) after merge, but merge is O(m)
- **Net effect**: Similar total cost, but distributed differently
- **Cache locality**: Slightly worse (iterating two separate arrays)

### Approach 2: Full Removal (Add Directly to Entities)

#### Concept
Entities are added directly to `entities_DO_NOT_USE` instead of `temp_entities`. No merging needed.

#### Pros
1. **Simpler API**: Entities immediately available for queries
2. **No Manual Merges**: Eliminate all `merge_entity_arrays()` calls
3. **Consistent Behavior**: All systems see entities immediately
4. **Fewer Bugs**: No "query will miss X ents in temp" warnings
5. **Better Performance**: No merge overhead, no temp array management

#### Cons
1. **Frame Isolation Loss**: New entities created in System A become visible to System B immediately
2. **Potential Iterator Issues**: Adding to vector during iteration (mitigated by shared_ptr copies)

### Approach 3: SOA (Structure of Arrays) Architecture

#### Concept
Instead of `std::vector<std::shared_ptr<Entity>>` where each Entity contains components, use separate arrays for each component type. Entities become just IDs.

#### Current Architecture (AoS - Array of Structures)
```cpp
struct Entity {
  EntityID id;
  ComponentArray componentArray;  // Array of component pointers
  // ...
};

using Entities = std::vector<std::shared_ptr<Entity>>;
```

#### SOA Architecture
```cpp
// Entities are just IDs
using EntityID = int;

// Component fingerprint storage (for fast filtering)
struct ComponentFingerprintStorage {
  std::vector<ComponentBitSet> fingerprints;  // Dense array of component bitsets
  std::vector<EntityID> entity_ids;          // Which entity owns each fingerprint
  std::unordered_map<EntityID, size_t> entity_to_index;  // Lookup
};

// Separate arrays for each component type
template<typename Component>
struct ComponentStorage {
  std::vector<Component> components;          // Dense array of components
  std::vector<EntityID> entity_ids;         // Which entity owns each component
  std::unordered_map<EntityID, size_t> entity_to_index;  // Lookup
};

// Global storages
ComponentFingerprintStorage fingerprints;  // For fast filtering
ComponentStorage<Transform> transforms;
ComponentStorage<Health> healths;
ComponentStorage<IsDish> dishes;
// ... etc
```

**Key Innovation: Component Fingerprint Storage**
- Store `ComponentBitSet` in dense array for each entity
- Filter queries by iterating over dense fingerprint array first
- Use SIMD for bitset comparisons (check multiple fingerprints at once)
- Only access component arrays for entities that pass fingerprint filter

#### Pros
1. **Better cache locality**: Components of same type stored contiguously
2. **Faster iteration**: Iterate over dense arrays of components
3. **Better for SIMD**: Can vectorize component operations
4. **Memory efficient**: No pointer indirection, better memory layout
5. **Temp entities become trivial**: Just delay adding entity ID to component storages

#### Cons
1. **Major architectural change**: Complete rewrite of entity system
2. **API breaking**: All component access changes
3. **Complexity**: Need to manage multiple arrays, synchronization
4. **Query complexity**: Must check multiple component storages
5. **Migration effort**: Massive refactoring required

#### Temp Entities in SOA
With SOA, "temp entities" could mean:
- **Option A**: Entity ID exists but components not yet added to storages
- **Option B**: Components in separate "temp" component storages
- **Option C**: Just delay adding entity ID to component storages until merge

**Simplest approach**: Entity ID created, components added immediately to component storages. No temp needed - just add directly.

### Comparison Matrix

| Aspect | Current (Merge) | Check Both Arrays | Full Removal | SOA |
|--------|----------------|------------------|--------------|-----|
| **Merge overhead** | High (every system) | None | None | N/A |
| **Query complexity** | Low (1 array) | Medium (2 arrays) | Low (1 array) | High (multiple storages) |
| **Frame isolation** | Yes | Yes | No | N/A (immediate) |
| **Code simplicity** | Medium | Medium | High | Low (complex) |
| **Performance** | Medium | Medium | High | Very High |
| **Migration effort** | N/A | Low | Low | Very High |
| **Threading support** | Medium | High | Medium | High |
| **Cache locality** | Poor | Poor | Poor | Excellent |

### Recommendation: SOA Architecture (SELECTED)

**Why SOA is the Right Choice**:
1. **Component Fingerprint Filtering**: With SOA, component fingerprints (`ComponentBitSet`) can be stored in dense arrays, enabling:
   - **SIMD operations**: Vectorized bitset comparisons
   - **Cache-friendly iteration**: Dense arrays of fingerprints = excellent cache locality
   - **Early rejection**: Filter by fingerprints before accessing components
   - **Batch processing**: Process multiple fingerprints in parallel

2. **Query Performance**: 
   - Current: Iterate entities → check bitset → access component (pointer indirection)
   - SOA: Iterate dense fingerprint array → filter → access dense component array (no indirection)

3. **Temp Entities Become Trivial**: With SOA, entities are just IDs - components added immediately to component storages. No temp array needed.

**Migration Strategy**:
- **Phase 1**: Implement SOA alongside current system (dual mode)
- **Phase 2**: Migrate systems one by one to use SOA
- **Phase 3**: Remove AoS implementation

**Performance Gains Expected**:
- **Component fingerprint queries**: 5-10x faster (SIMD + cache locality)
- **Component access**: 2-3x faster (no pointer indirection)
- **Memory usage**: 20-30% reduction (no pointer overhead)

---

## Impact on Library Improvements

### Improvements Directly Eliminated

#### #4. System Loop Optimization
**Current Plan**: Optimize `merge_entity_arrays()` to only merge when `temp_entities` is non-empty.

**After Removal**: 
- **Completely eliminated** - no merge function exists
- **Code removed**: `vendor/afterhours/src/core/system.h:371` (merge call in `tick()`)
- **Code removed**: `vendor/afterhours/src/core/system.h:388` (would need merge in `fixed_tick()` if we added it)
- **Code removed**: `vendor/afterhours/src/core/system.h:407` (would need merge in `render()` if we added it)
- **Code removed**: `vendor/afterhours/src/core/entity_helper.h:88-100` (entire `merge_entity_arrays()` function)
- **Code removed**: `vendor/afterhours/src/core/entity_helper.h:154, 171` (merge calls in `cleanup()` and `delete_all_entities()`)

**Impact**: Saves implementation time, eliminates ~15 lines of code, removes merge overhead entirely.

#### #13. Remove Unused Query Options
**Current Plan**: Simplify `QueryOptions` struct by removing `force_merge` and `ignore_temp_warning` options.

**After Removal**:
- **Completely eliminated** - these options become meaningless
- **Code removed**: `vendor/afterhours/src/core/entity_query.h:321-324` (QueryOptions struct)
- **Code removed**: `vendor/afterhours/src/core/entity_query.h:326-350` (entire temp_entities checking logic in constructor)
- **Code removed**: All `EntityQuery({.force_merge = true})` calls throughout codebase
- **Code removed**: All `EntityQuery({.ignore_temp_warning = true})` calls throughout codebase

**Impact**: Eliminates ~25 lines of complex query logic, removes 2 API options, simplifies query construction.

### Improvements Significantly Simplified

#### #1. EntityQuery Caching and Memory Optimization
**Current Complexity**: 
- Query constructor must check `temp_entities.size()` (line 328)
- Must handle `force_merge` option (lines 332-334)
- Must generate warnings about temp entities (lines 335-349)
- Cache invalidation must account for temp entities

**After Removal**:
- Query constructor becomes trivial: `EntityQuery() : entities(EntityHelper::get_entities()) {}`
- No temp checking logic needed
- Cache invalidation only needs to track main `entities` array changes
- Simpler memory optimization - only one array to manage

**Impact**: Reduces query construction overhead, simplifies caching logic, makes memory optimization easier.

#### #5. Query Result Caching Invalidation
**Current Issue**: `ran_query` flag never gets reset when entities change, and must account for temp entities.

**After Removal**:
- Only need to track changes to main `entities` array
- Can use simple entity count or modification timestamp
- No need to check if temp entities were merged
- Cache invalidation becomes straightforward: `if (entity_count_changed) ran_query = false`

**Impact**: Simpler invalidation logic, more reliable caching, easier to implement.

#### #14. Simplify Entity Creation Options
**Current Plan**: Replace `createEntityWithOptions()` with separate functions.

**After Removal**:
- Entity creation becomes: `entities.push_back(std::make_shared<Entity>())`
- No temp array management needed
- No merge timing concerns
- Can simplify to: `createEntity()` and `createPermanentEntity()` only

**Impact**: Simpler API, less code, no timing concerns.

### Improvements with Reduced Complexity

- **#7. Remove Dead Code**: Removing temp_entities reduces overall codebase complexity
- **#9. Consolidate Tag Query Methods**: Simpler query system makes consolidation easier
- **#16. Remove Redundant Query Methods**: Simpler query construction makes deduplication clearer

### Code Reduction Summary

**Estimated Code Reduction**:
- **Core library**: ~50-60 lines removed
- **Query complexity**: ~25 lines of complex logic removed
- **System loop**: ~5 lines removed
- **Codebase-wide**: ~10-20 manual merge calls removed
- **Total**: ~90-110 lines of code removed, plus significant complexity reduction

---

## Pre-Implementation Questions and Information Needed

Before starting SOA implementation, we need to answer the following questions and gather information:

### Key Decisions Made ✅

**API Compatibility**: **Full backward compatibility maintained**
- OptEntity, RefEntity, EntityQuery APIs remain unchanged
- Only internal library implementation changes
- Library users don't need to know about SOA

**Entity Implementation**: **Lightweight wrapper accessing SOA storage**
- Keep Entity struct with same API (`get<T>()`, `has<T>()`, etc.)
- Entity internally accesses SOA component storages
- Very little changes for library users

**Migration Strategy**: **All-at-once with API transparency**
- Migrate entire library to SOA internally
- Users see no API changes
- Entity wrapper handles SOA access transparently

**Singleton Components**: **Normal entities, not special case**
- Singletons are normal entities stored in SOA
- Product side assumes there's only one entity with that component
- Helper `get_singleton<Component>()` queries for entity with component (assumes only one)
- Easy to go from component -> sophie entity

**Component Operations**: **API unchanged, fastest internal implementation**
- `entity.addComponent<T>()` and `entity.removeComponent<T>()` work the same
- `entity.get<T>()` returns reference as before
- Use fastest implementation internally (lookup with caching if beneficial)
- No API changes, optimize internally

### 1. API Compatibility and Migration Strategy

**Q1.1: Do we need to maintain backward compatibility with existing code?**
- **Option A**: Maintain full API compatibility (RefEntity, OptEntity work the same)
- **Option B**: Break API but provide migration helpers
- **Option C**: Dual-mode API (SOA and AoS both supported)
- **✅ DECISION**: **Option A** - Maintain full API compatibility. OptEntity and RefEntity should not change. EntityQuery should not change API. Only the internals to the library should change - library users shouldn't need to know about SOA.

**Q1.2: How should we handle RefEntity and OptEntity in SOA?**
- **Current**: `RefEntity = std::reference_wrapper<Entity>` (references Entity object)
- **SOA**: Entities are just IDs, no Entity object to reference
- **Options**:
  - Create lightweight Entity wrapper that accesses SOA storage
  - Change RefEntity to store EntityID + component accessors
  - Keep Entity struct but it's just metadata (ID, tags, cleanup flag)
- **✅ DECISION**: **Keep real Entity struct for now**. Lightweight wrapper is fine as long as `entity->get<Component>()` still works. There should be very little actual changes for library users. Entity struct will access SOA storage internally but maintain the same API.

**Q1.3: Should we support gradual migration or all-at-once?**
- **Option A**: Dual-mode system (SOA + AoS run simultaneously)
- **Option B**: All-at-once migration (higher risk, faster)
- **✅ DECISION**: **Option B (all-at-once)** but with API compatibility maintained. Since the API doesn't change, from the user's perspective it's seamless, but internally we migrate everything to SOA at once. The Entity wrapper handles the SOA access transparently.

### 2. Component Storage and Access

**Q2.1: How should singleton components work in SOA?**
- **Current**: Stored in `singletonMap` pointing to Entity*
- **SOA Options**:
  - Keep singletonMap but point to EntityID
  - Store singletons in separate storage (not SOA)
  - Use special EntityID (e.g., -1) for singletons
- **✅ DECISION**: **Singletons are normal entities**. On the game/product side, assume there's only one. Used as a "Global King Entity" that holds information that would otherwise be static singletons. Need to easily go from component -> sophie entity. Store as normal EntityID in SOA, use helper `get_singleton<Component>()` that queries for entity with that component (assuming only one exists).

**Q2.2: How are tags stored in SOA?**
- **Current**: `TagBitset tags` in Entity struct (64 bits, stored per entity)
- **SOA Options**:

  **Option A: Separate TagStorage (parallel to fingerprints)**
  ```cpp
  struct TagStorage {
    std::vector<TagBitset> tags;           // Dense array of tag bitsets
    std::vector<EntityID> entity_ids;      // Parallel array
    std::unordered_map<EntityID, size_t> entity_to_index;
  };
  ```
  - **Pros**: 
    - ✅ Cache-friendly (dense array iteration for tag queries)
    - ✅ Can use SIMD for tag filtering (same as fingerprints)
    - ✅ Tags stored alongside fingerprints (same access pattern)
    - ✅ Fast tag-only queries (iterate tag array directly)
    - ✅ Can filter by tags + fingerprints in parallel
  - **Cons**: 
    - ❌ Extra storage structure to maintain
    - ❌ Need to keep in sync with fingerprints (two arrays)
    - ❌ Slightly more complex (two lookups for entity metadata)

  **Option B: Keep tags in Entity metadata struct**
  ```cpp
  struct EntityMetadata {
    EntityID id;
    TagBitset tags;
    bool cleanup;
  };
  std::vector<EntityMetadata> entity_metadata;
  ```
  - **Pros**: 
    - ✅ Simpler (tags stay with entity metadata)
    - ✅ No extra storage structure
    - ✅ Tags always available when you have Entity reference
    - ✅ Natural: tags are entity properties, not component properties
  - **Cons**: 
    - ❌ Entity metadata array still needed (can't eliminate entirely)
    - ❌ Less cache-friendly (tags scattered with metadata)
    - ❌ Can't easily filter by tags without iterating all entities
    - ❌ Harder to use SIMD for tag filtering (metadata mixed with tags)

  **Option C: Store tags alongside fingerprints**
  ```cpp
  struct FingerprintStorage {
    std::vector<ComponentBitSet> fingerprints;
    std::vector<TagBitset> tags;           // Parallel array
    std::vector<EntityID> entity_ids;
  };
  ```
  - **Pros**: 
    - ✅ Single storage structure (simpler management)
    - ✅ Tags and fingerprints together (same access pattern)
    - ✅ Can filter by both in one pass (same iteration)
    - ✅ Cache-friendly (both arrays dense and parallel)
  - **Cons**: 
    - ❌ Fingerprint storage becomes more complex (mixing concerns)
    - ❌ Tags are conceptually different from components (entity property vs component)
    - ❌ If you only need tags, still iterate fingerprint array

  **Option D: Hybrid - Tags in metadata, but cache in TagStorage for queries**
  ```cpp
  // Entity metadata (always up-to-date)
  struct EntityMetadata { EntityID id; TagBitset tags; bool cleanup; };
  
  // Cached for fast queries (regenerated on tag changes)
  struct TagStorage { std::vector<TagBitset> tags; ... };
  ```
  - **Pros**: 
    - ✅ Tags always available in Entity (natural API)
    - ✅ Fast tag queries via cached storage
    - ✅ Best of both worlds
  - **Cons**: 
    - ❌ Most complex (two storage locations)
    - ❌ Need to keep in sync (cache invalidation)
    - ❌ More memory usage

- **Recommendation**: **Option A (Separate TagStorage)** - Best performance for tag queries, clean separation, enables SIMD optimization. Tags are used frequently in queries (whereHasTag, whereHasAllTags), so fast filtering is important.
- **Status**: ⚠️ **NEEDS MORE ANALYSIS** - User wants to think about this more

### Additional Considerations for Q2.2 (Tag Storage)

**Factors to Consider**:

1. **Tag Query Frequency vs Component Query Frequency**
   - How often are tag-only queries used? (e.g., `whereHasTag<T>()` without component filters)
   - How often are tag+component queries used? (e.g., `whereHasComponent<T>().whereHasTag<U>()`)
   - If tags are mostly used WITH components, Option C (tags with fingerprints) might be better
   - If tags are frequently queried alone, Option A (separate storage) is better

2. **Tag Mutation Frequency**
   - How often are tags added/removed? (`enableTag()`, `disableTag()`)
   - If tags change frequently, keeping them in Entity metadata (Option B) might be simpler
   - If tags are mostly set at creation and rarely change, separate storage (Option A) is fine

3. **Memory Layout and Cache Behavior**
   - Option A: Tags in separate array = good for tag-only queries, but need two lookups for entity+tags
   - Option B: Tags with metadata = always available with entity, but scattered memory
   - Option C: Tags with fingerprints = single lookup, but mixes concerns

4. **Query Performance Requirements**
   - Are tag queries performance-critical? (e.g., filtering hundreds of entities per frame)
   - If yes, Option A or C (dense arrays) are better
   - If no, Option B (metadata) might be sufficient

5. **API Simplicity**
   - Option B keeps tags naturally with Entity (matches current API)
   - Option A/C require Entity wrapper to access tags from storage
   - But since API doesn't change, this is internal only

6. **Future Extensibility**
   - Will we add more entity metadata? (e.g., entity groups, layers, etc.)
   - If yes, Option B (metadata struct) might scale better
   - If no, Option A (separate storage) is cleaner

**Questions to Answer**:
- What percentage of queries use tags vs components?
- How many entities typically have tags vs components?
- Are tag queries in hot paths (every frame) or cold paths (occasional)?
- Do we need SIMD optimization for tag filtering, or is simple iteration fast enough?

**Next Steps**: 
- Profile current tag usage patterns
- Measure tag query performance
- Consider future use cases
- Decide based on actual usage patterns

**Q2.3: How do we handle component add/remove operations?**
- **Current**: `entity.addComponent<T>()` modifies Entity's componentArray
- **SOA**: Need to add/remove from ComponentStorage<T>
- **Questions**:
  - Do we maintain Entity wrapper that provides same API?
  - How do we handle component removal (sparse arrays vs compaction)?
  - What about component move semantics?
- **✅ DECISION**: **API should not change**. `entity.addComponent<T>()` and `entity.removeComponent<T>()` work the same. Entity wrapper internally adds/removes from ComponentStorage<T> and updates fingerprint. Implementation details (sparse arrays, compaction) are internal optimizations.

**Q2.4: How do we handle component access patterns?**
- **Current**: `entity.get<T>()` returns reference to component
- **SOA**: Need to lookup in ComponentStorage<T>
- **Questions**:
  - Cache component pointers in Entity wrapper?
  - Always lookup (safer but slower)?
  - Provide both APIs (fast path + safe path)?
- **✅ DECISION**: **API should not change**. `entity.get<T>()` returns reference as before. Use fastest implementation internally - likely lookup in ComponentStorage<T> with caching if beneficial. No API changes, optimize internally.

### 3. Query System

**Q3.1: Should we maintain the same query API?**
- **Current**: `EntityQuery().whereHasComponent<T>().gen()`
- **SOA**: Can optimize internally but keep same API
- **Questions**:
  - Keep exact same API?
  - Add SOA-specific query methods?
  - Auto-detect SOA vs AoS and optimize accordingly?
- **✅ DECISION**: **Keep exact same API**. EntityQuery API should not change - only internals change. Users call the same methods, but internally we use SOA fingerprint filtering for performance.

**Q3.2: How do we handle component fingerprint filtering?**
- **Questions**:
  - Use SIMD for bitset comparisons? (Which SIMD instruction set?)
  - Fallback for non-SIMD platforms?
  - Batch size for SIMD operations?
  - Cache fingerprint masks for common queries?

**Option A: SIMD-Optimized Fingerprint Filtering**
  ```cpp
  // Use SSE4.2/AVX2 for bitset comparisons
  // Process 4-8 fingerprints at once
  for (size_t i = 0; i < fingerprints.size(); i += 4) {
    __m128i fp1 = _mm_load_si128(&fingerprints[i]);
    __m128i fp2 = _mm_load_si128(&fingerprints[i+1]);
    __m128i fp3 = _mm_load_si128(&fingerprints[i+2]);
    __m128i fp4 = _mm_load_si128(&fingerprints[i+3]);
    __m128i mask = _mm_set1_epi64x(query_mask.to_ullong());
    
    // AND operation, compare to mask
    __m128i result1 = _mm_and_si128(fp1, mask);
    __m128i result2 = _mm_and_si128(fp2, mask);
    // ... check if result == mask
  }
  ```
  - **Pros**:
    - ✅ **4-8x faster** for fingerprint filtering (process multiple at once)
    - ✅ **Cache-friendly** (sequential memory access)
    - ✅ **Scalable** (AVX2 can process 8 fingerprints, AVX-512 can do 16)
    - ✅ **Best for large entity counts** (hundreds/thousands)
  - **Cons**:
    - ❌ **Platform-specific** (need different code for x86_64 vs ARM)
    - ❌ **Complexity** (SIMD intrinsics are harder to read/debug)
    - ❌ **Fallback needed** (non-SIMD platforms or when SIMD unavailable)
    - ❌ **Bitset size matters** (ComponentBitSet is 128 bits = 2x uint64_t, need to handle carefully)

**Option B: Scalar Fingerprint Filtering with Optimizations**
  ```cpp
  // Simple scalar loop with optimizations
  for (size_t i = 0; i < fingerprints.size(); ++i) {
    if ((fingerprints[i] & query_mask) == query_mask) {
      // Match found
    }
  }
  ```
  - **Pros**:
    - ✅ **Simple** (easy to read, debug, maintain)
    - ✅ **Portable** (works everywhere)
    - ✅ **Compiler optimizations** (compiler can auto-vectorize)
    - ✅ **Sufficient for small-medium entity counts** (< 1000 entities)
  - **Cons**:
    - ❌ **Slower** (process one at a time)
    - ❌ **Less cache-efficient** (though still good with dense arrays)
    - ❌ **May not scale** (performance degrades with many entities)

**Option C: Hybrid - SIMD with Scalar Fallback**
  ```cpp
  #ifdef __SSE4_2__
    // Use SIMD for bulk processing
    process_fingerprints_simd(fingerprints, query_mask);
  #else
    // Fallback to scalar
    process_fingerprints_scalar(fingerprints, query_mask);
  #endif
  ```
  - **Pros**:
    - ✅ **Best performance** when SIMD available
    - ✅ **Portable** (fallback for non-SIMD platforms)
    - ✅ **Future-proof** (can add AVX2, AVX-512, NEON support)
  - **Cons**:
    - ❌ **Most complex** (need both implementations)
    - ❌ **Code duplication** (SIMD + scalar versions)
    - ❌ **Testing complexity** (test both paths)

**Recommendation**: **Option C (Hybrid)** - Use SIMD when available (SSE4.2/AVX2 for x86_64, NEON for ARM), fallback to scalar. Cache fingerprint masks for common queries. Start with SSE4.2 (widely supported), add AVX2 later if needed.

**Additional Optimizations**:
- **Cache fingerprint masks**: Pre-compute common query masks (e.g., `Transform + Health`)
- **Early exit**: Stop iteration once we have enough results (for `take(N)` queries)
- **Batch processing**: Process fingerprints in chunks to improve cache locality

- **Status**: ⚠️ **NEEDS DECISION** - See options above with pros/cons

**Q3.3: How do we handle queries with multiple component types?**
- **Current**: Iterate entities, check each component
- **SOA Options**:
  - **Option A**: Filter by fingerprint first, then check components
    - Filter fingerprints to get candidate EntityIDs
    - Then access component storages for those EntityIDs
    - **Pros**: Fast fingerprint filtering, minimal component access
    - **Cons**: Two-pass (fingerprint then components)
  - **Option B**: Use intersection of component storage entity sets
    - Get EntityID sets from each ComponentStorage<T>
    - Intersect sets to find entities with all components
    - **Pros**: Single-pass for components, set operations are fast
    - **Cons**: Need to maintain EntityID sets per component storage
  - **Option C**: Hybrid approach
    - Filter by fingerprint first (fast)
    - For remaining candidates, check component storages
    - **Pros**: Best of both worlds
    - **Cons**: More complex

- **✅ DECISION**: **Option C (Hybrid) - Whatever is fastest**. Since queries are currently slow, prioritize performance. Filter by fingerprint first (SIMD-optimized), then access component storages for matching entities. Benchmark to verify this is fastest approach.

### 4. System Integration

**Q4.1: How do systems access entities in SOA?**
- **Current**: `for_each(Entity &entity, ...)` receives Entity reference
- **SOA Options**:
  - Entity wrapper that looks up components on-demand
  - Pass EntityID + component references directly
  - Keep Entity wrapper but it accesses SOA storage
- **✅ DECISION**: **Keep Entity wrapper that accesses SOA storage**. Systems still receive `Entity &entity` and call `entity.get<Component>()` - the Entity struct internally looks up components from SOA storage. No API changes for systems.

**Q4.2: How do we handle system iteration?**
- **Current**: Iterate `entities` vector, call `for_each` on each Entity
- **SOA Options**:
  - Iterate fingerprint storage, create Entity wrappers
  - Iterate component storages directly (for component-specific systems)
  - Hybrid: fingerprint filter first, then iterate matching entities
- **Status**: ⚠️ **NEEDS DESIGN**

**Q4.3: How do we handle `has_child_of()` checks?**
- **Current**: Iterate componentArray, use dynamic_cast
- **SOA**: Need to check component storages for derived types
- **Questions**:
  - Maintain component type hierarchy?
  - Store parent-child relationships?
  - Use RTTI or custom type system?
- **Status**: ⚠️ **NEEDS DESIGN**

### 5. Performance and Testing

**Q5.1: What are the current performance baselines?**
- **Need to measure**:
  - Query performance (entities with N components)
  - Component access patterns (frequency, cache misses)
  - System iteration overhead
  - Memory usage patterns
- **✅ SOLUTION**: **Performance benchmark example created**
  - **File**: `vendor/afterhours/example/performance_benchmark/main.cpp`
  - **Features**:
    - Creates configurable number of entities (default 10,000)
    - Tests entity creation performance
    - Tests component access patterns
    - Tests various query types (single component, multiple components, with tags)
    - Tests system iteration performance
    - Tests memory usage at different entity counts
    - Tests query scaling (100 to 50,000 entities)
  - **Usage**: 
    ```bash
    cd vendor/afterhours/example/performance_benchmark
    make                    # Run all benchmarks
    ./performance_benchmark.exe --queries    # Run only query benchmarks
    ./performance_benchmark.exe --scaling   # Run scaling benchmarks
    ```
  - **Output**: Detailed timing statistics (min, max, mean, median) for each benchmark
- **Status**: ✅ **READY** - Benchmark example created, ready to establish baselines

**Q5.2: How do we test SOA without breaking existing code?**
- **Options**:
  - Unit tests for SOA storage
  - Integration tests with dual-mode
  - Performance benchmarks
  - Regression tests for existing functionality
- **Status**: ⚠️ **NEEDS PLAN**

**Q5.3: What SIMD instruction sets should we target?**
- **Options**:
  - SSE4.2 (x86_64, widely supported)
  - AVX2 (x86_64, better performance)
  - NEON (ARM)
  - Auto-detect and use best available
- **Status**: ⚠️ **NEEDS DECISION**

### 6. Memory and Lifecycle

**Q6.1: How do we handle entity cleanup in SOA?**
- **Current**: Mark `entity.cleanup = true`, remove in cleanup()
- **SOA**: Need to remove from all component storages
- **Questions**:
  - Mark EntityID as deleted, compact later?
  - Remove immediately from all storages?
  - Lazy cleanup (mark and remove on next access)?
- **Status**: ⚠️ **NEEDS DESIGN**

**Q6.2: How do we handle permanent entities?**
- **Current**: `permanant_ids` set tracks permanent entities
- **SOA**: Need to track which EntityIDs are permanent
- **Status**: ⚠️ **NEEDS DESIGN**

**Q6.3: How do we handle component storage growth?**
- **Questions**:
  - Pre-allocate storage?
  - Dynamic growth with reallocation?
  - Use memory pools?
  - Reserve based on entity count?
- **Status**: ⚠️ **NEEDS DESIGN**

### 7. Migration and Rollback

**Q7.1: What's the rollback strategy if SOA causes issues?**
- **Options**:
  - Keep AoS code until SOA is proven stable
  - Feature flag to switch between SOA and AoS
  - Gradual migration allows partial rollback
- **✅ DECISION**: Since API compatibility is maintained, we can keep AoS code as fallback behind a feature flag or compile-time option. Entity wrapper can switch between SOA and AoS storage based on configuration. This allows rollback without breaking user code.

**Q7.2: Which systems should we migrate first?**
- **Priority candidates**:
  - Query-heavy systems (combat, rendering)
  - Hot path systems (Transform updates)
  - Simple systems (easier to verify)
- **Status**: ⚠️ **NEEDS ANALYSIS**

**Q7.3: How do we verify correctness during migration?**
- **Options**:
  - Run both SOA and AoS in parallel, compare results
  - Extensive test coverage
  - Gradual migration with validation at each step
- **Status**: ⚠️ **NEEDS PLAN**

### Information Needed Before Implementation

1. **Component Access Patterns**: Profile current codebase to understand:
   - Most common component access patterns
   - Query frequency and complexity
   - Component add/remove frequency
   - System iteration patterns

2. **Performance Baselines**: Measure current performance:
   - Query time for various component combinations
   - Component access time
   - Memory usage
   - Cache miss rates

3. **Codebase Analysis**: Understand:
   - How many systems use `for_each` vs direct queries
   - How many components are typically on an entity
   - How often components are added/removed
   - Singleton component usage patterns

4. **SIMD Research**: Determine:
   - Best SIMD approach for bitset operations
   - Platform support (SSE4.2, AVX2, NEON)
   - Fallback strategies

5. **Testing Strategy**: Plan:
   - Unit tests for SOA storage
   - Integration tests
   - Performance benchmarks
   - Regression tests

---

## Implementation Plans

### Implementation Plan: SOA Architecture (SELECTED)

**Key Principle**: **Zero API Changes** - All changes are internal to the library. Users continue using the same API (`Entity`, `RefEntity`, `OptEntity`, `EntityQuery`) but get SOA performance benefits.

#### Phase 1: Design Component Fingerprint Storage
- **File**: Create `vendor/afterhours/src/core/component_fingerprint_storage.h`
- **Design**: Dense array storage for `ComponentBitSet` with entity ID mapping
- **Key Features**:
  - `std::vector<ComponentBitSet> fingerprints` - dense array
  - `std::vector<EntityID> entity_ids` - parallel array for entity IDs
  - `std::unordered_map<EntityID, size_t> entity_to_index` - fast lookup
- **SIMD Support**: Design for 128-bit/256-bit SIMD operations on bitsets
- **API**: Internal only - users don't see this

#### Phase 2: Implement Component Storages
- **File**: Create `vendor/afterhours/src/core/component_storage.h`
- **Template**: `ComponentStorage<T>` for each component type
- **Features**:
  - Dense `std::vector<T> components`
  - Parallel `std::vector<EntityID> entity_ids`
  - Lookup map for O(1) access
- **Migration**: Start with hot components (Transform, Health, IsDish, etc.)
- **API**: Internal only - users don't see this

#### Phase 3: Modify Entity Struct to Access SOA Storage
- **Goal**: Entity struct becomes lightweight wrapper that accesses SOA storage
- **Implementation**:
  - Keep Entity struct with same API (`get<T>()`, `has<T>()`, etc.)
  - Entity stores EntityID, tags, cleanup flag (metadata only)
  - `entity.get<T>()` internally looks up component from `ComponentStorage<T>`
  - `entity.has<T>()` checks fingerprint storage
  - `entity.addComponent<T>()` adds to SOA storage and updates fingerprint
- **Files**: Modify `vendor/afterhours/src/core/entity.h`
- **API**: **No changes** - same API, different implementation

#### Phase 4: Update EntityHelper to Use SOA
- **Goal**: Entity creation and management uses SOA internally
- **Implementation**:
  - `createEntity()` creates EntityID and adds to fingerprint storage
  - `get_entities()` returns Entity wrappers that access SOA storage
  - Remove `temp_entities` - components added directly to SOA storage
  - Remove `merge_entity_arrays()` - no longer needed
- **Files**: Modify `vendor/afterhours/src/core/entity_helper.h`
- **API**: **No changes** - same methods, SOA implementation

#### Phase 5: Optimize Query System for SOA
- **File**: `vendor/afterhours/src/core/entity_query.h`
- **Changes**:
  - Internally use SOA fingerprint filtering
  - Filter by fingerprints first (SIMD-optimized)
  - Early rejection of non-matching entities
  - Return Entity wrappers (same RefEntity API)
- **Performance Target**: 5-10x faster for component fingerprint queries
- **API**: **No changes** - same query API, SOA-optimized internals

#### Phase 6: Update System Iteration
- **Goal**: Systems iterate over Entity wrappers that access SOA
- **Implementation**:
  - System loop iterates fingerprint storage
  - Creates Entity wrappers for matching entities
  - Calls `for_each(Entity &entity, ...)` as before
  - Entity wrapper accesses SOA storage when components accessed
- **Files**: Modify `vendor/afterhours/src/core/system.h`
- **API**: **No changes** - systems use same API

#### Phase 7: Testing and Validation
- **Goal**: Ensure SOA works correctly with zero API changes
- **Tests**:
  - All existing tests should pass without modification
  - Performance benchmarks to verify improvements
  - Regression tests for component access patterns
- **Validation**: Users shouldn't need to change any code

#### Phase 8: Cleanup (Optional)
- **After**: SOA proven stable
- **Remove**: Old AoS code paths (if kept for fallback)
- **Optimize**: Further SOA-specific optimizations

### Implementation Plan: Check Both Arrays (Alternative)

#### Step 1: Modify `run_query()` to Check Both Arrays
- **File**: `vendor/afterhours/src/core/entity_query.h`
- **Change**: Iterate both `entities` and `temp_entities` in `run_query()`
- **Remove**: Temp checking logic from constructor (lines 328-349)

#### Step 2: Remove Merge Logic
- **File**: `vendor/afterhours/src/core/system.h`
- **Remove**: Merge calls from system loop
- **File**: `vendor/afterhours/src/core/entity_helper.h`
- **Keep**: `temp_entities` array
- **Remove**: `merge_entity_arrays()` function (or make it no-op)

#### Step 3: Remove Manual Merges
- **Files**: All codebase files
- **Remove**: All `EntityHelper::merge_entity_arrays()` calls
- **Remove**: All `EntityQuery({.force_merge = true})` calls

#### Step 4: Optional: Lazy Merge
- **Option**: Keep merge function but call it lazily (e.g., during cleanup)
- **Benefit**: Eventually consolidate arrays for better cache locality
- **Cost**: Still need merge logic, but called less frequently

### Implementation Plan: Full Removal

#### Step 1: Validate Iterator Safety
- **File**: `vendor/afterhours/src/core/system.h`
- **Action**: Confirm range-based for loop with `shared_ptr` copies is safe during vector growth
- **Validation**: Write test that creates entities during system iteration, verify no crashes

#### Step 2: Modify `createEntity()` to Add Directly
- **File**: `vendor/afterhours/src/core/entity_helper.h`
- **Change**: `createEntityWithOptions()` adds to `entities_DO_NOT_USE` instead of `temp_entities`
- **Keep**: `temp_entities` array (for now) but don't use it

#### Step 3: Remove Merge Logic from System Loop
- **File**: `vendor/afterhours/src/core/system.h`
- **Change**: Remove `EntityHelper::merge_entity_arrays()` calls from:
  - `tick()` (line 371)
  - `fixed_tick()` (if we add it for consistency)
  - `render()` (if we add it for consistency)
- **Note**: `cleanup()` still calls merge - update to remove that call

#### Step 4: Simplify `EntityQuery`
- **File**: `vendor/afterhours/src/core/entity_query.h`
- **Change**: Remove `temp_entities` checking logic (lines 328-349)
- **Remove**: `force_merge` option (no longer needed)
- **Remove**: `ignore_temp_warning` option (no longer needed)

#### Step 5: Remove All Manual Merge Calls
- **Files**: 
  - `src/testing/test_server_helpers.h` (line 53)
  - `src/server/battle_simulator.cpp` (lines 100, 396, 460)
  - Any other files found via grep
- **Action**: Delete all `EntityHelper::merge_entity_arrays()` calls

#### Step 6: Update Tests
- **Action**: Remove any test code that relies on merge timing
- **Action**: Update tests that use `force_merge` to use normal queries
- **Validation**: All tests should pass with direct addition

#### Step 7: Remove `temp_entities` Infrastructure (Optional)
- **File**: `vendor/afterhours/src/core/entity_helper.h`
- **Action**: Remove `temp_entities` member, `get_temp()`, `merge_entity_arrays()`, `reserve_temp_space()`
- **Note**: Keep for one release cycle, then remove if no issues

### Validation Tests (Before Removing Merges)

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

### Patterns to Fix

1. **Unnecessary ID queries**: Create entity → store ID → merge → query by ID → use entity
   - **Better**: Create entity → use reference directly (no merge/query needed)

2. **Immediate queries after creation**: Create entity → immediately query for it
   - **Better**: Use reference directly, OR wait a frame (`app.pump_once(dt)` or `app.wait_for_frames(1)`)

3. **Cross-system queries**: System A creates entity → System B queries for it
   - **Better**: System loop merges automatically between systems, so next system can query normally

4. **Test patterns**: Create entity → merge → query
   - **Better**: Use reference directly, OR wait a frame, OR use `force_merge` only if truly needed

### Validation Steps

1. Build: `make`
2. Run headless tests: `./scripts/run_all_tests.sh`
3. Run non-headless tests: `./scripts/run_all_tests.sh -v`
4. Verify all tests pass, especially validation tests and `validate_effect_system`
5. Verify no warnings about temp entities in query logs
6. Commit: `git commit -m "refactor - remove manual merge_entity_arrays calls, use references and system loop"`

### Risk Assessment

#### Low Risk
- Iterator safety (shared_ptr copies protect against invalidation)
- Most systems already handle entities gracefully
- Query simplification reduces bugs

#### Medium Risk
- Some systems might assume entities created in same frame aren't visible yet
- Need to audit systems that create entities and immediately query

#### Mitigation Strategy
1. Run full test suite after removal
2. Keep temp_entities infrastructure commented out for one release cycle as fallback
3. Add logging to track entity creation timing if issues arise
4. Document the change clearly for developers

---

## Library Improvements Context

### Performance Improvements (with Validation)

#### 1. EntityQuery Caching and Memory Optimization
**File**: `src/core/entity_query.h`
- **Issue**: Query system allocates/deallocates entire entity lists on every query
- **Improvement**: 
  - Use index-based filtering with final copy instead of building intermediate vectors
  - Cache query results more aggressively - only invalidate when entities change
  - Pre-allocate `RefEntities` with `reserve()` based on entity count
- **Impact**: High - queries are hot path
- **Note**: Simplified after temp_entities removal

#### 2. Remove Unnecessary Sorting
**File**: `src/core/entity_query.h:411-419`
- **Issue**: Sorts even when result set has 1 item
- **Improvement**: Check size before sorting, skip sort entirely for single-item results
- **Impact**: Medium - saves O(n log n) for common case

#### 3. Partial Sort for `gen_first()`
**File**: `src/core/entity_query.h:257-266`
- **Issue**: Full sort when only first element needed
- **Improvement**: Use `std::nth_element` or `std::partial_sort` for `gen_first()` and `take()`
- **Impact**: Medium - O(n) instead of O(n log n) for first-only queries

#### 4. System Loop Optimization
**File**: `src/core/system.h:357-372`
- **Issue**: Calls `merge_entity_arrays()` after every system, even when no new entities
- **Status**: **ELIMINATED** if temp_entities removed - no merge function exists

#### 5. Query Result Caching Invalidation
**File**: `src/core/entity_query.h:244-248`
- **Issue**: `ran_query` flag never gets reset when entities change
- **Improvement**: Track entity count/modification timestamp, invalidate cache on change
- **Note**: Simplified after temp_entities removal

#### 6. Entity Component Array Optimization
**File**: `src/core/entity.h:27-28`
- **Issue**: `ComponentArray` uses pointer indirection overhead
- **Improvement**: Consider using `std::variant` or type-erased storage
- **Impact**: High - component access is very hot path
- **API Breaking**: Yes - major architectural change

### Simplicity Improvements (with Validation)

#### 7. Remove Dead Code: `include_store_entities`
**File**: `src/core/entity_query.h:355-368`
- **Issue**: Flag exists but never used in filtering
- **Improvement**: Remove the flag entirely, replace with tag-based filtering
- **API Breaking**: Yes

#### 8. Simplify Query Modification Pattern
**File**: `src/core/entity_query.h:26-29`
- **Issue**: Heap-allocated `Modification` objects via `std::unique_ptr` adds complexity
- **Improvement**: Use `std::function` directly or value semantics

#### 9. Consolidate Tag Query Methods
**File**: `src/core/entity_query.h:105-157`
- **Issue**: Many overloads for tag queries create API surface
- **Improvement**: Use single template that handles all cases via SFINAE

#### 10. Remove `OptEntity` Wrapper Complexity
**File**: `src/core/entity.h:263-288`
- **Issue**: `OptEntity` wraps `std::optional<std::reference_wrapper<Entity>>` adding indirection
- **Improvement**: Use `std::optional<Entity*>` or `std::optional<RefEntity>` directly
- **API Breaking**: Yes

#### 11. Simplify System Template Metaprogramming
**File**: `src/core/system.h:63-214`
- **Issue**: Complex template metaprogramming with platform-specific workarounds
- **Improvement**: Use C++17 `if constexpr` consistently

#### 12. Fix Typo: `permanant_ids`
**File**: `src/core/entity_helper.h:32`
- **Issue**: Spelling error "permanant" should be "permanent"
- **Improvement**: Rename to `permanent_ids`
- **API Breaking**: Yes - but internal field name

### Complexity Reduction (with Validation)

#### 13. Remove Unused Query Options
**File**: `src/core/entity_query.h:321-324`
- **Issue**: `QueryOptions` has `force_merge` and `ignore_temp_warning` but logic is complex
- **Status**: **ELIMINATED** if temp_entities removed - these options become meaningless

#### 14. Simplify Entity Creation Options
**File**: `src/core/entity_helper.h:35-37, 74-86`
- **Issue**: `CreationOptions` struct with single bool field
- **Improvement**: Use overloaded functions `createEntity()` vs `createPermanentEntity()` instead
- **Note**: Simplified after temp_entities removal

#### 15. Consolidate Component Access Methods
**File**: `src/core/entity.h:162-187`
- **Issue**: `get()`, `get_with_child()`, `has()`, `has_child_of()` - many similar methods
- **Improvement**: Consider single `get()` with optional template parameter
- **API Breaking**: Yes

#### 16. Remove Redundant Query Methods
**File**: `src/core/entity_query.h:230-237`
- **Issue**: `has_values()` and `is_empty()` both call `run_query()` with same options
- **Improvement**: Make `is_empty()` call `!has_values()` to avoid duplication

#### 17. Simplify Library Template
**File**: `src/library.h:17-122`
- **Issue**: `Library<T>` has many iterator methods that just forward to `std::map`
- **Improvement**: Inherit from or use composition with `std::map` more directly

#### 18. Remove Unused `entity_type` Field
**File**: `src/core/entity.h:35`
- **Issue**: `int entity_type = 0;` field exists but may not be used
- **Improvement**: Remove if unused, or document its purpose
- **API Breaking**: Yes

### API Improvements (with Validation)

#### 19. Add Query Builder Fluent Interface
**File**: `src/core/entity_query.h`
- **Issue**: Current API requires chaining, but could be more ergonomic
- **Improvement**: Add helper functions for fluent interface

#### 20. Fix Component ID Overflow Check
**File**: `src/core/base_component.h:24-34`
- **Issue**: Overflow check is commented out
- **Improvement**: Fix the check or use `std::atomic` properly
- **Impact**: High - memory safety

#### 21. Make `gen_random()` Use Proper RNG
**File**: `src/core/entity_query.h:299-306`
- **Issue**: Uses `rand()` which is not thread-safe and low quality
- **Improvement**: Accept RNG as parameter or use `std::random_device`
- **API Breaking**: Yes

#### 22. Add Query Result Iterators
**File**: `src/core/entity_query.h`
- **Issue**: Must call `gen()` to get vector, then iterate
- **Improvement**: Make `EntityQuery` itself iterable, or return iterator range

#### 23. Const-Correctness for Systems
**File**: `src/core/system.h:159-161, 307-313`
- **Issue**: Const systems can't be used in update loop
- **Improvement**: Support const systems in update loop, or make const/non-const versions

#### 24. Remove `values_ignore_cache()` Public API
**File**: `src/core/entity_query.h:238-242`
- **Issue**: Public method that's only used internally
- **Improvement**: Make private or remove, use `run_query()` directly
- **API Breaking**: Yes

### Code Quality (with Validation)

#### 25. Fix Logging Implementation
**File**: `src/logging.h:10-47`
- **Issue**: Uses `va_list` which is C-style, not type-safe
- **Improvement**: Use variadic templates with `fmt` library (already in vendor)
- **API Breaking**: Yes

#### 26. Use `std::expected` Consistently
**File**: `src/library.h:41-48, 90-106`
- **Issue**: Some methods return `expected`, others throw or return null
- **Improvement**: Standardize on `std::expected` for all fallible operations
- **API Breaking**: Yes

#### 27. Remove Platform-Specific Workarounds
**File**: `src/core/system.h:96-118, 134-166`
- **Issue**: `#ifdef _WIN32` branches for template specialization
- **Improvement**: Use C++17 `if constexpr` which works on all platforms

#### 28. Fix Reserve Calculation
**File**: `src/core/entity_helper.h:45-47`
- **Issue**: `reserve(sizeof(EntityType) * 100)` reserves bytes, not elements
- **Improvement**: `reserve(100)` to reserve 100 entities
- **Impact**: Low - bug fix

### Plugin/Core Separation Strategy

#### 29. Define Public API Boundary
**Goal**: Clearly separate core ECS (public API) from plugins (examples using public API)

**Current State**:
- Core headers: `core/entity.h`, `core/entity_query.h`, `core/system.h`, `core/base_component.h`, `core/entity_helper.h`
- Plugins: `plugins/input_system.h`, `plugins/ui/`, `plugins/animation.h`, etc.
- Problem: Plugins may use private APIs from `EntityHelper` internals

**Improvements**:
1. Audit Plugin Dependencies
2. Create Public API Header
3. Move Plugins to Examples
4. Plugin Registration System
5. Document Plugin Interface
6. Refactor Existing Plugins
7. CI Enforcement

---

## Implementation Order Recommendation

### Phase 1: SOA Architecture Implementation (SELECTED)
**Why First**: 
- **Eliminates temp_entities entirely** (entities are just IDs, components added immediately)
- **Enables massive performance gains** (5-10x faster component fingerprint filtering)
- **Foundation for future optimizations** (SIMD, parallelization, cache optimizations)
- **Simplifies many improvements** (#1, #4, #5, #13, #14 become trivial or unnecessary)

**Implementation Steps**:
1. Design and implement component fingerprint storage
2. Implement component storages for hot components
3. Create dual-mode system (SOA + AoS)
4. Optimize query system for SOA with SIMD support
5. Migrate systems gradually (query-heavy first)
6. Remove AoS implementation once fully migrated

### Phase 2: SOA-Optimized Improvements
After SOA implementation, implement:
- #1: EntityQuery caching (now much simpler with SOA)
- #5: Query cache invalidation (simplified with fingerprint storage)
- #2: Remove unnecessary sorting (SOA enables better sorting strategies)
- #3: Partial sort for gen_first() (SOA makes this more efficient)
- SIMD optimizations for fingerprint filtering

### Phase 3: Remaining Improvements
Continue with other improvements that benefit from SOA architecture:
- Component array optimizations (now trivial with SOA)
- Memory optimizations (SOA reduces overhead)
- Plugin separation (easier with cleaner architecture)

---

## Conclusion

**Selected Approach: SOA Architecture**

SOA migration is a **high-value architectural change** that:
- **Eliminates temp_entities entirely** (entities are just IDs, components added immediately)
- **Dramatically improves query performance** (5-10x faster component fingerprint filtering)
- **Enables SIMD optimizations** (vectorized bitset operations)
- **Better cache locality** (dense arrays of components)
- **Reduces memory overhead** (no pointer indirection)
- **Simplifies future optimizations** (easier to parallelize, vectorize)

**Migration Path**:
1. **Phase 1**: Implement SOA alongside current system (dual mode) - allows gradual migration
2. **Phase 2**: Migrate systems one by one to use SOA - start with hot paths (queries, combat systems)
3. **Phase 3**: Remove AoS implementation - once all systems migrated

**Temp Entities in SOA**:
- With SOA, temp_entities pattern becomes unnecessary
- Entities are just IDs - components added immediately to component storages
- No merge needed - components are immediately available
- Frame isolation can be achieved via component flags if needed

**Recommendation**: 
1. **Implement SOA architecture** - major rewrite but huge performance gains
2. **Start with component fingerprint storage** - enables fast filtering
3. **Migrate systems gradually** - low risk, can roll back if needed
4. **Remove temp_entities as part of SOA migration** - simplifies the new architecture

This refactoring should be implemented **before** implementing the library improvements plan, as SOA will change how many optimizations work.

