# Code to Move to Engine Analysis

## At-a-Glance
- **Sequence:** 05 / 21 — codifies what must live inside `vendor/afterhours` so every game benefits and duplication disappears.
- **Vision:** Gradually upstream shared utilities/systems into engine plugins without breaking my_name_chef delivery cadence.
- **Current Focus:** High-priority bucket (Math, Utils, Translation Manager) ready for execution once Plan 03 lands; medium/low priorities staged behind validation + owners.
- **Guardrails:** Each migration needs (a) test coverage in engine + consumer, (b) documentation of extension points, (c) rollout plan for kart/tetr.
- **Dependencies:** SOA/engine hygiene from Plan 03 + library improvements from Plan 04; test infrastructure from Plan 06 ensures parity.

## Work Breakdown Snapshot
|Wave|Scope|Example Items|Checklist|
|---|---|---|---|
|Wave 1: Copy-Paste Offenders|Pure utility duplication|#1 Math, #2 Utils|API parity confirmed, consumers switched, delete originals|
|Wave 2: Service Singletons|Translation, resources, settings, sound libs|#3, #5, #6, #7|Parameterization hooks, docs, migration guide issued|
|Wave 3: Rendering / Content Helpers|Texture/shader libs, query extensions|#4, #8, #9|Performance baseline recorded, toggles for product-specific code|
|Wave 4: Testing + Tooling|Test framework, navigation/UI patterns|#14 (testing), #15 (UI navigation)|Shared harness green in CI across repos|
|Wave 5: Strategic Systems|Render backend, multipass, state manager|#11-13|Executive sign-off (higher blast radius) + release alignment|

### Execution Notes
1. **One file per PR** where feasible; easier diff review and rollback.
2. **Adopt “move-then-delete” approach**: upstream to engine, switch consumers, remove local copy in same change.
3. **Compatibility Shims:** For public APIs, keep adapters in my_name_chef until kart-afterhours + tetr are updated.
4. **Telemetry:** Track reduction in duplicate LOC + maintenance cost per migration wave.

This document identifies code in `my_name_chef` that would be beneficial to move into the `vendor/afterhours` engine for use across multiple games (my_name_chef, kart-afterhours, tetr-afterhours).

## High Priority - Nearly Identical Across Games

### 1. Math Utilities (`src/math_util.h`)
**Status**: Identical in both `my_name_chef` and `kart-afterhours`

**Location**: `src/math_util.h`

**Why Move**: 100% duplicate code - identical implementations across games

**Functions to Move**:
- `sgn()` - Sign function
- `distance_sq()` - Squared distance calculation
- `vec_dot()`, `vec_cross()`, `vec_mag()`, `vec_norm()` - Vector math utilities
- `to_radians()`, `to_degrees()` - Angle conversions
- `is_point_inside()`, `rect_center()` - Rectangle utilities
- `truncate_to_minutes()`, `truncate_to_seconds()` - Time formatting
- `vec_rand_in_box()` - Random point generation

**Engine Location**: `vendor/afterhours/src/core/math_util.h` (new file)

**Notes**: These are pure utility functions with no game-specific dependencies.

---

### 2. Utils (`src/utils.h`)
**Status**: Identical in both `my_name_chef` and `kart-afterhours`

**Location**: `src/utils.h`

**Why Move**: 100% duplicate code

**Content**:
- `overloaded` struct - Variant visitor helper
- `to_string(float, int)` - Formatted float to string

**Engine Location**: `vendor/afterhours/src/utils.h` (new file) or merge with existing utilities

---

### 3. Translation Manager (`src/translation_manager.h`, `.cpp`)
**Status**: Very similar implementation across games, minor game-specific parameter differences

**Location**: `src/translation_manager.h`, `src/translation_manager.cpp`

**Why Move**: Core i18n functionality that all games need, with only enum differences

