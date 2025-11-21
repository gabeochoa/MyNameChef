# Battle Server Implementation - Plan and Questions

## Implementation Plan Overview

### Goal
Create a server that can simulate battles using the **exact same code** as the main game to ensure identical simulation results. The client will replay battles using the server's seed, and the results must match exactly.

### Architecture Overview

**Shared Battle Core**:
- Extract battle systems into a shared library approach (header-only or static library)
- Server and client both use the same ECS systems, components, and logic
- Server runs in headless mode (no rendering, no Raylib dependencies)
- Both use `SeededRng` with server-provided seed for determinism

**Server Components**:
1. **Battle Simulation Engine**: Reuses game's battle systems in headless mode
2. **HTTP API Server**: Exposes REST endpoints for battle requests
3. **Team/Seed Management**: Handles opponent selection and seed generation

### Implementation Steps

#### Step 1: Create Headless Battle Library
**Files**: Create `src/server/` directory structure
- `src/server/battle_simulator.h` / `.cpp` - Core battle simulation wrapper
- `src/server/battle_context.h` / `.cpp` - Headless ECS context (no rendering dependencies)

**Approach**:
- Use existing `render_backend::is_headless_mode` flag
- Create minimal ECS context that doesn't require Raylib
- Reuse all battle systems: `ResolveCombatTickSystem`, `TriggerDispatchSystem`, `EffectResolutionSystem`, `ComputeCombatStatsSystem`, etc.
- Systems must check `is_headless_mode` and skip rendering/animation logic

#### Step 2: Create HTTP Server Infrastructure
**Files**: 
- `src/server/http_server.h` / `.cpp` - HTTP server implementation
- `src/server/battle_api.h` / `.cpp` - Battle API endpoints

**API Endpoints**:
- `POST /battle` - Request battle simulation
  - Request: `{ userId, teamId, teamSnapshot: {...} }` (JSON format with levels and powerups)
  - Response: `{ seed: uint64, opponentId: string, outcomes: [...], events: [...], checksum: string }`
- `GET /health` - Health check

**Technology**: Use `cpp-httplib` (vendored in `vendor/cpp-httplib/`)

#### Step 3: Integrate Battle Systems into Server
**Files**: Update server to use game's battle systems
- `src/server/battle_simulator.cpp` - Wraps battle simulation
- Initialize ECS with only battle systems (no rendering systems)
- **Automatic system registration**: Create helper function that automatically discovers and registers all battle systems from main game
- Register systems in same order as main game for determinism
- Skip render systems (they're no-ops anyway)

**Battle Completion Detection**:
- Check `CombatQueue.complete` flag (set by `StartCourseSystem` when one team has no remaining dishes)
- Verify: no active dishes remain on at least one team

#### Step 4: Team Loading and Opponent Selection
**Files**: 
- `src/server/team_manager.h` / `.cpp` - Team loading and opponent selection
- Reuse existing `BattleTeamLoaderSystem` logic

**Functionality**:
- Load player team from JSON (same format as existing `output/battles/*.json` files)
- **Team JSON must include**: dish types, slots, levels, and powerups for each dish
- Select opponent team randomly from `resources/battles/opponents/*.json` pool
- **Opponent file tracking**: On server startup, scan and count opponent files; log warning if count exceeds threshold (e.g., >1000 files)
- Generate deterministic seed using `std::random_device` for battle
- Initialize battle with both teams

#### Step 5: Battle Result Serialization
**Files**: 
- `src/server/battle_result_serializer.h` / `.cpp` - Convert battle state to JSON

**Output Format**:
```json
{
  "seed": 123456789,
  "opponentId": "opponent_abc123",
  "checksum": "abc123...",
  "debug": false,
  "outcomes": [
    { "slotIndex": 0, "winner": "Player", "ticks": 3 }
  ],
  "events": [
    { "t_ms": 0, "course": 0, "type": "OnStartBattle" },
    { "t_ms": 100, "course": 0, "type": "OnServe", "entityId": 101 },
    { "t_ms": 700, "course": 0, "type": "OnBiteTaken", "damage": 2 }
  ],
  "snapshots": [] // Only included when debug=true
}
```

**Note**: 
- Full event log with single `debug` flag to turn off debug state for production (smaller response)
- State snapshots only included when `debug=true` (for testing/verification)
- No animation events (only combat events needed for simulation)
- Entity IDs: TODO - need design for dishes that generate new dishes (won't match across executables)

#### Step 6: Build System Integration
**Files**: Update `make.lua`

**New Target**:
- `target("battle_server")` - Server executable
- Exclude rendering systems and Raylib dependencies
- Link HTTP library (`cpp-httplib`)
- Set `is_headless_mode = true` at compile time
- Share source files with main game (compile same `.cpp` files)

#### Step 7: Determinism Verification
**Files**: Create test utilities
- `src/server/determinism_test.h` / `.cpp` - Verify identical results

**Test**:
- Run server simulation with seed X, capture all events
- Run client simulation with seed X, capture all events
- Compare: same seed must produce identical sequence
- Verify: damage values, trigger events, effect applications all match
- **Checksum validation**: Server provides checksum/hash of final battle state in response; client can verify its simulation matches
- **Testing approach**: Separate test suite (`src/server/tests/`), unit tests (no server needed) + integration tests (with `--use-server` flag to spin up server instance)
- **Comparison method**: Use checksum/hash comparison (easier, client doesn't need to send back to server until "next round")
- **Test data**: Use specific teams for validation, but also test with real teams to catch edge cases

### Key Decisions Made

- **HTTP Library**: `cpp-httplib` (vendored in `vendor/cpp-httplib/`)
- **Port**: 8080 (configurable via `--port` command-line argument)
- **Threading**: Single-threaded
- **Raylib**: Not initialized, use stubs in `render_backend.h`
- **System Registration**: All battle systems in same order, automatic discovery mechanism needed
- **Render Systems**: Skip render systems
- **Timestep**: Fixed 1/60s (but faster execution, no delays - still deterministic)
- **Team Format**: Existing JSON format with levels and powerups
- **Opponent Selection**: Random from pool, track file count on startup
- **Seed Generation**: `std::random_device`, always in response
- **Validation**: Checksum/hash of final state in response (client validates, doesn't send back until "next round")
- **Error Handling**: HTTP 400 for invalid teams (validate as much as possible), HTTP 500 for server errors
- **Initialization**: `ServerContext::initialize()` function
- **Result Serialization**: Full event log with `debug` flag (turn off for production), snapshots only in debug mode, no animations
- **Logging**: Existing `log_info()` system, INFO level
- **Testing**: Separate test suite, unit + integration (with `--use-server` flag), checksum comparison
- **Build**: Separate make target, share source files with main game
- **Configuration**: Config file (JSON/YAML)
- **Concurrency**: One battle at a time (scale with multiple instances + message queue)
- **Results Persistence**: Save results, but audit and keep only last 10 (implement cleanup/rotation)
- **Animation**: Skip all animations entirely (faster execution)
- **Entity Creation**: Use same exact logic as client (`BattleTeamLoaderSystem`), but skip Transform components (no positions needed) and all visual/rendering components (sprite/texture) for performance
- **Client Integration**: HTTP POST with JSON body (production), direct function call for testing, no client library needed, parse JSON and extract seed to initialize battle

### Testing and Validation Strategy

**Test Categories**:
1. **Existing Test Compatibility**: Ensure all current game tests still pass
2. **Server Determinism Tests**: Verify server produces deterministic results
3. **Server-Client Simulation Matching**: Verify client simulation matches server exactly
4. **State Snapshot Comparison**: Capture and compare battle state at any point
5. **Event Sequence Verification**: Verify trigger events fire in identical order
6. **Integration Test**: Full server-client flow test
7. **Regression Test Suite**: Ensure existing battle functionality still works

**Validation Commands** (after each step):
1. Build server: `make build battle_server`
2. Build client: `make build my_name_chef`
3. Run existing tests: `./scripts/run_all_tests.sh` (verify no regressions)
4. Run server determinism: `./output/battle_server.exe --test-determinism`
5. Run client-server match: `./output/my_name_chef.exe --run-test validate_server_client_match`
6. Test API: `curl -X POST http://localhost:8080/battle -d '{"userId":"test","teamId":"t1","teamSnapshot":{...}}'`
7. Verify response format matches expected JSON
8. Compare server/client results with same seed (must be identical)

---

## Questions for Clarification

This document lists all questions that need to be answered before a junior engineer can implement the battle server. Each question has multiple choice options with a recommended default.

## 1. HTTP Server Library & Technology

**Q1.1: Which HTTP library should we use?**
- a) `cpp-httplib` (header-only, lightweight, recommended default) ✅ **USER ANSWER: a**
- b) `crow` (C++ microframework)
- c) `libcurl` with custom wrapper
- d) Custom implementation using system sockets
- **Decision**: Use `cpp-httplib`

