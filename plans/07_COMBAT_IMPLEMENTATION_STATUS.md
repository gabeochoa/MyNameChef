# Combat Implementation Status & Next Steps

## At-a-Glance
- **Sequence:** 07 / 21 — canonical combat backlog for SAP-style battles.
- **Goal:** Complete deterministic head-to-head combat loop with all supporting systems (triggers, effects, replays).
- **Current State:** Phases 1–2 largely complete; Phase 3 systems and downstream replay hooks outstanding.
- **Critical Dependencies:** 
  - Trigger/effect plumbing (Plans `09`, `10`, `11`)
  - Replay UX (Plan `19`)
  - Backend parity (Plans `20`, `21`) for determinism verification
- **Readiness KPI:** Combat QA suite covers seeds, pairings, outcomes, and agrees with server simulations.

## Work Breakdown Snapshot
|Phase|Status|Dependencies|Exit Criteria|
|---|---|---|---|
|1. Baseline Fixes|✅ done|n/a|Deterministic tick logic, outcomes & win counts recorded|
|2. Missing Components|⚠️ partial|Shared structs + trigger/effect components|CourseOutcome, TriggerEvent/Queue, Deferred/Pending mods committed + tested|
|3. Systems Bring-up|⚠️ not started|Plans `09`, `10`, `11`|ApplyPairingsAndClashes, TriggerDispatch, EffectResolution online with tests|
|4. E2E Validation|Blocked|Replay infra (Plan `19`), deterministic RNG|Seeded runs + edge cases validated, automation recorded|
|5. Advanced Features|Deferred|Status/duration/effect chain plans|Replay UI + advanced combat modifiers feature-flagged|

### Coordination Notes
1. Treat this plan as the authoritative checklist for combat readiness; other plans reference its phase numbering.
2. When a dependency (e.g., TriggerDispatch) lands, immediately re-run the combat regression suite.
3. Update `Priority Order` section when phases complete so downstream plans know when they can start.

## Current Implementation Status

### ✅ **COMPLETED (Phase 1 & 2)**

**Components:**
- ✅ `CombatStats` - Implemented with baseZing, baseBody, currentZing, currentBody
- ✅ `PreBattleModifiers` - Implemented with zingDelta, bodyDelta  
- ✅ `CombatQueue` - Implemented as singleton with current_index, total_courses, complete
- ✅ `DishBattleState` - Updated with new phases (InQueue, Entering, InCombat, Finished) and fields
- ✅ `BattleResult` - Updated with playerWins, opponentWins, ties, outcomes vector

**Systems:**
- ✅ `InitCombatState` - Resets combat queue and sets dishes to InQueue
- ✅ `ComputeCombatStatsSystem` - Calculates Zing/Body from FlavorStats with level scaling
- ✅ `StartCourseSystem` - Finds dishes for current slot and starts entering animation
- ✅ `BattleEnterAnimationSystem` - Handles enter_progress animation (0.45s duration)
- ✅ `ResolveCombatTickSystem` - Alternating bite combat with 150ms tick rate
- ✅ `AdvanceCourseSystem` - Advances to next course when both dishes finished

**UI:**
- ✅ `RenderZingBodyOverlay` - Shows Zing/Body numbers on dishes
- ✅ Combat systems registered in main.cpp

### ❌ **MISSING/INCOMPLETE**

**Missing Components:**
- ❌ `CourseOutcome` component (exists in BattleResult but not as standalone)
- ❌ Trigger system components (`TriggerEvent`, `TriggerQueue`)
- ❌ Effect system components (`DeferredFlavorMods`, `PendingCombatMods`)

**Missing Systems:**
- ❌ `ApplyPairingsAndClashesSystem` (Phase 3)
- ❌ `TriggerDispatchSystem` (Phase 2b) 
- ❌ `EffectResolutionSystem` (Phase 3b)
- ❌ Replay systems (Phase 2c, 5)

**Critical Issues Found:**
- ❌ `ResolveCombatTickSystem` has static `player_turn` variable (not deterministic)
- ❌ No course outcome recording when dishes finish
- ❌ No win counting in `BattleResult`
- ❌ Missing transition to Results when combat completes

## Implementation Plan

### **Phase 1: Fix Core Combat Issues**

**Goal:** Fix critical bugs preventing proper combat flow

**Tasks:**

1. **Fix Deterministic Turn Alternation**
   - Replace static `player_turn` in `ResolveCombatTickSystem` with deterministic logic
   - Use `SeededRng` for any random decisions
   - Ensure same seed produces identical combat sequence

2. **Add Course Outcome Recording**
   - Record `CourseOutcome` when dishes finish combat
   - Determine winner based on which dish's Body reached 0 first
   - Store outcome in `BattleResult.outcomes` vector

3. **Add Win Counting**
   - Increment `BattleResult.playerWins`/`opponentWins`/`ties` based on outcomes
   - Update win counts after each course completes

4. **Fix Results Transition**
   - Ensure `AdvanceCourseSystem` properly transitions to Results when all courses complete
   - Verify `GameStateManager::to_results()` is called correctly

**Files to modify:**
- `src/systems/ResolveCombatTickSystem.h` - Fix turn alternation
- `src/systems/AdvanceCourseSystem.h` - Add outcome recording and win counting
- `src/components/battle_result.h` - Ensure proper structure

### **Phase 2: Complete Missing Components**