**Key Components**:
- `TranslatableString` class - String wrapper with formatting support
- `TranslationManager` singleton - Language switching and font management
- Global helper functions (`get_string()`, `set_language()`, etc.)
- CJK font loading utilities

**Engine Location**: `vendor/afterhours/src/plugins/translation.h` (new plugin)

**Customization Points**:
- `Language` enum (game-specific)
- `i18nParam` enum (game-specific, can be templated/extended)
- Translation string mappings (game-specific data)

**Pattern**: Core translation infrastructure with game-specific enums/data

---

## Medium Priority - Reusable Patterns

### 4. EntityQuery Extensions (`src/query.h`)
**Status**: Similar patterns with game-specific extensions

**Location**: `src/query.h`

**Why Move**: Common query patterns that multiple games use

**Patterns to Extract**:
- `WhereInRange` - Spatial queries by distance
- `WhereOverlaps` - Rectangle overlap detection
- `orderByDist()` - Sorting by distance
- Rectangle overlap utility function

**Engine Location**: `vendor/afterhours/src/core/entity_query_spatial.h` (extension to existing query system)

**Customization**: Game-specific query modifiers (like `WhereSlotID` in my_name_chef) remain game-specific

---

### 5. Resources/Files System (`src/resources.h`, `.cpp`)
**Status**: Similar pattern across games (appears in both my_name_chef and kart-afterhours)

**Location**: `src/resources.h`, `src/resources.cpp`

**Why Move**: Standard file/resource management pattern

**Key Features**:
- Game folder management (using sago for cross-platform paths)
- Settings file discovery (resource folder, current directory, user folder)
- Resource path fetching
- Directory iteration helpers

**Engine Location**: `vendor/afterhours/src/plugins/files.h` (new plugin)

**Customization Points**:
- Game folder name (parameterized)
- Settings file name (parameterized)
- Resource folder structure (configurable)

**Note**: Already uses singleton pattern compatible with engine

---

### 6. Settings System (`src/settings.h`, `.cpp`)
**Status**: Similar structure but different settings per game

**Why Move**: Common pattern for JSON-based settings persistence

**Reusable Components**:
- `ValueHolder<T>` template - Generic value wrapper
- `Pct` - Percentage value with clamping (0-1)
- JSON serialization helpers for common types
- Settings file loading with fallback chain
- Settings refresh/application system

**Engine Location**: `vendor/afterhours/src/plugins/settings.h` (new plugin)

**Pattern**: Generic settings infrastructure that games extend with their specific `S_Data` structures

**Game-Specific**:
- Actual settings fields (resolution, volume, language, etc.)
- Game-specific setting types

**Reusable Code**:
```cpp
template<typename SettingsData>
struct SettingsManager {
  // Generic load/save/refresh infrastructure
  bool load_save_file(const Files& files);
  void write_save_file();
  void refresh_settings();
  // ... generic implementation
};
```

---

### 7. Sound/Music Library Pattern (`src/sound_library.h`, `music_library.h`)
**Status**: Very similar implementation pattern in both games

**Location**: `src/sound_library.h`, `src/music_library.h`

**Why Move**: Common resource loading and management pattern

**Pattern to Extract**:
- Library wrapper around `Library<T>` base class
- Volume management with update callbacks
- Prefix-based sound lookup and playback strategies
- Singleton pattern with proper cleanup

**Engine Location**: `vendor/afterhours/src/plugins/audio.h` (new plugin) or extend existing audio support

**Game-Specific**:
- Sound file enums
- Loading strategies (specific files vs. prefix matching)

**Reusable**: The library wrapper pattern, volume management, singleton structure

---

### 8. Texture Library (`src/texture_library.h`)
**Status**: Simple wrapper pattern, similar across games

**Location**: `src/texture_library.h`

**Why Move**: Simple, reusable resource loading pattern

**Pattern**: Singleton wrapper around `Library<Texture2D>` with texture-specific loading

**Engine Location**: Already partially exists in `vendor/afterhours/src/plugins/texture_manager.h` - could enhance or consolidate