**Q1.2: Should the HTTP library be vendored or system-installed?**
- a) Vendor in `vendor/cpp-httplib/` (recommended for consistency) ✅ **USER ANSWER: a**
- b) System-installed via package manager
- c) Git submodule
- **Decision**: Vendor in `vendor/cpp-httplib/`

**Q1.3: What port should the server listen on?**
- a) 8080 (recommended default) ✅ **USER ANSWER: a**
- b) 3000
- c) Configurable via environment variable or config file
- d) Command-line argument `--port`
- **Decision**: Default to 8080, with command-line override `--port`

**Q1.4: Should the server be single-threaded or multi-threaded?**
- a) Single-threaded (simpler, recommended for MVP) ✅ **USER ANSWER: a**
- b) Multi-threaded with thread pool
- c) One thread per request
- **Decision**: Single-threaded for initial implementation

## 2. Server Architecture & Initialization

**Q2.1: Should the server require Raylib/OpenGL initialization?**
- a) No Raylib at all - pure headless (recommended) ✅ **USER ANSWER: a**
- b) Initialize Raylib but don't render (for compatibility)
- c) Use existing `Preload::init(headless=true)` pattern
- **Decision**: Server should not initialize Raylib

**Q2.2: How should we handle dependencies that currently require Raylib?**
- a) Refactor to remove Raylib dependencies from battle systems
- b) Create stub implementations for Raylib functions ✅ **USER ANSWER: b**
- c) Use conditional compilation to exclude Raylib code
- **Decision**: Create stubs in `render_backend.h` that are no-ops in server mode

**Q2.3: Should the server have its own main() or be a library?**
- a) Separate executable with its own `main()` (recommended) ✅ **USER ANSWER: a**
- b) Library that can be embedded in other programs
- c) Both - executable + library
- **Decision**: Separate `src/server/main.cpp`

**Q2.4: How should we initialize the ECS/afterhours framework?**
- a) Minimal initialization (no window, no rendering)
- b) Same as headless test mode but without window creation
- c) Create new initialization function specifically for server ✅ **USER ANSWER: c**
- **Decision**: Create `ServerContext::initialize()` that sets up ECS without rendering

## 3. Battle System Registration & Execution

**Q3.1: Which systems should be registered in the server?**
- a) All battle systems in exact same order as main game (recommended) ✅ **USER ANSWER: a**
- b) Only combat logic systems (exclude UI, rendering, input)
- c) Configurable list
- **Decision**: All battle systems in same order, BUT make it automatic so new systems are included automatically
- **Registration Approach**: ✅ **USER ANSWER: Option D** - Extract shared function that both `main.cpp` and server call
  - Create `register_battle_systems(SystemManager&)` function
  - Both `main.cpp` and server call this same function
  - Pros: DRY, single source of truth, ensures identical registration
  - Cons: Requires refactoring `main.cpp` to extract registration code

**Q3.2: Should we register rendering systems (they'll be no-ops)?**
- a) Yes, register all systems for consistency (recommended)
- b) No, only register update systems ✅ **USER ANSWER: b**
- c) Conditionally register based on flag
- **Decision**: Skip render systems

**Q3.3: How should the server run the battle simulation loop?**
- a) Fixed timestep (1/60s) like headless tests (recommended) ✅ **USER ANSWER: a**
- b) Variable timestep based on wall clock
- c) As fast as possible (no delays)
- **Decision**: Fixed 1/60s timestep for determinism

**Q3.4: How do we detect when a battle is complete?**
- a) Check `BattleResult` component state
- b) Check `CombatQueue.complete` flag
- c) Check all dishes are in `Finished` phase
- d) Combination of above ✅ **USER ANSWER: d**
- **Decision**: Use `CombatQueue.complete` as primary signal, but also add verification check that one team has no active dishes remaining. We can remove the verification later if `cq.complete` proves reliable.

## 4. Team Loading & Data Format

**Q4.1: What format should the client send team data in?**
- a) Same JSON format as existing `output/battles/*.json` files (recommended) ✅ **USER ANSWER: a**
- b) Simplified format with just dish types and slots
- c) Full entity state including components
- **Decision**: Use existing JSON format

**Q4.2: How should we handle opponent team selection?**
- a) Server picks random opponent from predefined pool (recommended) ✅ **USER ANSWER: a**
- b) Client specifies opponent team in request
- c) Server generates opponent based on player team strength
- d) Fixed opponent team for testing
- **Decision**: Random from pool, with (d) as fallback for testing

**Q4.3: Where should opponent team JSON files be stored?**
- a) `resources/battles/opponents/*.json` (recommended) ✅ **USER ANSWER: a**
- b) `output/battles/opponents/*.json`
- c) Embedded in server code
- d) Database or external service
- **Decision**: `resources/battles/opponents/`, BUT track file count on startup to prevent millions of files filling disk
- **Implementation**: Scan and count opponent files on startup; log warning if count > threshold (e.g., 1000); consider cleanup/archival strategy

**Q4.4: Should we support dish levels in team JSON?**
- a) Yes, read `level` field from JSON (recommended) ✅ **USER ANSWER: a**
- b) No, always level 1
- c) Calculate level from team composition
- **Decision**: Yes - team JSON must include levels AND powerups for each dish
- **Powerup Format**: ✅ **USER ANSWER: Option D** - Defer for now, add empty array `[]` placeholder
  - Add `"powerups": []` field to JSON structure for future use
  - Define format later when powerup system is implemented
  - Server should accept and ignore powerups for now (for forward compatibility)

