# Combat Implementation Status & Next Steps

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

**Phase 0 - Legacy Removal (CRITICAL):**
- ❌ `JudgingSystems.h` systems still exist and may conflict
- ❌ Legacy judging UI (`RenderJudges`, `RenderScoringBar`) still active
- ❌ `JudgingState` component still referenced in some systems

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

### **Phase 0: Remove Legacy Judging (IMMEDIATE PRIORITY)**

**Goal:** Remove legacy tug-of-war judging systems to prevent conflicts

**Tasks:**
1. **Remove JudgingSystems.h registrations from main.cpp**
   - Remove `#include "systems/JudgingSystems.h"` 
   - Remove `InitJudgingState` and `AdvanceJudging` system registrations
   - Verify no other files include `JudgingSystems.h`

2. **Guard/Remove Legacy UI Systems**
   - Comment out `RenderJudges` system registration
   - Comment out `RenderScoringBar` system registration  
   - Add `#ifdef LEGACY_JUDGING` guards around judge UI code

3. **Remove JudgingState References**
   - Update `BattleDebugSystem` to not check for `JudgingState`
   - Remove `JudgingState` includes from systems that don't need it
   - Mark `src/components/judging_state.h` as legacy (keep file for rollback)

4. **Verify Build**
   - Ensure project compiles clean after removal
   - Test that battle screen loads without judge totals

**Files to modify:**
- `src/main.cpp` - Remove JudgingSystems includes/registrations
- `src/systems/BattleDebugSystem.h` - Remove JudgingState checks
- `src/systems/RenderJudges.h` - Add LEGACY_JUDGING guards
- `src/systems/RenderScoringBar.h` - Add LEGACY_JUDGING guards

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

1. **Phase 0** - Remove legacy judging (CRITICAL - prevents conflicts)
2. **Phase 1** - Fix core combat issues (CRITICAL - system doesn't work without these)
3. **Phase 2** - Complete missing components (HIGH - needed for advanced features)
4. **Phase 3** - Implement missing systems (MEDIUM - adds functionality)
5. **Phase 4** - Test complete flow (HIGH - verify everything works)
6. **Phase 5** - Advanced features (LOW - future enhancements)

## Success Criteria

- [ ] Game compiles clean after legacy removal
- [ ] Combat flows from battle start to results without crashes
- [ ] Win counting works correctly
- [ ] Same seed produces identical combat sequence
- [ ] All 7 courses resolve properly
- [ ] Results screen shows correct winner

## Estimated Completion

- **Phase 0**: 1-2 hours (straightforward removal)
- **Phase 1**: 2-3 hours (fixing core bugs)
- **Phase 2**: 1-2 hours (adding components)
- **Phase 3**: 3-4 hours (implementing systems)
- **Phase 4**: 1-2 hours (testing)
- **Total**: 8-13 hours for basic working combat system

The combat system is ~70% complete with solid foundations, but needs these critical fixes to function properly.