---

### 9. Shader Library (`src/shader_library.h`)
**Status**: Unique to my_name_chef, but useful pattern

**Location**: `src/shader_library.h`

**Why Move**: Useful for games that need shader management

**Key Features**:
- Enum-based shader type system
- Automatic shader loading via magic_enum
- Uniform location caching
- Shader value updates (time, resolution, etc.)

**Engine Location**: `vendor/afterhours/src/plugins/shaders.h` (new plugin)

**Customization**: Game-specific `ShaderType` and `UniformLocation` enums

---

### 10. Preload System (`src/preload.h`, `.cpp`)
**Status**: Similar singleton initialization pattern

**Location**: `src/preload.h`, `src/preload.cpp`

**Why Move**: Common initialization pattern

**Pattern**: Singleton with initialization chaining (`init()` -> `make_singleton()`)

**Engine Location**: Could be part of engine initialization utilities

**Note**: Very simple, may not warrant separate move unless pattern becomes more complex

---

## Lower Priority - Game-Specific but Potentially Useful

### 11. Game State Manager Pattern (`src/game_state_manager.h`)
**Status**: Game-specific screens/states but similar pattern

**Location**: `src/game_state_manager.h`

**Why Move**: Common state management pattern (Menu/Playing/Paused + Screen navigation)

**Reusable Pattern**:
- State machine with screen management
- Next screen queuing (`next_screen` pattern)
- Screen transition helpers
- State query helpers (`is_game_active()`, `is_paused()`, etc.)

**Engine Location**: `vendor/afterhours/src/plugins/game_state.h` (optional, game-agnostic base)

**Game-Specific**: Screen enums, state enums, transition logic

**Note**: More game-specific than others, but the pattern is reusable

---

### 12. Render Backend (`src/render_backend.h`)
**Status**: Unique to my_name_chef for headless testing

**Location**: `src/render_backend.h`

**Why Move**: Useful for headless testing/CI across all games

**Purpose**: Wraps raylib rendering calls with headless mode checks

**Engine Location**: `vendor/afterhours/src/plugins/render_backend.h` or as part of window_manager plugin

**Benefits**: Enables headless testing across all engine games

---

### 13. Multipass Renderer (`src/multipass_renderer.h`, `multipass_integration.h`)
**Status**: Unique to my_name_chef, but useful for games needing layered rendering

**Location**: `src/multipass_renderer.h`, `src/multipass_integration.h`

**Why Move**: Useful for games with complex rendering pipelines

**Features**:
- Render pass configuration (priority-based)
- Pass enable/disable system
- Clear configuration per pass
- Entity grouping by render priority

**Engine Location**: `vendor/afterhours/src/plugins/multipass_renderer.h` (new plugin)

**Note**: Currently tied to `HasShader` component - could be generalized

---

### 14. Testing Framework (`src/testing/`)
**Status**: Unique to my_name_chef, but very useful for all games

**Location**: `src/testing/` directory

**Why Move**: Testing infrastructure benefits all games

**Components**:
- `TestSystem` - Test execution within game loop
- `UITestHelpers` - UI element validation and interaction
- `TestInteraction` - Safe game state mutations for tests
- `TestContext` - Test environment management
- Test runner script pattern

**Engine Location**: `vendor/afterhours/src/plugins/testing.h` (new plugin)

**Key Features**:
- Headless test execution (`--run-test` flag)
- UI element querying and validation
- Entity validation helpers
- Screen transition testing
- Test timeout protection

**Benefits**: 
- Consistent testing across all engine games
- Automated test discovery and execution
- UI testing utilities reusable across games

**Customization**: Game-specific test helpers and validators

---

### 15. UI Navigation Pattern (`src/ui/navigation.h`, `.cpp`)
**Status**: Similar navigation stack pattern

**Location**: `src/ui/navigation.h`, `src/ui/navigation.cpp`