**Q4.5: How should we handle invalid team data?**
- a) Return HTTP 400 with error message (recommended) ✅ **USER ANSWER: a**
- b) Use fallback team
- c) Return HTTP 500 (server error)
- **Decision**: HTTP 400 with descriptive error

## 5. Seed Generation & Determinism

**Q5.1: How should the server generate battle seeds?**
- a) Use `std::random_device` to generate unique seed (recommended) ✅ **USER ANSWER: a**
- b) Use timestamp + random component
- c) Sequential seeds (1, 2, 3...)
- d) Hash of team data + timestamp
- **Decision**: Use `std::random_device` for unique seeds

**Q5.2: Should the seed be included in the response?**
- a) Yes, always include seed (required for client replay) (recommended) ✅ **USER ANSWER: a**
- b) Optional, only if requested
- c) Never include (security concern?)
- **Decision**: Always include seed - it MUST be in the response

**Q5.3: Should we validate that client simulation matches server?**
- a) Yes, server stores results and client can verify (recommended)
- b) No, trust client simulation
- c) Server provides checksum/hash of final state ✅ **USER ANSWER: c**
- **Decision**: Server provides checksum/hash of final state - allows flagging hacked clients

**Q5.4: How should we ensure all RNG uses SeededRng?**
- a) Audit all battle systems to use SeededRng (recommended) ✅ **USER ANSWER: a**
- b) Replace `std::random` with SeededRng wrapper
- c) Runtime check that no other RNG is used
- **Decision**: Audit systems - but add TODO to handle bad clients sending custom teams (security hardening needed)

## 6. Battle Result Serialization

**Q6.1: What level of detail should be in the response JSON?**
- a) Full event log with all triggers and effects (recommended for debugging) ✅ **USER ANSWER: a**
- b) Minimal: just seed, outcomes, and final scores
- c) Configurable detail level
- **Decision**: Full event log, BUT setup structure with single flag to turn off debug state for production (smaller response size)

**Q6.2: Should we include intermediate state snapshots?**
- a) Yes, include state after each course (recommended) ✅ **USER ANSWER: a**
- b) No, only final state
- c) Optional, based on request parameter
- **Decision**: Yes, but only for debug mode (so we can easily test and verify). Production mode excludes snapshots.

**Q6.3: How should we serialize entity IDs?**
- a) Include original entity IDs from simulation (recommended)
- b) Normalize to sequential IDs (0, 1, 2...)
- c) Use entity type + index instead
- **Decision**: ⚠️ **TODO FOR NOW** - Don't use entity IDs as they won't match across executables
- **User Answer**: teamId + dish order might not be enough for dishes that generate new dishes (e.g., dishes that spawn other dishes). Need incrementing ID for player side. 
- **Action**: Add TODO - design ID system that works across server/client and handles dish generation. For now, use team-relative IDs (teamId + dish order + generation counter).

**Q6.4: Should we include animation timing data?**
- a) Yes, include all animation events (recommended)
- b) No, only combat events ✅ **USER ANSWER: b**
- c) Optional
- **Decision**: No, only things needed for the simulation (combat events only)

## 7. Error Handling & Logging

**Q7.1: What logging framework should the server use?**
- a) Existing `log_info()`, `log_error()` from game (recommended) ✅ **USER ANSWER: a**
- b) Separate server logging system
- c) Standard library `std::cout` / `std::cerr`
- d) Structured logging (JSON logs)
- **Decision**: Use existing logging system

**Q7.2: What log level should the server use?**
- a) INFO for normal operation (recommended) ✅ **USER ANSWER: a**
- b) WARN (minimal logging)
- c) DEBUG (verbose)
- d) Configurable via command-line flag
- **Decision**: INFO for now (can add configurable flag later)

**Q7.3: How should we handle simulation errors (e.g., division by zero)?**
- a) Return HTTP 500 with error details (recommended) ✅ **USER ANSWER: a**
- b) Log error and return partial results
- c) Use fallback simulation
- **Decision**: HTTP 500 with error details

**Q7.4: Should we validate team size limits?**
- a) Yes, enforce max 7 dishes per team (recommended) ✅ **USER ANSWER: a**
- b) No, accept any size
- c) Configurable limit
- **Decision**: Yes, validate as much as possible and disqualify teams that aren't possible or are invalid

## 8. Testing & Validation

**Q8.1: How should we structure server tests?**
- a) Separate test suite in `src/server/tests/` (recommended) ✅ **USER ANSWER: a**
- b) Integrate into existing test system
- c) Manual testing only
- **Decision**: Separate test suite in `src/server/tests/`

**Q8.2: Should server tests require running server process?**
- a) Yes, integration tests that start server (recommended)
- b) No, unit tests only
- c) Both unit and integration tests ✅ **USER ANSWER: c**
- **Decision**: Both - unit tests should only need the platform they're working on. Integration tests should have `--use-server` flag that spins up a server instance to test against.

**Q8.3: How should we compare server vs client results?**
- a) Export JSON from both, compare files (recommended)
- b) In-memory comparison during test
- c) Use checksum/hash comparison ✅ **USER ANSWER: c**
- **Decision**: Use checksum/hash comparison (easier, client doesn't need to send back to server until after they click "next round" next time)

**Q8.4: What tolerance should we use for floating point comparisons?**
- a) Exact match (no tolerance) (recommended for determinism)
- b) Small epsilon (1e-6) ✅ **USER ANSWER: b**
- c) Configurable tolerance
- **Decision**: Small epsilon, but we should avoid floats if possible

**Q8.5: Should we test with existing test teams?**
- a) Yes, reuse `output/battles/*.json` test files (recommended)
- b) Generate test teams programmatically
- c) Both ✅ **USER ANSWER: c**
- **Decision**: Both - use specific teams to make it easier to validate test results, but also test with real teams eventually to make sure nothing crazy happens

## 9. Build System & Dependencies

**Q9.1: Should the server be a separate make target?**
- a) Yes, `target("battle_server")` (recommended) ✅ **USER ANSWER: a**
- b) No, build as part of main game
- c) Conditional build flag
- **Decision**: Yes, separate target

**Q9.2: Should server share source files with main game?**
- a) Yes, compile same `.cpp` files (recommended) ✅ **USER ANSWER: a**
- b) No, copy/symlink files
- c) Create shared library
- **Decision**: Yes, as much similarity as possible

**Q9.3: How should we handle Raylib dependency for server?**
- a) Exclude Raylib entirely from server build (recommended) ✅ **USER ANSWER: a**
- b) Link but don't initialize
- c) Use stub implementations
- **Decision**: Server will not need Raylib or any visuals

**Q9.4: Should we include test files in server build?**
- a) Yes, include `src/server/tests/*.cpp` (recommended)
- b) No, separate test executable
- c) Conditional compilation
- **Decision**: ⚠️ **TODO FOR NOW** - Decide during implementation. Options: separate test executable (`battle_server_test.exe`) or conditional compilation.

## 10. Configuration & Deployment