**Goal:** Add remaining components needed for full combat system

**Tasks:**

1. **Create Standalone CourseOutcome Component**
   ```cpp
   struct CourseOutcome : BaseComponent {
     int slotIndex = 0;
     enum struct Winner { Player, Opponent, Tie } winner = Winner::Tie;
     int ticks = 0;
   };
   ```

2. **Add Trigger System Components**
   ```cpp
   struct TriggerEvent : BaseComponent {
     enum struct Hook { OnServe, OnBiteTaken, OnDishFinished, OnCourseStart, OnCourseComplete } hook;
     int sourceEntityId = 0;
     int slotIndex = 0;
     DishBattleState::TeamSide teamSide = DishBattleState::TeamSide::Player;
     // payload data
   };
   
   struct TriggerQueue : BaseComponent {
     std::vector<TriggerEvent> events;
   };
   ```

3. **Add Effect System Components**
   ```cpp
   struct DeferredFlavorMods : BaseComponent {
     int satiety = 0, sweetness = 0, spice = 0, acidity = 0, umami = 0, richness = 0, freshness = 0;
   };
   
   struct PendingCombatMods : BaseComponent {
     int deltaZing = 0, deltaBody = 0;
   };
   ```

**Files to create:**
- `src/components/course_outcome.h`
- `src/components/trigger_event.h`
- `src/components/trigger_queue.h`
- `src/components/deferred_flavor_mods.h`
- `src/components/pending_combat_mods.h`

### **Phase 3: Implement Missing Systems**

**Goal:** Add remaining combat systems for full functionality

**Tasks:**

1. **ApplyPairingsAndClashesSystem**
   - Scan team lineups for pairings/clashes
   - Apply simple global rules (+Body for pairings, -Zing for clashes)
   - Write modifiers to `PreBattleModifiers` components

2. **TriggerDispatchSystem**
   - Process `TriggerQueue` events in deterministic order
   - Emit `OnServe`, `OnBiteTaken`, `OnDishFinished` hooks
   - Order events by `(slotIndex asc, Player then Opponent, sourceEntityId asc)`

3. **EffectResolutionSystem**
   - Handle basic effect operations: `AddFlavorStat`, `AddCombatBody`
   - Apply effects to `DeferredFlavorMods` or `CombatStats`
   - Support targeting scopes: `Self`, `Opponent`, `AllAllies`, etc.

**Files to create:**
- `src/systems/ApplyPairingsAndClashesSystem.h`
- `src/systems/TriggerDispatchSystem.h`
- `src/systems/EffectResolutionSystem.h`

### **Phase 4: Test Complete Combat Flow**

**Goal:** Verify end-to-end combat functionality

**Tasks:**

1. **Test Battle Flow**
   - Battle starts → dishes enter → combat resolves → results screen
   - Verify win counting works correctly
   - Ensure proper transition to Results

2. **Test Determinism**
   - Same seed produces identical combat sequence
   - Verify replayability works

3. **Test Edge Cases**
   - Empty slots, ties, level scaling
   - Verify no crashes or infinite loops

### **Phase 5: Advanced Features (Future)**

**Goal:** Add advanced combat features

**Tasks:**

1. **Replay System**
   - `ReplayState`, `ReplayControllerSystem`, `ReplayUISystem`
   - Pause/Play, Speed controls, Seek to course
   - Load `BattleReport` from JSON

2. **Battle Report Persistence**
   - Generate `BattleReport` with seed, outcomes, events
   - JSON serialization at `output/battles/results/*.json`
   - Results screen "Replay" button

3. **Advanced Effects**
   - Status effects, team auras
   - Complex targeting and timing rules
   - Rich effect payload system

## Priority Order

1. **Phase 1** - Fix core combat issues (CRITICAL - system doesn't work without these)
2. **Phase 2** - Complete missing components (HIGH - needed for advanced features)
3. **Phase 3** - Implement missing systems (MEDIUM - adds functionality)
4. **Phase 4** - Test complete flow (HIGH - verify everything works)
5. **Phase 5** - Advanced features (LOW - future enhancements)

## Success Criteria

- [ ] Game compiles clean after legacy removal
- [ ] Combat flows from battle start to results without crashes
- [ ] Win counting works correctly
- [ ] Same seed produces identical combat sequence
- [ ] All 7 courses resolve properly
- [ ] Results screen shows correct winner

## Estimated Completion

- **Phase 1**: 2-3 hours (fixing core bugs)
- **Phase 2**: 1-2 hours (adding components)
- **Phase 3**: 3-4 hours (implementing systems)
- **Phase 4**: 1-2 hours (testing)
- **Total**: 7-11 hours for basic working combat system

The combat system is ~70% complete with solid foundations, but needs these critical fixes to function properly.

## Outstanding Questions
1. **Ownership:** Which engineer owns ongoing combat maintenance once Phases 2–3 land (needed for fast bug triage)?
2. **Determinism Validation:** Do we require nightly seed-comparison tests between client + server before promoting Phase 4 to “complete”?
3. **Effect Scope:** Should Phase 3’s EffectResolutionSystem launch with a subset of operations or wait for full Plan 09/10/11 alignment?
4. **Replay Coupling:** Does Phase 5 block on Replay UX (Plan 19) or can we ship headless replay hooks earlier?
5. **Instrumentation:** What metrics (tick duration, event queue depth) are mandatory to monitor combat health post-launch?
