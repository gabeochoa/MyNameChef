# Next TODO - Combat System Improvements Plan

## Server-Authoritative Battle Architecture

**Flow**:
1. **Client sends team to server**: `POST /battle` with `{ userId, teamId, teamSnapshot }`
2. **Server simulates battle**:
   - Picks random opponent team
   - Generates deterministic seed
   - Simulates battle using seed (all RNG uses this seed)
   - Returns JSON: `{ seed: uint64, opponentId: string, outcomes: [...], events: [...] }`
3. **Client replays battle**:
   - Receives server response with seed and results
   - Uses seed to initialize `SeededRng` for battle simulation
   - Replays battle deterministically (must match server results exactly)
   - Visual playback via ReplaySystem
   - Stores BattleReport locally for later replay

**Key Requirements**:
- Battle seed comes FROM server (not generated client-side)
- All combat RNG must use server-provided seed via SeededRng
- Client simulation with server seed must produce identical results to server
- Server results JSON contains: seed, opponentId, outcomes, events
- BattleReport deserialization must handle server JSON format

## Implementation Plan - Simplest to Most Complex

### Workflow Requirements

**CRITICAL: Test Between Every Commit**
- Each task must be a **separate commit**
- After every change:
  1. Build: `xmake`
  2. Run headless tests: `./scripts/run_all_tests.sh`
  3. Run non-headless tests: `./scripts/run_all_tests.sh -v`
  4. Both test runs **must pass** before committing
  5. Only then create the commit with appropriate message

**Commit Message Format**:
- Use prefixes: `bf -` for bug fixes, `be -` for backend/engine, `ui -` for UI, no prefix for features
- All lowercase, concise and descriptive


## Tier 3: System Implementation (2-4hrs each)

### Task 11: Verify/Complete TriggerDispatchSystem
**Files**: `src/systems/TriggerDispatchSystem.h` (exists, may need completion)
- **Status**: File exists, verify implementation completeness
- **Needs**: 
  - Deterministic event ordering (slotIndex asc, Player then Opponent, sourceEntityId asc)
  - All hook handlers implemented (OnServe, OnBiteTaken, OnDishFinished, OnCourseStart, OnCourseComplete)
- **Validation Steps**:
  1. Review and complete TriggerDispatchSystem implementation
  2. Add any missing handlers
  3. Build: `xmake`
  4. Run headless tests: `./scripts/run_all_tests.sh`
  5. Run non-headless tests: `./scripts/run_all_tests.sh -v`
  6. Verify all hooks fire in correct order during battle (check logs)
  7. Commit: `git commit -m "complete TriggerDispatchSystem with all hook handlers"`
- **Estimated**: 2-3 hours

### Task 12: Verify/Complete EffectResolutionSystem
**Files**: `src/systems/EffectResolutionSystem.h` (exists, may need completion)
- **Status**: File exists, verify implementation completeness
- **Needs**:
  - Effect operations: AddFlavorStat, AddCombatBody, AddCombatZing
  - Targeting system: Self, Opponent, AllAllies, DishesAfterSelf, FutureAllies
  - Integration with DeferredFlavorMods and PendingCombatMods
- **Validation Steps**:
  1. Review and complete EffectResolutionSystem implementation
  2. Add missing effect operations and targeting logic
  3. Integrate with DeferredFlavorMods and PendingCombatMods
  4. Build: `xmake`
  5. Run headless tests: `./scripts/run_all_tests.sh`
  6. Run non-headless tests: `./scripts/run_all_tests.sh -v`
  7. Verify sample effects (FrenchFries, Salmon) work correctly (check stats in combat)
  8. Commit: `git commit -m "complete EffectResolutionSystem with targeting and operations"`
- **Estimated**: 3-4 hours

### Task 13: Fix Survivor Carryover Visual Issue
**Files**: `src/systems/StartCourseSystem.h`, `src/systems/AdvanceCourseSystem.h`
- **Issue**: All dishes appear simultaneously instead of sequentially when survivor carryover occurs
- **Root Cause**: Transform position not updated on retargeting, dishes not restored after course completion
- **Fix**: 
  - Update Transform.position when retargeting to slot 0 position
  - Restore retargeted dishes to original slots after course finishes
  - Handle active survivors properly
- **Validation Steps**:
  1. Fix Transform position updates in StartCourseSystem
  2. Fix dish restoration in AdvanceCourseSystem
  3. Build: `xmake`
  4. Run headless tests: `./scripts/run_all_tests.sh` (especially validate_survivor_carryover)
  5. Run non-headless tests: `./scripts/run_all_tests.sh -v`
  6. Verify dishes appear one course at a time, proper visual flow (manual test)
  7. Commit: `git commit -m "bf - fix survivor carryover visual issue with transform positions"`
- **Estimated**: 2-3 hours