**Q10.1: How should server configuration be managed?**
- a) Command-line arguments only (recommended for MVP)
- b) Config file (JSON/YAML) ✅ **USER ANSWER: b**
- c) Environment variables
- d) Combination of above
- **Decision**: Config file is best

**Q10.2: Should the server support multiple concurrent battles?**
- a) No, one battle at a time (recommended for MVP) ✅ **USER ANSWER: a**
- b) Yes, queue requests
- c) Yes, parallel processing
- **Decision**: One at a time. We can always spin up more of the same executable and use a message queue to round-robin it

**Q10.3: Should we persist battle results to disk?**
- a) Yes, save to `output/battles/results/*.json` (recommended) ✅ **USER ANSWER: a**
- b) No, only return in response
- c) Optional, based on flag
- **Decision**: Yes, for now, but audit them so we don't have too many. Only keep the last 10 or something (implement cleanup/rotation)

**Q10.4: Should the server support CORS headers?**
- a) Yes, allow all origins (recommended for development) ✅ **USER ANSWER: a**
- b) No CORS (server-side only)
- c) Configurable allowed origins
- **Decision**: Yes, allow all origins for development

## 11. Animation & Timing

**Q11.1: How should we handle animation timing in server?**
- a) Skip animations, set all animation values to final state immediately (recommended) ✅ **USER ANSWER: a**
- b) Simulate animation timing
- c) Include animation events in response
- **Decision**: No animation, skip entirely

**Q11.2: Should we include slide-in animation timing?**
- a) No, set `enter_progress = 1.0` immediately (recommended) ✅ **USER ANSWER: a**
- b) Yes, simulate slide-in duration
- c) Include in response but don't wait
- **Decision**: No animation, set `enter_progress = 1.0` immediately

**Q11.3: How should we handle combat tick cadence timing?**
- a) Use exact same timing as client (150ms ticks) (recommended)
- b) Faster (no delays) ✅ **USER ANSWER: b**
- c) Configurable
- **Decision**: Faster, no delays. Should still be deterministic but can run faster as we don't need the user to see it

## 12. Entity Creation & Management

**Q12.1: Should server create entities the same way as client?**
- a) Yes, use `BattleTeamLoaderSystem` logic (recommended) ✅ **USER ANSWER: a**
- b) Simplified entity creation
- c) Create entities directly without systems
- **Decision**: Same exact logic - use `BattleTeamLoaderSystem` logic

**Q12.2: Do we need Transform components for server entities?**
- a) Yes, some systems may check Transform (recommended)
- b) No, server doesn't need positions ✅ **USER ANSWER: b**
- c) Create minimal Transform with dummy values
- **Decision**: No positions don't matter - skip Transform components entirely

**Q12.3: Should we create sprite/texture components?**
- a) No, skip all rendering components (recommended) ✅ **USER ANSWER: a**
- b) Yes, for compatibility
- c) Create stubs
- **Decision**: Skip all visual things on server - we want the server to be as performant as possible

## 13. Integration Points

**Q13.1: How should the client send battle requests?**
- a) HTTP POST with JSON body (recommended) ✅ **USER ANSWER: a and d**
- b) HTTP GET with query parameters
- c) WebSocket connection
- d) Direct function call (for testing) ✅ **USER ANSWER: a and d**
- **Decision**: HTTP POST with JSON body (production), AND direct function call for testing

**Q13.2: Should we provide a client library for sending requests?**
- a) No, client uses standard HTTP library (recommended for MVP) ✅ **USER ANSWER: a**
- b) Yes, C++ client library
- c) Yes, but future work
- **Decision**: No client library - client uses standard HTTP library (a is good)

**Q13.3: How should client handle server response?**
- a) Parse JSON, extract seed, initialize battle with seed (recommended) ✅ **USER ANSWER: a**
- b) Use response as replay data
- c) Verify results match before replaying
- **Decision**: Parse JSON, extract seed, initialize battle with seed

## 14. Implementation Plan Clarifications

These questions emerged from reviewing the detailed implementation plan and need clarification before implementation:

**Q14.1: How should temporary JSON files be cleaned up?**
- a) Keep last 10 temp files, delete any others (recommended) ✅ **USER ANSWER: a**
- b) Delete immediately after `BattleTeamLoaderSystem` processes the request
- c) Delete after battle completes
- d) Keep until server shutdown
- e) Use a cleanup thread that runs periodically
- **Decision**: Keep last 10 temp files, delete any others (same rotation strategy as result files)
- **Risk Analysis**: 
  - ⚠️ **Risk**: If files accumulate, disk space can fill up quickly with high request volume
  - ✅ **Solution**: Use same rotation helper as result files (Q14.10) - keep last 10 temp files, delete oldest when limit reached
  - 💡 **Implementation**: After creating temp file, call rotation helper for `output/battles/temp_*` directory. Clean up after each battle.

**Q14.2: How should entities be cleaned up between battles in the server?**
- a) Let the engine handle it (cleanup systems manage it) (recommended) ✅ **USER ANSWER: a**
- b) Reuse same entities, just reset their state (performance, but risky)
- c) Delete all entities and recreate fresh (cleaner, but more overhead)
- d) Manually mark entities and call `EntityHelper::cleanup()`
- **Decision**: Let the engine handle cleanup via cleanup systems (CleanupBattleEntities, etc.). Must be aware of side effects that could cause non-deterministic differences.
- **Risk Analysis**:
  - ⚠️ **Critical Risk**: Must be aware of side effects that could cause non-deterministic differences between battles
  - ✅ **Solution**: Let cleanup systems handle it, but add validation checks on battle load to ensure state is reset properly (see Q14.3)
  - 💡 **Implementation**: 
    - After battle completes, run systems normally - `CleanupBattleEntities` will handle cleanup when appropriate
    - However, we need to ensure cleanup systems work without screen transitions
    - May need to manually set `GameStateManager` screen to `Results` temporarily to trigger cleanup, or adjust cleanup system logic
    - Add validation in `start_battle()` to check for leftover entities (Q14.3)
  - ⚠️ **Critical Issue**: `CleanupBattleEntities` checks `last_screen == Results` (line 21) - server doesn't have screen transitions. 
  - 💡 **Potential Solutions**:
    1. Manually set `GameStateManager` screen to `Results` temporarily after battle completes to trigger cleanup
    2. Modify `CleanupBattleEntities` to also check `is_headless_mode` and cleanup when battle complete (if `CombatQueue.complete == true`)
    3. Create server-specific cleanup trigger that marks entities for cleanup after battle completes
  - ⚠️ **Recommendation**: Solution 2 - add headless mode check to `CleanupBattleEntities` so it triggers cleanup when battle completes, not just on screen transition