**Why Move**: Common menu navigation pattern

**Pattern**:
- Navigation stack component
- `navigation::to()` and `navigation::back()` helpers
- Navigation system integration

**Engine Location**: Could be part of UI plugin or separate navigation plugin

**Note**: Similar pattern exists in kart-afterhours - could be unified

---

## Implementation Recommendations

### Phase 1: High Priority (Near Identical Code)
1. **Math Utilities** - Zero risk, pure functions
2. **Utils** - Zero risk, simple utilities
3. **Translation Manager Core** - High value, requires abstraction for game-specific enums

### Phase 2: Medium Priority (Reusable Patterns)
4. **Query Extensions** - Extend existing query system
5. **Resources/Files** - Standardize file management
6. **Settings Infrastructure** - Generic settings system
7. **Audio Libraries** - Unify audio resource management

### Phase 3: Lower Priority (Potentially Useful)
8. **Testing Framework** - High value for development
9. **Render Backend** - Enable headless testing
10. **Multipass Renderer** - If needed by other games

### Considerations

**Abstraction Strategy**:
- Use templates for game-specific enums (Language, Settings fields, etc.)
- Provide base classes that games extend
- Use composition over inheritance where possible

**Breaking Changes**:
- Moving code to engine requires updating includes in all games
- Consider versioning or feature flags for gradual migration

**Dependencies**:
- Some utilities depend on raylib types (vec2, Rectangle) - keep these dependencies
- Translation manager depends on magic_enum - already used in engine
- Settings depends on nlohmann/json - check if engine uses this

**Testing**:
- Before moving, verify code works in at least 2 games
- Create engine tests for moved utilities
- Update all games after move

---

## Files Already in Engine (For Reference)

Reviewing `vendor/afterhours/src/` shows:
- ✅ Core ECS system (`core/`)
- ✅ UI system (`plugins/ui/`)
- ✅ Input system (`plugins/input_system.h`)
- ✅ Window manager (`plugins/window_manager.h`)
- ✅ Texture manager (`plugins/texture_manager.h`)
- ✅ Singleton pattern (`singleton.h`)
- ✅ Library base class (`library.h`)
- ✅ Logging (`logging.h`)

**Missing from engine** (candidates for addition):
- Math utilities
- Translation/i18n system
- Settings persistence
- Files/resource management
- Testing framework
- Audio resource management

---

## Summary

**Total Candidates**: 15 areas identified

**High Priority**: 3 items (math_util, utils, translation_manager core)
**Medium Priority**: 7 items (query extensions, resources, settings, audio, etc.)
**Lower Priority**: 5 items (game state, render backend, testing, etc.)

**Estimated Impact**:
- **Code Reduction**: ~2000-3000 lines of duplicate code across games
- **Maintenance**: Single source of truth for common utilities
- **Consistency**: All games use same math, i18n, and resource patterns
- **Testing**: Unified testing framework benefits all games

**Risk Assessment**:
- **Low Risk**: Math utilities, utils (pure functions)
- **Medium Risk**: Translation manager (requires enum abstraction)
- **Higher Risk**: Settings, Game State (more game-specific)

## Outstanding Questions
1. **Release Coordination:** Should each migration wave align with a tagged engine release so downstream repos can upgrade predictably, or do we embed changes and bump submodules ad hoc?
2. **Ownership & Review:** Who signs off on moving shared subsystems (e.g., audio, translation) to guarantee that kart-afterhours/tetr-afterhours requirements are baked in?
3. **Versioning Strategy:** Do we introduce semantic versioning for the engine plus changelogs, or continue pinning commit hashes across games?
4. **Testing Footprint:** When we relocate the test framework/UI navigation, do we port existing tests into the engine repo or rely on consumer projects for coverage?
5. **Documentation Venue:** Should migration guides and plugin instructions live alongside the engine (e.g., `/docs/engine/`), or reside in each consumer repo to capture product-specific notes?

