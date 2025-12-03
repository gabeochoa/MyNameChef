# Afterhours Library Improvements, Validation, and Plugin Separation

## At-a-Glance
- **Sequence:** 04 / 21 — scoped to modernize the shared afterhours ECS so downstream game + tooling work inherits cleaner primitives.
- **Mission:** Deliver measurable wins in performance, simplicity, and API clarity while defining a clean plugin/public boundary.
- **Status:** Awaiting SOA foundation from Plan 03; once available we execute the prioritized backlog below.
- **Success Metrics:** 
  - ≥30% reduction in hot-path allocations + query time (benchmarked pre/post).
  - All deprecated APIs either removed or wrapped with migration helpers.
  - Plugin boundary enforced via CI script + documentation.
- **Dependencies:** `03_TEMP_ENTITIES...` (SOA adoption), `05_move_to_engine.md` (code sharing), `06_test_helper_refactor_plan.md` (test coverage for refactors).

## Work Breakdown Snapshot
|Stream|Objective|Key Tasks|Exit Criteria|
|---|---|---|---|
|Performance|Speed up queries & loops|Items #1-6 (cache, sorting, partial sort, merge removal, invalidation, component layout)|Perf dashboards show measurable gains, no regressions|
|Simplicity/API Hygiene|Shrink surface area, remove dead code|Items #7-12, #13-18 (query options, creation API, helper cleanup)|Docs + headers updated, migration notes published|
|Plugin/Core Boundary|Protect engine vs product contracts|Items #29 + CI enforcement script|`check_plugin_boundaries.sh` gates PRs, docs published|
|Validation|Prove safety of changes|Instrumentation, benchmarks, cross-project smoke tests|my_name_chef + kart-afterhours compile + run tests cleanly|