**Q14.3: Should the server reuse the same SystemManager instance between battles?**
- a) Yes, reuse single SystemManager instance (recommended for performance)
- b) No, create new SystemManager per battle (cleaner isolation, safer) ✅ **USER ANSWER: b is safer, but prefer reuse**
- c) Pool SystemManager instances
- **Decision**: Reuse SystemManager (preferred for performance), but add validation checks on battle load to ensure things seem reset
- **Risk Analysis**:
  - ⚠️ **User Concern**: Option b (new SystemManager per battle) is safer but less performant. User prefers reuse but wants safety checks.
  - ✅ **Solution**: Reuse `SystemManager` but add validation checks in `BattleSimulator::start_battle()` to verify:
    - No leftover entities from previous battle (query for `IsPlayerTeamItem`, `IsOpponentTeamItem` - should be empty)
    - Singleton entities are in expected state (CombatQueue reset, TriggerQueue empty, etc.)
    - GameStateManager is in Battle screen
  - 💡 **Implementation**: 
    - Reuse `SystemManager` from `ServerContext`
    - In `start_battle()`, add validation function that checks state is clean
    - Log warnings if validation fails, but continue (for debugging)
    - Consider adding a `--strict-validation` flag that fails on validation errors

**Q14.4: How should GameStateManager be handled in server mode?**
- a) Create minimal GameStateManager singleton with Screen::Battle always active (recommended) ✅ **USER ANSWER: a**
- b) Skip GameStateManager entirely, modify systems to not check it
- c) Create stub GameStateManager that always returns Battle screen
- **Decision Needed**: `BattleTeamLoaderSystem` checks `GameStateManager::Screen::Battle` in `should_run()`. Server doesn't have screen transitions. Should we initialize a minimal GameStateManager or modify systems?
- **Risk Analysis**:
  - ⚠️ **Critical Risk**: Many systems check `GameStateManager::Screen::Battle`:
    - `BattleTeamLoaderSystem` (line 39)
    - `BattleDebugSystem` (checks `active_screen == Battle`)
    - `BattleProcessorSystem` (line 12)
    - `CleanupBattleEntities` (checks screen transitions)
  - ✅ **Solution**: Create `GameStateManager` singleton and set `active_screen = Screen::Battle` and `current_state = GameState::Playing` at server initialization.
  - ⚠️ **Warning**: Don't modify systems - they're shared between client and server. Changing them would break client.
  - 💡 **Implementation**: In `ServerContext::initialize()`, create `GameStateManager` singleton and set appropriate state.

**Q14.5: Should SeededRng be reset between battles?**
- a) Yes, call `set_seed()` for each new battle (recommended) ✅ **USER ANSWER: a (definitely)**
- b) No, continue with same seed sequence
- c) Create new SeededRng instance per battle
- **Decision Needed**: The plan sets seed per battle, but should we reset the RNG state entirely?
- **Risk Analysis**:
  - ✅ **Verified**: `SeededRng` has `set_seed(uint64_t)` method (line 27-30) that properly reseeds the underlying `std::mt19937_64` generator.
  - ⚠️ **Critical**: Must use `set_seed()`, not just set `seed` field directly. `set_seed()` calls `gen.seed(seed)` which resets the RNG state.
  - ✅ **Solution**: Call `SeededRng::get().set_seed(battle_seed)` in `BattleSimulator::start_battle()` to completely reset RNG state for each battle.
  - 💡 **Implementation**: In `BattleSimulator::start_battle()`, call `SeededRng::get().set_seed(battle_seed)` instead of just `SeededRng::get().seed = battle_seed`.

**Q14.6: How should BattleLoadRequest lifecycle be managed?**
- a) Reuse same singleton, update paths and reset `loaded` flag (recommended) ✅ **USER ANSWER: a (okay)**
- b) Create new BattleLoadRequest per battle, delete after use
- c) Keep BattleLoadRequest for entire server lifetime
- **Decision Needed**: The plan creates a new `BattleLoadRequest` per battle. Should we clean it up after the battle completes, or reuse it?
- **Risk Analysis**:
  - ✅ **Note**: `CleanupBattleEntities` explicitly keeps `BattleLoadRequest` singleton for next battle (line 43-44 comment).
  - ✅ **Solution**: Reuse singleton, just update `playerJsonPath`, `opponentJsonPath`, and set `loaded = false` before starting new battle.
  - 💡 **Implementation**: In `BattleSimulator::start_battle()`, get existing singleton or create new one, then update paths and reset `loaded` flag.

**Q14.7: Should InitCombatState skip creating SlideIn animation events in headless mode?**
- a) Yes, skip animation event creation entirely (recommended) ✅ **USER ANSWER: a (yes, there should never be any animation on the server)**
- b) Create animation events but systems will no-op them
- c) Set animation values immediately (enter_progress = 1.0)
- **Decision Needed**: `InitCombatState` creates `SlideIn` animation events (line 59). In headless mode, should we skip this entirely or let animation systems handle it as no-ops?
- **Risk Analysis**:
  - ⚠️ **Risk**: Creating animation events wastes memory and CPU cycles even if they're no-ops
  - ✅ **Solution**: Modify `InitCombatState` to check `render_backend::is_headless_mode` before creating animation events. Set `enter_progress = 1.0` immediately instead.
  - ⚠️ **Warning**: This requires modifying `InitCombatState`, which is shared. Must ensure it doesn't break client.
  - 💡 **Implementation**: Add `if (!render_backend::is_headless_mode)` check before `make_animation_event()` call. In headless mode, set all dish `enter_progress = 1.0` directly.

**Q14.8: How should BattleTeamLoaderSystem handle level field from JSON?**
- a) Modify BattleTeamLoaderSystem to parse "level" field from JSON (recommended) ✅ **USER ANSWER: a (yes, it should read the level from the team on each dish)**
- b) Create server-specific version that handles levels
- c) Use separate system to set levels after loading
- **Decision Needed**: Current `BattleTeamLoaderSystem` hardcodes `DishLevel(1)` (line 190). The plan requires levels from JSON. Should we modify the existing system or create server-specific logic?
- **Risk Analysis**:
  - ✅ **Solution**: Modify existing system to parse optional `"level"` field from JSON. Default to 1 if missing (backward compatible).
  - ⚠️ **Risk**: If client sends teams without levels, must default gracefully.
  - 💡 **Implementation**: In `create_battle_dish_entity()`, check `dishEntry.contains("level")` and parse it. Default to `DishLevel(1)` if missing.
  - ✅ **Powerups**: Also parse `"powerups"` array (empty for now) for future compatibility, but ignore it.

**Q14.9: Where should result files be saved and what naming convention?**
- a) `output/battles/results/YYYYMMDD_HHMMSS_<seed>.json` (recommended) ✅ **USER ANSWER: a seems fine, OR timestamp_battle_id and then in the file include the two team usernames or team unique ids**
- b) `output/battles/results/<opponent_id>_<seed>.json`
- c) `output/battles/results/result_<timestamp>.json`
- **Decision**: Use `YYYYMMDD_HHMMSS_<battle_id>.json` format, where `battle_id` is a unique identifier. Inside the JSON file, include both team usernames/unique IDs.
- **Risk Analysis**:
  - ✅ **Benefits**: Timestamp + battle_id provides traceability and prevents collisions
  - ✅ **User Preference**: Include team identifiers in file contents rather than filename (cleaner filenames, more flexible)
  - 💡 **Implementation**: 
    - Generate unique battle_id (could be seed, or UUID, or incrementing counter)
    - Filename: `YYYYMMDD_HHMMSS_<battle_id>.json`
    - File contents: Include `"playerTeamId"`, `"opponentTeamId"`, `"playerUsername"`, `"opponentUsername"` fields in result JSON