### Task 14: Fix Effect System Test Issue
**Files**: `src/testing/tests/ValidateEffectSystemTest.h` (find salmon test)
- **Issue**: Test `test_salmon_neighbor_freshness_persists_to_combat` fails - handler can't find entities
- **Root Cause**: Entity query timing in tests vs production (merge timing)
- **Fix**: Ensure entity merge happens before handler queries in test setup
- **Validation Steps**:
  1. Fix entity merge timing in test setup
  2. Build: `xmake`
  3. Run headless tests: `./scripts/run_all_tests.sh` (especially validate_effect_system)
  4. Run non-headless tests: `./scripts/run_all_tests.sh -v`
  5. Verify test passes and salmon effect applies correctly
  6. Commit: `git commit -m "bf - fix effect system test entity merge timing"`
- **Estimated**: 1-2 hours

---

## Tier 4: New Feature Systems (4-8hrs each)

### Task 15: Implement Tags and SynergyCountingSystem
**Files**: Create tag components, `src/systems/SynergyCountingSystem.h`
- **Components Needed**:
  - `CourseTag`, `CuisineTag`, `BrandTag`, `DietaryTag`, `DishArchetypeTag`
  - `SynergyCounts` singleton
- **System**: Scan inventory, count tags, compute set thresholds (2/4/6)
- **UI**: Update tooltips to show tags and synergy counts
- **Validation Steps**:
  1. Create tag component files
  2. Create SynergyCountingSystem
  3. Update tooltip system to display tags
  4. Build: `xmake`
  5. Run headless tests: `./scripts/run_all_tests.sh`
  6. Run non-headless tests: `./scripts/run_all_tests.sh -v`
  7. Verify tags display, counts update correctly, tooltips show synergies (manual test)
  8. Commit: `git commit -m "implement tags and SynergyCountingSystem"`
- **Estimated**: 4-6 hours

### Task 16: Implement BattleReport Persistence and Server Integration
**Files**: Create/update `src/components/battle_report.h`, add JSON deserialization
- **Architecture**: Server-authoritative battle system
  - Client sends team snapshot (user ID, team ID, team data) to server
  - Server picks random opponent, generates seed, simulates battle deterministically
  - Server returns JSON: `{ seed, opponentId, outcomes[], events[] }`
  - Client receives JSON and uses seed to replay battle visually (must match server simulation)
- **Components**: `BattleReport{ opponentId, seed, outcomes[], events[], receivedFromServer }`
- **Deserialization**: Load BattleReport from server JSON response
- **Storage**: Store received reports in `output/battles/results/*.json` for local replay
- **Validation Steps**:
  1. Create BattleReport component
  2. Add JSON deserialization from server format
  3. Integrate with Results screen to save reports
  4. Build: `xmake`
  5. Run headless tests: `./scripts/run_all_tests.sh`
  6. Run non-headless tests: `./scripts/run_all_tests.sh -v`
  7. Verify reports saved and can be loaded (check output/battles/results/ directory)
  8. Verify client simulation with server seed matches server results exactly
  9. Commit: `git commit -m "implement BattleReport persistence with server JSON deserialization"`
- **Estimated**: 4-5 hours

### Task 17: Implement Tier 1 Food Trigger Effects
**Files**: Update `src/components/dish_types.cpp` handlers
- **Effects to Implement** (from HEAD_TO_HEAD_COMBAT_PLAN.md):
  - Salmon: OnServe → If adjacent has freshness, +1 freshness to Self/Previous/Next
  - Bagel: OnServe → +1 richness to DishesAfterSelf
  - Baguette: OnServe → -1 Zing to Opponent
  - Garlic Bread: OnServe → +1 spice to FutureAllies
  - Fried Egg: OnDishFinished → +2 Body to AllAllies
  - French Fries: Already implemented
  - Potato: No trigger (raw ingredient)
- **Validation Steps**:
  1. Implement each effect handler in dish_types.cpp
  2. Build: `xmake`
  3. Run headless tests: `./scripts/run_all_tests.sh` (especially validate_effect_system)
  4. Run non-headless tests: `./scripts/run_all_tests.sh -v`
  5. Verify each effect triggers and applies correctly (manual test each one)
  6. Commit: `git commit -m "implement tier 1 food trigger effects"`
- **Estimated**: 4-6 hours

---

## Tier 5: Complex Systems (8+ hours each)

### Task 18: Full Replay System (Client-Side Visual Playback)
**Files**: Create `src/components/replay_state.h`, `src/systems/ReplayControllerSystem.h`, `src/systems/ReplayUISystem.h`
- **Architecture**: Client replay is visual playback of server's deterministic simulation
  - Server already computed results using seed
  - Client receives seed + results from server
  - Client uses seed to re-simulate battle (deterministic - must match server exactly)
  - Replay system is just visual playback of the local deterministic simulation
- **Components**: `ReplayState`, event playback system
- **Features**: Pause/Play, Speed controls (0.5x/1x/2x/4x), Seek to course
- **Integration**: 
  - Load BattleReport from server JSON response
  - Use seed to initialize deterministic simulation
  - Visual playback of simulation matches server results