### Operating Notes
1. Each improvement entry already lists file touchpoints + validation steps—apply them verbatim when scheduling work.
2. When an improvement depends on SOA (e.g., #1, #4, #6), mark it blocked until Plan 03 hits the relevant milestone.
3. Record perf metrics before/after every merged change; regressions must be rolled back or fixed immediately.

## Overview
Comprehensive review of `vendor/afterhours` ECS library with focus on performance, simplicity, complexity reduction, and API improvements. Each improvement includes validation strategies to measure impact. Also includes plan to separate core library from plugins.

## Performance Improvements (with Validation)

### 1. EntityQuery Caching and Memory Optimization
**File**: `src/core/entity_query.h`
- **Issue**: Query system allocates/deallocates entire entity lists on every query (lines 219-222, 391-422)
- **Current**: `run_query()` creates new `RefEntities` vector every time, even when cached
- **Improvement**: 
  - Use index-based filtering with final copy instead of building intermediate vectors
  - Cache query results more aggressively - only invalidate when entities change
  - Pre-allocate `RefEntities` with `reserve()` based on entity count
  - Use `std::partition` more efficiently (already used but can optimize)
- **Impact**: High - queries are hot path
- **API Breaking**: No
- **Validation**:
  - Add instrumentation to count `RefEntities` allocations per frame
  - Measure average query result size vs entity count
  - Profile query-heavy scenarios (battle simulation, UI rendering)
  - Compare before/after memory allocations using valgrind/massif

### 2. Remove Unnecessary Sorting
**File**: `src/core/entity_query.h:411-419`
- **Issue**: Sorts even when result set has 1 item (line 411 check is too late)
- **Improvement**: Check size before sorting, skip sort entirely for single-item results
- **Impact**: Medium - saves O(n log n) for common case
- **API Breaking**: No
- **Validation**:
  - Count queries that use `orderBy` and result in single-item results
  - Add telemetry: `if (orderby && out.size() > 1)` to measure frequency
  - Run example apps and log sort skip rate

### 3. Partial Sort for `gen_first()`
**File**: `src/core/entity_query.h:257-266`
- **Issue**: Full sort when only first element needed
- **Improvement**: Use `std::nth_element` or `std::partial_sort` for `gen_first()` and `take()`
- **Impact**: Medium - O(n) instead of O(n log n) for first-only queries
- **API Breaking**: No
- **Validation**:
  - Grep codebase for `gen_first()` usage frequency
  - Count queries that use `orderBy` + `gen_first()` pattern
  - Benchmark: compare `std::sort` vs `std::nth_element` for typical query sizes

### 4. System Loop Optimization
**File**: `src/core/system.h:357-372`
- **Issue**: Calls `merge_entity_arrays()` after every system, even when no new entities
- **Improvement**: Only merge when `temp_entities` is non-empty
- **Impact**: Medium - reduces unnecessary work
- **API Breaking**: No
- **Validation**:
  - Instrument `merge_entity_arrays()` to log: (#temp entities, call site) per frame
  - Count how often `temp_entities.empty()` is true when merge is called
  - Measure frame time before/after optimization

### 5. Query Result Caching Invalidation
**File**: `src/core/entity_query.h:244-248`
- **Issue**: `ran_query` flag never gets reset when entities change
- **Improvement**: Track entity count/modification timestamp, invalidate cache on change
- **Impact**: Medium - prevents stale cache issues
- **API Breaking**: No
- **Validation**:
  - Add counter for entity count changes per frame
  - Test: create entity, query, modify entity, query again - verify cache invalidation
  - Measure cache hit rate with proper invalidation

### 6. Entity Component Array Optimization
**File**: `src/core/entity.h:27-28`
- **Issue**: `ComponentArray` uses `std::array<std::unique_ptr<BaseComponent>, max_num_components>` - pointer indirection overhead
- **Improvement**: Consider using `std::variant` or type-erased storage for better cache locality (if component sizes are small)
- **Impact**: High - component access is very hot path
- **API Breaking**: Yes - would require component storage redesign
- **Note**: This is a major architectural change, may not be worth it
- **Validation**:
  - Profile component access patterns (cache misses)
  - Measure average component size
  - Benchmark: compare current vs variant-based storage for hot components

## Simplicity Improvements (with Validation)

### 7. Remove Dead Code: `include_store_entities`
**File**: `src/core/entity_query.h:355-368`
- **Issue**: `_include_store_entities` flag exists but never used in filtering (RFC tags.md line 80 notes this)
- **Improvement**: Remove the flag entirely, replace with tag-based filtering
- **Impact**: Low - reduces confusion
- **API Breaking**: Yes - removes `.include_store_entities()` method
- **Migration**: Use tags instead: `whereHasTag<StoreEntity>()` or `whereMissingTag<StoreEntity>()`
- **Validation**:
  - Grep both `my_name_chef` and `kart-afterhours` for `.include_store_entities(`
  - Count usage: if zero, safe to remove
  - If any exist, document migration path

### 8. Simplify Query Modification Pattern
**File**: `src/core/entity_query.h:26-29`
- **Issue**: Heap-allocated `Modification` objects via `std::unique_ptr` adds complexity
- **Improvement**: Use `std::function` directly or value semantics with type erasure
- **Impact**: Medium - simpler code, but may have performance cost
- **API Breaking**: No (internal only)
- **Validation**:
  - Count average number of modifiers per query
  - If typically <4, consider `small_vector` on stack
  - Benchmark: compare heap allocation vs function overhead

### 9. Consolidate Tag Query Methods
**File**: `src/core/entity_query.h:105-157`
- **Issue**: Many overloads for tag queries (enum, auto, bitset) create API surface
- **Improvement**: Use single template that handles all cases via SFINAE
- **Impact**: Low - reduces code duplication
- **API Breaking**: No
- **Validation**:
  - Count occurrences of each tag query method variant
  - Ensure all variants are used (if not, remove unused ones)

### 10. Remove `OptEntity` Wrapper Complexity
**File**: `src/core/entity.h:263-288`
- **Issue**: `OptEntity` wraps `std::optional<std::reference_wrapper<Entity>>` adding indirection
- **Improvement**: Use `std::optional<Entity*>` or `std::optional<RefEntity>` directly
- **Impact**: Low - simpler, but may need to check usage
- **API Breaking**: Yes - changes return types
- **Note**: Check kart-afterhours usage first
- **Validation**:
  - Grep for `OptEntity` usage in both repos
  - Verify all usages only call: `has_value()`, `asE()`, `operator bool()`
  - If more complex usage exists, document migration

### 11. Simplify System Template Metaprogramming
**File**: `src/core/system.h:63-214`
- **Issue**: Complex template metaprogramming with `#ifdef _WIN32` workarounds for empty type lists
- **Improvement**: Use C++17 `if constexpr` consistently, remove platform-specific branches
- **Impact**: Medium - cleaner code, better maintainability
- **API Breaking**: No
- **Validation**:
  - Test compilation on Windows, macOS, Linux
  - Verify all example apps still compile
  - Check that empty component lists work correctly

### 12. Fix Typo: `permanant_ids`
**File**: `src/core/entity_helper.h:32`
- **Issue**: Spelling error "permanant" should be "permanent"
- **Improvement**: Rename to `permanent_ids`
- **Impact**: Low - code quality
- **API Breaking**: Yes - but internal field name
- **Validation**:
  - Grep for `permanant_ids` to find all usages
  - Verify it's only used internally

## Complexity Reduction (with Validation)

### 13. Remove Unused Query Options
**File**: `src/core/entity_query.h:321-324`
- **Issue**: `QueryOptions` has `force_merge` and `ignore_temp_warning` but logic is complex
- **Improvement**: Simplify to single `auto_merge` flag, remove warning logic (or make it debug-only)
- **Impact**: Medium - simpler API
- **API Breaking**: Yes - changes `QueryOptions` struct
- **Validation**:
  - Count usage of `force_merge` and `ignore_temp_warning` in both repos
  - Measure how often temp warning actually triggers
  - If rarely used, consider removing

### 14. Simplify Entity Creation Options
**File**: `src/core/entity_helper.h:35-37, 74-86`
- **Issue**: `CreationOptions` struct with single bool field
- **Improvement**: Use overloaded functions `createEntity()` vs `createPermanentEntity()` instead
- **Impact**: Low - simpler API
- **API Breaking**: Yes - removes `createEntityWithOptions()`
- **Validation**:
  - Grep for `createEntityWithOptions` usage
  - Verify all callers can use simpler API
  - Count migration effort

### 15. Consolidate Component Access Methods
**File**: `src/core/entity.h:162-187`
- **Issue**: `get()`, `get_with_child()`, `has()`, `has_child_of()` - many similar methods
- **Improvement**: Consider single `get()` with optional template parameter for child-of behavior
- **Impact**: Medium - but may reduce clarity
- **API Breaking**: Yes - significant API change
- **Note**: This might reduce clarity, needs careful design
- **Validation**:
  - Count usage of each method variant
  - Measure if `get_with_child()` is actually used
  - Survey: would unified API be clearer or more confusing?

### 16. Remove Redundant Query Methods
**File**: `src/core/entity_query.h:230-237`
- **Issue**: `has_values()` and `is_empty()` both call `run_query()` with same options
- **Improvement**: Make `is_empty()` call `!has_values()` to avoid duplication
- **Impact**: Low - code deduplication
- **API Breaking**: No
- **Validation**:
  - Count usage of both methods
  - Verify they're equivalent (test with edge cases)

### 17. Simplify Library Template
**File**: `src/library.h:17-122`
- **Issue**: `Library<T>` has many iterator methods that just forward to `std::map`
- **Improvement**: Inherit from or use composition with `std::map` more directly
- **Impact**: Low - less boilerplate
- **API Breaking**: No
- **Validation**:
  - Check if all iterator methods are actually used
  - Verify inheritance vs composition doesn't break anything

### 18. Remove Unused `entity_type` Field
**File**: `src/core/entity.h:35`
- **Issue**: `int entity_type = 0;` field exists but may not be used
- **Improvement**: Remove if unused, or document its purpose
- **Impact**: Low - reduces entity size
- **API Breaking**: Yes - removes field
- **Note**: Check usage in both projects first
- **Validation**:
  - Grep for `entity_type` in both repos
  - If unused, safe to remove
  - If used, document purpose or consider better alternative

## API Improvements (with Validation)

### 19. Add Query Builder Fluent Interface
**File**: `src/core/entity_query.h`
- **Issue**: Current API requires chaining, but could be more ergonomic
- **Improvement**: Add helper functions like `query().whereHas<Component>().whereHasTag<Tag>().gen()`
- **Impact**: Low - convenience
- **API Breaking**: No - additive
- **Validation**:
  - Survey: do users find current API ergonomic?
  - Check if kart-afterhours custom queries would benefit

### 20. Fix Component ID Overflow Check
**File**: `src/core/base_component.h:24-34`
- **Issue**: Overflow check is commented out (todo.md line 59)
- **Improvement**: Fix the check or use `std::atomic` properly
- **Impact**: High - memory safety
- **API Breaking**: No
- **Validation**:
  - Run `component_overflow_test` example
  - Add unit test with low `AFTER_HOURS_MAX_COMPONENTS`
  - Verify `VALIDATE` triggers correctly

### 21. Make `gen_random()` Use Proper RNG
**File**: `src/core/entity_query.h:299-306`
- **Issue**: Uses `rand()` which is not thread-safe and low quality
- **Improvement**: Accept RNG as parameter (already has overload) or use `std::random_device`
- **Impact**: Medium - better randomness
- **API Breaking**: Yes - changes default behavior, but has overload
- **Validation**:
  - Count usage of `gen_random()` without RNG parameter
  - Test: verify randomness quality
  - Benchmark: compare `rand()` vs `std::mt19937`

### 22. Add Query Result Iterators
**File**: `src/core/entity_query.h`
- **Issue**: Must call `gen()` to get vector, then iterate
- **Improvement**: Make `EntityQuery` itself iterable, or return iterator range
- **Impact**: Low - convenience
- **API Breaking**: No - additive
- **Validation**:
  - Count patterns like `for (auto& e : query.gen())`
  - Survey: would iterator interface be useful?

### 23. Const-Correctness for Systems
**File**: `src/core/system.h:159-161, 307-313`
- **Issue**: Const systems can't be used in update loop (todo.md line 49)
- **Improvement**: Support const systems in update loop, or make const/non-const versions
- **Impact**: Medium - better const-correctness
- **API Breaking**: No - additive
- **Validation**:
  - Grep for `const` systems in both repos
  - Count how many systems could be const
  - Test: register const system, verify it works

### 24. Remove `values_ignore_cache()` Public API
**File**: `src/core/entity_query.h:238-242`
- **Issue**: Public method that's only used internally
- **Improvement**: Make private or remove, use `run_query()` directly
- **Impact**: Low - cleaner API
- **API Breaking**: Yes - removes public method
- **Validation**:
  - Grep for `values_ignore_cache` usage
  - If only internal, make private
  - If external usage exists, document migration

## Code Quality (with Validation)

### 25. Fix Logging Implementation
**File**: `src/logging.h:10-47`
- **Issue**: Uses `va_list` which is C-style, not type-safe
- **Improvement**: Use variadic templates with `fmt` library (already in vendor)
- **Impact**: Medium - type safety, better performance
- **API Breaking**: Yes - changes function signatures
- **Note**: fmt is already in vendor, just needs integration
- **Validation**:
  - Count `log_*` usage in both repos
  - Test: compile with new API, verify format strings work
  - Benchmark: compare `va_list` vs `fmt` performance

### 26. Use `std::expected` Consistently
**File**: `src/library.h:41-48, 90-106`
- **Issue**: Some methods return `expected`, others throw or return null
- **Improvement**: Standardize on `std::expected` for all fallible operations
- **Impact**: Low - consistency
- **API Breaking**: Yes - changes return types
- **Validation**:
  - Audit all fallible methods in library
  - Count methods that throw vs return expected
  - Plan migration for each

### 27. Remove Platform-Specific Workarounds
**File**: `src/core/system.h:96-118, 134-166`
- **Issue**: `#ifdef _WIN32` branches for template specialization
- **Improvement**: Use C++17 `if constexpr` which works on all platforms
- **Impact**: Medium - cleaner code
- **API Breaking**: No
- **Validation**:
  - Test compilation on Windows, macOS, Linux
  - Verify all example apps compile
  - Check CI builds pass

### 28. Fix Reserve Calculation
**File**: `src/core/entity_helper.h:45-47`
- **Issue**: `reserve(sizeof(EntityType) * 100)` reserves bytes, not elements
- **Improvement**: `reserve(100)` to reserve 100 entities
- **Impact**: Low - bug fix
- **API Breaking**: No
- **Validation**:
  - Add assertion: verify capacity after reserve
  - Run entity creation stress test
  - Measure: does current code actually cause reallocations?

## Plugin/Core Separation Strategy

### 29. Define Public API Boundary
**Goal**: Clearly separate core ECS (public API) from plugins (examples using public API)

**Current State**:
- Core headers: `core/entity.h`, `core/entity_query.h`, `core/system.h`, `core/base_component.h`, `core/entity_helper.h`
- Plugins: `plugins/input_system.h`, `plugins/ui/`, `plugins/animation.h`, etc.
- Problem: Plugins may use private APIs from `EntityHelper` internals

**Improvements**:

#### 29.1 Audit Plugin Dependencies
- **Validation**:
  - For each plugin file, list all includes
  - Identify which includes are from `core/` (allowed) vs internal helpers
  - Check if plugins access private members (e.g., `EntityHelper::singletonMap` directly)

#### 29.2 Create Public API Header
- **File**: `src/afterhours.h` or `src/public_api.h`
- **Content**: Only expose public methods from core headers
- **Validation**:
  - Create test: plugins can only include this header
  - Verify all plugins compile with only public API

#### 29.3 Move Plugins to Examples
- **Structure**:
  ```
  vendor/afterhours/
    src/core/          # Core ECS (public API)
    examples/plugins/  # Reference plugins (use only public API)
      input/
      ui/
      animation/
  ```
- **Validation**:
  - Each plugin should compile standalone
  - Document: "This is how you write your own plugin"
  - CI: verify plugins don't include private headers

#### 29.4 Plugin Registration System
- **Idea**: Provide `PluginRegistry` that allows external plugins to register systems/components
- **API**:
  ```cpp
  namespace afterhours {
    struct Plugin {
      virtual void register_systems(SystemManager&) = 0;
      virtual void initialize() = 0;
      virtual void shutdown() = 0;
    };
    
    void register_plugin(std::unique_ptr<Plugin>);
  }
  ```
- **Validation**:
  - Create example external plugin that uses only public API
  - Verify it can register systems without modifying core

#### 29.5 Document Plugin Interface
- **File**: `docs/PLUGINS.md`
- **Content**:
  - How to write a plugin (step-by-step)
  - What APIs are available (public only)
  - Examples from existing plugins
  - How to register custom components/systems
- **Validation**:
  - New developer can write plugin following docs
  - Plugin compiles without accessing private APIs

#### 29.6 Refactor Existing Plugins
- **For each plugin**:
  1. List private API dependencies
  2. Either:
     - Move needed functionality to public API (if generally useful)
     - Refactor plugin to use public API only
     - Document why private API is needed (if unavoidable)
- **Validation**:
  - Run `include-what-you-use` on plugin files
  - Verify no private includes remain
  - Test: plugins still work after refactor

#### 29.7 CI Enforcement
- **Script**: `scripts/check_plugin_boundaries.sh`
- **Checks**:
  - Plugins in `examples/plugins/` can only include public headers
  - No direct access to `EntityHelper` internals
  - All plugin code compiles standalone
- **Validation**:
  - Add to CI pipeline
  - Verify it catches violations

### 30. Plugin Packaging Examples
- **Goal**: Show how external projects can create plugins

**Structure**:
```
my_custom_plugin/
  plugin.h          # Plugin interface implementation
  systems.h         # Custom systems
  components.h      # Custom components
  README.md         # How to use
```

**Validation**:
- Create example external plugin project
- Verify it integrates with afterhours
- Document integration steps

## Measurement Infrastructure

### Add Profiling Helpers
**File**: `src/core/profiling.h` (new, optional)

```cpp
#ifdef AFTER_HOURS_PROFILING
  #define PROFILE_QUERY(query) QueryProfiler _prof(query)
  #define PROFILE_MERGE() MergeProfiler _prof
#else
  #define PROFILE_QUERY(query)
  #define PROFILE_MERGE()
#endif
```

**Metrics to Track**:
- Query count per frame
- Average query result size
- Merge frequency and temp entity count
- Component access patterns
- System execution time

**Validation**:
- Enable profiling in example apps
- Collect metrics during typical gameplay
- Analyze: which optimizations have highest impact?

## Summary

**High Impact, No API Break** (Implement First):
- #1: Query caching and memory optimization
- #2: Remove unnecessary sorting
- #3: Partial sort for gen_first()
- #5: System loop optimization
- #6: Query cache invalidation
- #20: Fix component ID overflow check
- #28: Fix reserve calculation

**Medium Impact, No API Break**:
- #8: Simplify query modification pattern
- #11: Simplify system templates
- #16: Remove redundant query methods
- #23: Const-correctness for systems
- #27: Remove platform workarounds

**High Impact, API Breaking** (Requires Migration):
- #7: Remove `include_store_entities` (use tags)
- #13: Simplify query options
- #14: Simplify entity creation
- #25: Fix logging with fmt

**Plugin Separation** (High Priority):
- #29: Complete plugin/core separation
- Move plugins to examples
- Document public API boundary
- CI enforcement

**Low Priority**:
- #9: Consolidate tag methods
- #10: Simplify OptEntity
- #12: Fix typo
- #17: Simplify library template
- #18: Remove unused field
- #19: Add fluent interface helpers
- #22: Add iterators
- #24: Remove internal API
- #26: Use expected consistently

**Major Refactors** (Consider for Future):
- #4: Component array optimization
- #15: Consolidate component access
- #30: SoA architecture

## Outstanding Questions
1. **Sequencing vs Plan 03:** Which improvements must strictly wait for SOA landing, and can any be prototyped against AoS without risking rework?
2. **API Deprecation Path:** Do we provide deprecation warnings (e.g., `#pragma message`) for breaking changes (#7, #13, #14, #25) or just ship release notes?
3. **Plugin Boundary Enforcement:** Is the proposed shell script sufficient, or do we need a more robust static analyzer to audit includes/types?
4. **Documentation Venue:** Should plugin boundary + public API guidance live in `docs/PLUGINS.md`, the main README, or a new developer portal page?
5. **Performance Budget Ownership:** Who signs off on perf regressions if an improvement temporarily slows another system (e.g., logging overhaul), and what rollback policy do we adopt?