**Q14.10: How should result file rotation/cleanup work?**
- a) Keep last 10 files, delete oldest when limit reached (per-battle cleanup) (recommended) ✅ **USER ANSWER: a (we can make a common helper that does this for a given folder)**
- b) Keep all files, rely on external cleanup
- c) Keep files for last 24 hours
- d) Configurable limit via config file
- **Decision**: Create common helper function for file rotation that can be reused for result files and temp files
- **Risk Analysis**:
  - ✅ **Solution**: Create `rotate_files_in_directory(const std::string &dir, size_t keep_count)` helper function
  - 💡 **Implementation**: 
    - Helper function: `server::FileRotationHelper::rotate_files(dir, keep_count)`
    - Scans directory, sorts files by `last_write_time()`, keeps newest N, deletes rest
    - Call after saving result file and after creating temp files
    - Reusable for both `output/battles/results/` and `output/battles/temp_*`

**Q14.11: What should happen if a system throws an exception during simulation?**
- a) Catch exception, return HTTP 500 with error details (recommended) ✅ **USER ANSWER: a (yes, make it a flag though - we don't want users to know what the detailed error is, but it'll be helpful for debugging so we should definitely log it somewhere)**
- b) Let exception propagate and crash server
- c) Log error and return partial results
- d) Use try-catch around entire simulation loop (most robust)
- **Decision**: Wrap simulation loop in try-catch. Use error detail level flag:
  - **Production mode**: Return HTTP 500 with generic error message to user, log full details internally
  - **Debug mode**: Return HTTP 500 with detailed error message (for debugging)
- **Risk Analysis**:
  - ⚠️ **Security**: Don't expose internal error details to users (could reveal implementation details)
  - ✅ **Solution**: Add `--error-detail-level` flag (or config option):
    - `minimal`: Generic "Internal server error" to user, full details in logs
    - `detailed`: Full error details in HTTP response (for debugging)
  - 💡 **Implementation**: 
    - Wrap entire simulation loop in try-catch
    - Log exception details to log file with full stack trace
    - Return sanitized error message based on detail level flag
    - Ensure cleanup happens even on exception (temp files, entity cleanup)

**Q14.12: How should debug mode be set and propagated?**
- a) Per-request debug flag in BattleLoadRequest
- b) Global debug flag on startup (recommended) ✅ **USER ANSWER: b**
- c) Both: Global flag + per-request override
- **Decision**: Global debug flag on startup (via `--debug` flag or config file)
- **Pros/Cons Analysis**:
  
  **Option A: Per-request flag in BattleLoadRequest (JSON)**
  - ✅ **Pros**: 
    - Flexible - can enable debug for specific requests
    - Good for production servers that want to debug specific battles
    - Client can request debug data when needed
  - ❌ **Cons**:
    - Requires modifying request JSON format
    - Every request needs to specify flag
    - Harder to enable debug for all requests (must modify client)
  
  **Option B: Global debug flag on startup (--debug flag or config)** ✅ **SELECTED**
  - ✅ **Pros**:
    - Simple - one flag sets behavior for all requests
    - Good for development/testing
    - No need to modify request format
    - Easy to enable/disable for entire server session
  - ❌ **Cons**:
    - All-or-nothing - can't selectively enable debug for specific battles
    - Less flexible for production debugging
  
  **Option C: Both (global default + per-request override)**
  - ✅ **Pros**:
    - Best of both worlds
    - Global flag for development, per-request for production debugging
    - Most flexible
  - ❌ **Cons**:
    - More complex implementation
    - Two places to check flag
  
  **Implementation**: 
    - Add `--debug` command-line flag or `"debug": true` in config file
    - Store in global variable or server config
    - Systems check global flag when deciding to include debug data (snapshots, verbose events)
    - Serialization systems check global flag to include/exclude snapshots in response

**Q14.13: Should InitialShopFill and other shop systems be registered in server?**
- a) No, skip shop systems entirely (recommended) ✅ **USER ANSWER: a (skip shop)**
- b) Yes, register it but it will be no-op
- c) Only register if shop state is needed
- **Decision Needed**: The plan includes `InitialShopFill` in system registration. Does server need shop systems? Should we exclude shop-related systems?
- **Risk Analysis**:
  - ✅ **Solution**: Exclude shop systems from server. Server only needs battle simulation, not shop logic.
  - ⚠️ **Systems to Exclude**: 
    - `InitialShopFill` (line 206 in main.cpp)
    - `register_shop_update_systems()` systems (GenerateShopSlots, GenerateInventorySlots, ScreenTransitionSystem)
  - 💡 **Implementation**: In `battle_systems::register_battle_systems()`, don't include shop systems. Only register battle-related systems.
  - ⚠️ **Note**: `ProcessBattleRewards` is included - verify if it needs shop state. If yes, may need to skip or stub it.

**Q14.14: How should we verify system registration order matches main game?**
- a) Add compile-time assertions or unit tests (recommended) ✅ **USER ANSWER: a (yes)**
- b) Manual verification during code review
- c) Runtime validation that checksums match (also useful)
- d) Combination: Unit tests + runtime validation (most robust)
- **Decision Needed**: Critical that system order matches. How do we ensure this stays in sync?
- **Risk Analysis**:
  - ⚠️ **Critical Risk**: System order affects determinism. Different order = different results, breaking server-client matching.
  - ✅ **Solution**: Combination approach:
    - Unit test that compares system registration order between `register_battle_systems()` and main.cpp
    - Runtime test that runs same battle with same seed on both, compares checksums
  - 💡 **Implementation**: 
    - Create test that extracts system names from both registration functions and compares order
    - Add to CI/CD to catch regressions
    - Consider adding system name/ID to registration to make comparison easier

**Q14.15: What should be the HTTP timeout for battle requests?**
- a) 30 seconds (recommended for most battles)
- b) 60 seconds
- c) No timeout (let it run until complete)
- d) Configurable via config file ✅ **USER ANSWER: d (and when it happens, let's store that battle and the two teams in a debug place so we can see if it's on purpose or if it was a bug)**
- **Decision**: Configurable timeout via config file. When timeout occurs, store battle state and both teams in debug location for analysis.
- **Risk Analysis**:
  - ⚠️ **Risk**: Long-running battles could indicate bugs (infinite loops, stuck states) or legitimate long battles
  - ✅ **Solution**: 
    - Configurable timeout (default 30s, configurable via config file)
    - When timeout occurs, save battle state snapshot to `output/battles/debug/timeout_<timestamp>_<battle_id>.json`
    - Include: both team JSONs, current battle state, seed, simulation time, entity counts
    - This allows analysis to determine if timeout was bug or legitimate long battle
  - 💡 **Implementation**: 
    - Add timeout check in simulation loop (check elapsed time)
    - On timeout, serialize battle state before returning HTTP 408 (Request Timeout)
    - Save to debug directory for later analysis