- **UI**: Replay controls on Results screen
- **Validation Steps**:
  1. Create ReplayState component
  2. Create ReplayControllerSystem for playback logic
  3. Create ReplayUISystem for controls
  4. Integrate with Results screen
  5. Build: `xmake`
  6. Run headless tests: `./scripts/run_all_tests.sh`
  7. Run non-headless tests: `./scripts/run_all_tests.sh -v`
  8. Verify client simulation with server seed produces identical results
  9. Verify can replay any saved battle, seek works, speed controls work (manual test)
  10. Commit: `git commit -m "implement full replay system with pause, speed, and seek"`
- **Estimated**: 8-12 hours

### Task 19: Advanced Effect System Extensions
**Files**: Extend `EffectResolutionSystem.h`
- **Advanced Features**:
  - Status effects (PalateCleanser, Heat, SweetTooth, Fatigue)
  - Complex targeting scopes (Next, DishesAfterSelf, FutureAllies, etc.)
  - Duration-based effects
  - Effect chains and dependencies
- **Validation Steps**:
  1. Extend EffectResolutionSystem with advanced features
  2. Add status effect components and handling
  3. Implement complex targeting and duration tracking
  4. Build: `xmake`
  5. Run headless tests: `./scripts/run_all_tests.sh`
  6. Run non-headless tests: `./scripts/run_all_tests.sh -v`
  7. Verify complex effects work correctly, no edge cases (extensive manual testing)
  8. Commit: `git commit -m "extend EffectResolutionSystem with status effects and advanced targeting"`
- **Estimated**: 10-15 hours

### Task 20: Move Code to Engine (Lower Priority)
**Files**: Various (see `move_to_engine.md`)
- **High Priority**: Math utilities, Utils, Translation Manager core
- **Medium Priority**: Query extensions, Resources/Files, Settings infrastructure
- **Lower Priority**: Testing framework, Render backend, Game state patterns
- **Note**: This is a refactoring task, not a feature addition
- **Estimated**: 20-40 hours total

---

## Feature Status Summary

### High Priority (Foundation)
1. **SeededRng** - Required for determinism and replay (critical for server sync)
2. **ApplyPairingsAndClashesSystem** - Core gameplay mechanic, component exists but unused
3. **Replace random_device with SeededRng** - In shop generation and menu export

### Medium Priority (Features)
4. **RerollCost and Freezeable** - Improve shop UX
5. **Tags and SynergyCountingSystem** - Foundation for set bonuses
6. **BattleReport persistence** - Enable replay system (with server integration)
7. **Fix level scaling bug** - Current 2^(level-1) may be too aggressive

### Low Priority (Polish)
8. **Replace static dish pool** - Code cleanup
9. **Enhanced tooltips** - UX improvement
10. **Animation polish** - Visual refinement

---

## Priority Recommendations

**Immediate (This Week)**:
- Tiers 1 & 2: Fix bugs and complete basic features (10-16 hours total)
- **20 separate commits**, each with tests run in both modes
- Ensure SeededRng system is ready for server seed injection

**Short Term (Next 2 Weeks)**:
- Tier 3: Complete trigger/effect systems and fix known issues (8-12 hours total)
- **4 separate commits**, each with tests run in both modes
- Verify all combat RNG uses SeededRng (deterministic)

**Medium Term (Next Month)**:
- Tier 4: Add tags, persistence, tier 1 effects (12-18 hours total)
- **3 separate commits**, each with tests run in both modes
- Implement server JSON deserialization and seed injection
- Verify deterministic simulation matches server results

**Long Term (Future)**:
- Tier 5: Replay system and advanced features (20+ hours total)
- **2 separate commits**, each with tests run in both modes
- Full server integration (HTTP client, team submission, result handling)

---

## Success Criteria

- All tests pass in headless and non-headless modes
- Combat flows from battle start to results without crashes
- Win counting works correctly
- Same seed produces identical combat sequence (critical for server sync)
- All 7 courses resolve properly
- Results screen shows correct winner
- **Client simulation matches server results exactly when using same seed** (critical for server-authoritative architecture)

---

## Notes on Server Integration

**Seed Management**:
- Shop seed: Client-generated (can use SeededRng with local seed)
- Battle seed: **Server-provided** (must be injected into SeededRng before battle simulation)
- All battle RNG must use the server-provided seed for determinism

**Battle Report Format** (from server):
```json
{
  "seed": 123456789,
  "opponentId": "opponent_abc123",
  "outcomes": [
    { "slotIndex": 1, "winner": "Player", "ticks": 3 },
    { "slotIndex": 2, "winner": "Opponent", "ticks": 5 }
  ],
  "events": [
    { "t_ms": 0, "course": 1, "type": "Enter", "side": "Player", "entityId": 101 },
    { "t_ms": 450, "course": 1, "type": "Serve" },
    { "t_ms": 700, "course": 1, "type": "Bite", "side": "Player", "payload": { "damage": 2 } }
  ]
}
```

**Implementation Order**:
1. First ensure deterministic simulation works with local seeds
2. Then add server JSON deserialization
3. Finally add HTTP client for server communication (future task)