**Q14.16: Should opponent files directory be created if missing?**
- a) No, return error if directory doesn't exist (recommended)
- b) Yes, create directory structure automatically ✅ **USER ANSWER: b (we should make sure it's there on startup)**
- c) Use fallback opponent if directory missing
- **Decision**: Create directory structure automatically on server startup if missing
- **Risk Analysis**:
  - ✅ **Solution**: On server startup, check if `resources/battles/opponents/` exists, create it if missing
  - ⚠️ **Note**: Should also create parent directories (`resources/battles/` if needed)
  - 💡 **Implementation**: In `TeamManager::track_opponent_file_count()` or server startup, use `std::filesystem::create_directories()` to ensure directory exists

**Q14.17: How should absolute vs relative paths be handled for temp files?**
- a) Use absolute paths based on server working directory
- b) Always use relative paths
- c) Configurable base path (most flexible) ✅ **USER ANSWER: c (but in the config file)**
- **Decision**: Configurable base path set in config file (not command-line)
- **Clarification of Options**:
  
  **Option A: Absolute paths based on server working directory**
  - Example: Server runs from `/home/user/my_name_chef/`, temp files at `/home/user/my_name_chef/output/battles/temp_*.json`
  - ✅ **Pros**: Works regardless of where server is run from
  - ❌ **Cons**: Paths are hardcoded to working directory, can't change without restarting server from different location
  
  **Option B: Always use relative paths**
  - Example: Temp files always at `output/battles/temp_*.json` relative to current working directory
  - ✅ **Pros**: Simple, portable
  - ❌ **Cons**: Breaks if server run from different directory (e.g., `cd /tmp && ./my_name_chef/battle_server.exe`)
  
  **Option C: Configurable base path** ✅ **SELECTED**
  - Example: Config file sets `"base_path": "/var/battles"` or `"base_path": "output"`, temp files at `{base_path}/battles/temp_*.json`
  - ✅ **Pros**: Most flexible, can set base path independently of working directory
  - ✅ **Use case**: Server can be run from anywhere, base path points to project directory
  - ✅ **User Preference**: Set in config file (not command-line)
  - ❌ **Cons**: Requires configuration, more complex
  
  **Implementation**: 
    - Add `"base_path"` field to config file (JSON/YAML)
    - Default to current working directory if not specified
    - Use `std::filesystem::path(base_path) / "output" / "battles" / "temp_*.json"` for temp files
    - Use same base path for result files: `{base_path}/output/battles/results/`
    - Use same base path for opponent files: `{base_path}/resources/battles/opponents/`

**Q14.18: Should server validate that BattleFingerprint::compute() exists before using it?**
- a) Yes, add compile-time check (recommended) ✅ **USER ANSWER: a (yes)**
- b) No, assume it exists (it does)
- c) Provide fallback hash function if missing
- **Decision Needed**: Plan references `BattleFingerprint::compute()` - should we verify it's available or add fallback?

**Q14.19: How should Preload initialization be called in server?**
- a) Call `Preload::get().init("battle_server", true).make_singleton()` in ServerContext::initialize()
- b) Call in server main before ServerContext
- c) Skip Preload entirely if possible ✅ **USER ANSWER: c (we should skip preload if we can and don't need anything in there)**
- **Decision**: Skip Preload if possible. Only initialize if systems require its singletons.
- **Risk Analysis**:
  - ⚠️ **What Preload does** (from `preload.cpp::make_singleton()`):
    - Creates UI singletons (`ui::AutoLayoutRoot`, `ui::UIComponent`, `ui::FontManager`) - **NOT NEEDED** for server
    - Creates `SoundEmitter` singleton - **NOT NEEDED** for server (no audio)
    - Creates `HasCamera` singleton - **NOT NEEDED** for server (no rendering)
    - Sets up texture manager (in headless mode, just empty texture) - **MAY BE NEEDED**
    - Sets up input, window manager singletons - **NOT NEEDED** for server
  - ⚠️ **Texture Manager Usage Analysis**:
    - `BattleTeamLoaderSystem` creates `HasSprite` components (line 206)
    - However, we skip all rendering components on server (Q12.3) - no Transform, no sprite components
    - Render systems that use texture_manager are skipped (RenderBattleTeams, RenderSpritesByOrder, etc.)
    - `GenerateDishesGallery` uses texture_manager - but this is likely for gallery rendering, not battle simulation
  - ✅ **Solution**: 
    - **Skip Preload entirely** - we don't need sprite components on server
    - Modify `BattleTeamLoaderSystem` to skip sprite component creation in headless mode (already skipping Transform per Q12.2)
    - If any system requires texture_manager singleton, initialize just that part: `texture_manager::add_singleton_components(sophie, {})`
  - 💡 **Implementation**: 
    - Skip Preload initialization
    - In `BattleTeamLoaderSystem::create_battle_dish_entity()`, check `render_backend::is_headless_mode` and skip `HasSprite` component creation
    - If compilation fails due to missing texture_manager singleton, initialize just that: `texture_manager::add_singleton_components(sophie, {})`

**Q14.20: Should server handle CORS in httplib setup or in each endpoint?**
- a) Set default headers in `setup_routes()` (recommended) ✅ **USER ANSWER: a (sure)**
- b) Set headers in each endpoint handler
- c) Use middleware pattern
- **Decision Needed**: Plan shows CORS in `setup_routes()` but doesn't specify exact location. Should it be in `BattleAPI::setup_routes()`?

## Summary of Decided Answers

**Decided**:
- HTTP Library: `cpp-httplib` (vendored)
- Port: 8080 (configurable via `--port`)
- Threading: Single-threaded
- Raylib: Not initialized, use stubs
- System Registration: All battle systems in same order, automatic discovery needed
- Render Systems: Skip render systems
- Timestep: Fixed 1/60s (but faster execution, no delays)
- Team Format: Existing JSON format with levels and powerups
- Opponent Selection: Random from pool, track file count
- Seed Generation: `std::random_device`, always in response
- Validation: Checksum/hash of final state (client validates, doesn't send back until "next round")
- Error Handling: HTTP 400 for invalid teams, HTTP 500 for server errors
- Initialization: `ServerContext::initialize()` function
- Result Serialization: Full event log with debug flag, snapshots only in debug mode, no animations
- Logging: Existing `log_info()` system, INFO level
- Testing: Separate test suite, unit + integration (with `--use-server` flag), checksum comparison
- Build: Separate make target, share source files
- Configuration: Config file (JSON/YAML)
- Concurrency: One battle at a time (scale with multiple instances + message queue)
- Results Persistence: Save results, but audit and keep only last 10
- Animation: Skip all animations entirely

**✅ ALL QUESTIONS ANSWERED - READY FOR IMPLEMENTATION!**

All questions from Sections 1-14 have been answered and documented. The implementation plan has been updated with all decisions. Ready to proceed with coding.

**Decided**:
- ✅ Q3.4: Battle completion - Use `CombatQueue.complete` + verification check (can remove verification later if reliable)
- ✅ Q3.1: System registration - Extract shared `register_battle_systems()` function (Option D)
- ✅ Q4.4: Powerup format - Add empty array `[]` placeholder for now, define format later
- ⚠️ Q6.3: Entity ID serialization - TODO for now (design later when dish generation is needed)
- ⚠️ Q9.4: Test files in build - TODO for now (decide during implementation)

**Fully Answered Sections**:
- ✅ Section 1: HTTP Server Library & Technology (all answered)
- ✅ Section 2: Server Architecture & Initialization (all answered)
- ✅ Section 7: Error Handling & Logging (all answered)
- ✅ Section 8: Testing & Validation (all answered)
- ✅ Section 10: Configuration & Deployment (all answered)
- ✅ Section 11: Animation & Timing (all answered)
- ✅ Section 12: Entity Creation & Management (all answered)
- ✅ Section 13: Integration Points (all answered)

**Still Need Answers** (Mostly from Section 14 - Implementation Plan Review):
1. ✅ **Q3.4**: Battle completion detection - DECIDED
2. ✅ **Q4.4**: Powerup format - DECIDED (empty array placeholder)
3. ✅ **Q3.1**: System registration - DECIDED (shared function)
4. ✅ **Q6.1-6.4**: Result serialization - DECIDED
5. ✅ **Q7.1-7.4**: Logging and error handling - DECIDED
6. ✅ **Q8.1-8.5**: Testing approach - DECIDED
7. ✅ **Q9.1-9.4**: Build system - DECIDED
8. ✅ **Q10.1-10.4**: Configuration - DECIDED
9. ✅ **Q11.1-11.3**: Animation/timing - DECIDED
10. ✅ **Q12.1-12.3**: Entity creation - DECIDED
11. ✅ **Q13.1-13.3**: Integration - DECIDED

**All Section 14 Questions Answered** ✅:
- Q14.1-Q14.20: All answered - see Section 14 for detailed decisions and implementation notes

## Critical Decisions Needed

These questions require explicit answers before implementation:

1. ✅ **HTTP Library choice** (Q1.1) - DECIDED: cpp-httplib
2. ✅ **Raylib handling** (Q2.1, Q9.3) - DECIDED: No Raylib, use stubs
3. ✅ **System registration approach** (Q3.1) - DECIDED: Extract shared `register_battle_systems()` function
4. ✅ **Team data format** (Q4.1) - DECIDED: Existing JSON format
5. ✅ **Result serialization detail** (Q6.1) - DECIDED: Full event log with debug flag
6. ✅ **Battle completion detection** (Q3.4) - DECIDED: CombatQueue.complete + verification check

**All Section 14 Questions Answered** ✅:
7. ✅ **Entity cleanup between battles** (Q14.2, Q14.3) - Let engine handle, add validation checks
8. ✅ **GameStateManager handling** (Q14.4) - Create singleton with Battle screen always active
9. ✅ **BattleTeamLoaderSystem level parsing** (Q14.8) - Parse level from JSON, default to 1
10. ✅ **Result file management** (Q14.9, Q14.10) - timestamp_battle_id.json, common rotation helper
11. ✅ **Shop systems inclusion** (Q14.13) - Skip shop systems entirely
12. ✅ **Temporary file cleanup** (Q14.1) - Keep last 10, use rotation helper
13. ✅ **Error handling strategy** (Q14.11) - Try-catch with error detail level flag
14. ✅ **System registration verification** (Q14.14) - Compile-time assertions/unit tests
15. ✅ **Debug mode** (Q14.12) - Global flag on startup
16. ✅ **Path handling** (Q14.17) - Configurable base path in config file

## Critical Risks and Implementation Considerations

### 🔴 High Priority Risks

1. **Determinism Breaking**: System registration order MUST match main game exactly. Different order = different results.
   - **Mitigation**: Add unit tests + runtime validation (Q14.14)

2. **GameStateManager Dependency**: Many systems check `GameStateManager::Screen::Battle`. Server must initialize singleton.
   - **Mitigation**: Create `GameStateManager` singleton with `Screen::Battle` always active (Q14.4)

3. **Entity Cleanup**: `CleanupBattleEntities` relies on screen transitions. Server must manually trigger cleanup.
   - **Mitigation**: Manually mark entities for cleanup and call `EntityHelper::cleanup()` after each battle (Q14.2)

4. **RNG State Reset**: Must call `SeededRng::set_seed()`, not just set `seed` field directly.
   - **Mitigation**: Use `SeededRng::get().set_seed(battle_seed)` in `start_battle()` (Q14.5)

5. **Exception Handling**: Unhandled exceptions crash entire server.
   - **Mitigation**: Wrap entire simulation loop in try-catch (Q14.11)

### 🟡 Medium Priority Risks

1. **Shared System Modifications**: Modifying `InitCombatState` or `BattleTeamLoaderSystem` must not break client.
   - **Mitigation**: Use `render_backend::is_headless_mode` checks, ensure backward compatibility

2. **File Path Management**: Relative paths break if server run from different directory.
   - **Mitigation**: Use absolute paths or configurable base path (Q14.17)

3. **Shop System Dependencies**: `ProcessBattleRewards` may need shop state. Verify if it runs in server.
   - **Mitigation**: Check if `ProcessBattleRewards` needs shop singletons; stub if needed (Q14.13)

### 🟢 Low Priority / Implementation Details

1. **Temp File Cleanup**: Keep last 10 temp files, rotate like result files (Q14.1)
2. **Result File Rotation**: Common helper function for file rotation (Q14.10)
3. **Debug Mode**: Global flag + per-request override (Q14.12)
4. **SystemManager Reuse**: Reuse single instance with validation checks (Q14.3)
5. **Preload**: Skip entirely, modify BattleTeamLoaderSystem to skip sprite creation in headless mode (Q14.19)

## All Section 14 Questions - Final Decisions

**Decided**:
- ✅ Q14.1: Keep last 10 temp files (rotation helper)
- ✅ Q14.2: Let engine handle cleanup, watch for side effects
- ✅ Q14.3: Reuse SystemManager with validation checks on battle load
- ✅ Q14.4: Create GameStateManager singleton with Battle screen
- ✅ Q14.5: Call `set_seed()` for each battle (definitely)
- ✅ Q14.6: Reuse BattleLoadRequest singleton
- ✅ Q14.7: Skip all animations on server (never any animation)
- ✅ Q14.8: Parse level from JSON in BattleTeamLoaderSystem
- ✅ Q14.9: `timestamp_battle_id.json` format, team IDs in file contents
- ✅ Q14.10: Common helper function for file rotation
- ✅ Q14.11: Try-catch with error detail level flag (log internally, sanitize for users)
- ✅ Q14.12: Global debug flag on startup (--debug flag or config file)
- ✅ Q14.13: Skip shop systems
- ✅ Q14.14: Compile-time assertions/unit tests
- ✅ Q14.15: Configurable timeout, save timeout battles to debug location
- ✅ Q14.16: Create opponent directory on startup if missing
- ✅ Q14.17: Configurable base path in config file (defaults to working directory)
- ✅ Q14.18: Compile-time check for BattleFingerprint
- ✅ Q14.19: Skip Preload, modify BattleTeamLoaderSystem to skip sprite creation
- ✅ Q14.20: Set default CORS headers in setup_routes()
