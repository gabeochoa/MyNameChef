# Next TODO - Game Development Priorities

## ✅ Completed Systems

- **Server Implementation**: ✅ Complete (all steps done, server builds and runs)
- **TriggerDispatchSystem**: ✅ Complete (exists and functional)
- **EffectResolutionSystem**: ✅ Complete (exists with full targeting and operations)
- **ApplyPairingsAndClashesSystem**: ✅ Complete (exists and registered)
- **Tier 1 Food Trigger Effects**: ✅ Complete (commit f9a902c - effects implemented with level-based scaling)
- **Effect System Test Fix**: ✅ Complete (commit d3c487b - salmon test fixed)
- **Legacy Judging Systems Removal**: ✅ Complete (commits 75a6323, 9568e3f, 2730127 - all legacy judging removed)
- **Level Scaling Bug Fix**: ✅ Complete (commit 0c0aa8e - fixed to use 2x per level instead of 2^(level-1))
- **Static Dish Pool Replacement**: ✅ Complete (uses magic_enum::enum_values internally)
- **Effect Info in Tooltips**: ✅ Complete (commit c93e68e - tooltips now display effect descriptions with triggers, targets, and stat changes)
- **Tags and SynergyCountingSystem**: ✅ Complete (all tag components exist, SynergyCountingSystem counts tags, tooltips display tags and synergy counts)
- **Dish Merging System**: ✅ Complete (merge system implemented, 2 level N dishes make 1 level N+1, uses DishLevel.contribution_value())
- **RerollCost, Freezeable, and Seeded RNG Cleanup**: ✅ Complete (all shop RNG uses SeededRng, RerollCost is deterministic, Freezeable works correctly, tests pass)
- **Survivor Carryover Test**: ✅ Complete (ValidateSurvivorCarryoverTest.h exists and tests survivor retargeting and position updates)
- **BattleReport Persistence**: ✅ Complete (BattleReport component, SaveBattleReportSystem, JSON serialization, file retention, tests pass)
- **Set Bonus Effects & Legend UI**: ✅ Complete (set bonuses apply, legend displays, tests pass - core functionality complete)
- **Server RNG Determinism**: ✅ Complete (random_device replaced with SeededRng, seed stored in response, determinism test created)
- **Server-battle test sync**: ✅ Complete (server-battle tests waited on ECS course completion, but the authoritative `BattleProcessor` sim finishes first and the Results transition freezes ECS mid-course; `validate_server_opponent_match` and `validate_server_battle_integration` now wait for the Results screen instead)

---

## ⚠️ MANDATORY WORKFLOW BETWEEN TASKS

**CRITICAL: Before moving to the next task, you MUST:**

1. **Build**: Run `make` - code must compile successfully
2. **Test (Headless)**: Run `./scripts/run_all_tests.sh` - ALL tests must pass
3. **Test (Non-Headless)**: Run `./scripts/run_all_tests.sh -v` - ALL tests must pass  
4. **Commit**: `git commit -m "<message>"` with appropriate prefix:
   - `bf -` for bug fixes
   - `be -` for backend/engine changes
   - `ui -` for UI changes
   - No prefix for new features
5. **Only then proceed** to the next task

**Do NOT skip these steps. Each task must be fully tested and committed before starting the next one.**

---

## Tier 1: Critical Bug Fixes & Test Coverage (2-4hrs each)

---

## Plan Creation Tasks

1. **Combine Duplicates into Levels (3-of-a-kind)** ✅ Complete
   - **Plan**: See `plans/three_of_a_kind_merge_plan.md`
   - Implemented: one drag onto a same-type dish consumes an adjacent same-type inventory dish too, adding both donors' full contributions (no value destroyed). Triple-merge drop target highlights cyan. `ValidateTripleMergeTest` covers triple, far-third double-only, and mixed-level cases. Also fixed shared drop-slot ID ambiguity (`whereSlotIDInArea`) exposed by the work.

2. **Shop UX Affordances (Prices & Drop Highlights)** ✅ Complete
   - **Plan**: See `plans/shop_ux_affordances_plan.md`
   - Verified already implemented: price display (`RenderShopPriceDisplaySystem`), drop highlights (`RenderDropTargetHighlightsSystem`, registered in `main.cpp`), match star (`RenderShopItemMatchIndicatorSystem`), reroll label `Reroll (X gold)`.

---

## Tier 2: Core Gameplay Features (4-8hrs each)

---

## Tier 3: Enhanced Features & Polish (6-12hrs each)

### Task 4: Enhance Replay System ✅ Complete
**Files**: `src/replay.h`, `src/systems/ReplayControllerSystem.h`, Results/History UI
- **Plan**: See `plans/replay_system_enhancement_plan.md`
- **Completed**: speed buttons 0.5x/1x/2x/4x fixed to match labels, shared replay helpers, Watch Again on Results, History uses shared helper, Watch Again replays award no rewards, invalid-report handling. Covered by `ValidateReplayControlsTest`. Seek-to-course / event-log viewer skipped per plan.
- **Enhancements Needed**:
  - Speed controls (0.5x/1x/2x/4x) - partially exists, needs UI buttons
  - Load BattleReport from saved JSON files (from Task 3)
- **Removed Features** (not needed):
  - Seek to course (skip this feature)
  - Event log viewer (not needed)
- **UI Implementation**: Add UI buttons for speed controls (0.5x, 1x, 2x, 4x) on screen during replay
- **Fun Ideas**:
  - Timeline scrubber showing battle progress
  - Slow-motion replay for epic moments
  - "Watch Again" button on Results screen
  - Battle highlights (auto-detect exciting moments)
- **Validation Steps**:
  1. Create ReplayUISystem for speed control buttons (0.5x/1x/2x/4x)
  2. Add BattleReport loading from JSON (integrate with Task 3)
  3. Integrate with Results screen
  4. **MANDATORY CHECKPOINT**: Build: `make`
  5. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  6. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  7. Verify can replay any saved battle, speed controls work (manual test)
  8. **MANDATORY CHECKPOINT**: Commit: `git commit -m "enhance replay system with speed controls and battle history"`
  9. **Only after successful commit, proceed to Task 6**
- **Estimated**: 4-6 hours

### Task 6: Enhanced Shop UX ✅ Complete
**Files**: Update shop systems and UI
- **Plan**: See `plans/shop_ux_affordances_plan.md` (from Plan Creation Tasks #4)
- **Completed**: verified already implemented (price display, drop highlights, match star, reroll cost label). Fun ideas below remain stretch.
- **Features**:
  - Show price in shop (all items cost 3 gold, so display this in shop UI, not on each item)
  - Note: Eventually drinks will cost 1 gold and get paired with dishes, but not implementing that yet
  - Faint highlight on legal drop targets (empty inventory slots, or slots where dish can merge)
  - Reroll cost display (current cost, increment)
- **Removed Features** (already implemented):
  - "Frozen" badge on frozen items (already done)
  - Freeze toggle (already done, no changes needed)
- **Fun Ideas**:
  - Item rarity glow (tier 1-5 different colors)
  - "Recommended" tag for dishes that synergize with current team
  - Shop refresh animation
  - Sound effects for purchase/sell
- **Validation Steps**:
  1. Add price display to shop UI (show "3 gold" somewhere visible)
  2. Add drop target highlighting (faint highlight on empty slots or merge targets)
  3. Update reroll cost display
  4. **MANDATORY CHECKPOINT**: Build: `make`
  5. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  6. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  7. Verify all UX improvements work correctly (manual test)
  8. **MANDATORY CHECKPOINT**: Commit: `git commit -m "ui - enhance shop UX with prices, drop highlights, and reroll cost"`
- **Estimated**: 3-4 hours

## Tier 4: Advanced Systems (10+ hours each)

### Task 10: Implement Status Effects ✅ Complete
**Files**: `src/components/status_effects.h`, `src/systems/EffectResolutionSystem.h`, `src/systems/RenderStatusEffectBadgesSystem.h`
- **Plan**: See `plans/status_effects_plan.md` (design decisions recorded there)
- **Completed**: `StatusEffectType` (Heat +1 Zing, SweetTooth +1 Body, Fatigue -1 Body) on `StatusEffect` with `courseApplied`; stacking preserved (clamps bound it); PalateCleanser deferred (needs a behavior hook). Sources: `GarlicBread` OnServe applies Fatigue to Opponent; `WagyuSteak` debuff preserved. Badges render on battle dishes. Tests: `ValidateStatusEffectsTest` + revived `ValidateWagyuSteakTest`.
- **Goal**: Implement status effects system (e.g., PalateCleanser, Heat, SweetTooth, Fatigue)
- **Implementation**:
  - Create status effect components
  - Define status effect types
  - Extend EffectResolutionSystem to apply and track status effects
  - Add visual indicators for status effects (small badges on dishes)
- **Status Effect Design**: ⚠️ **NEEDS FULL DESIGN PLAN** - Consider:
  - **Source**: Maybe drinks and synergies will both give status effects
  - **Distinction**: Need to think through what kinds of things will be dish effects vs status effects
  - **Mechanics**: Status effects can do both:
    - Stat modifiers (e.g., Heat = +1 Spice)
    - Behavior modifiers (e.g., PalateCleanser = reset flavor stats)
  - **System Design**: Need to plan how status effects interact with dish effects, when they apply, how they stack, etc.
- **Fun Ideas**:
  - Status effect icons on dishes (small badges)
  - Effect tooltips explaining what each status does
- **Validation Steps**:
  1. **DESIGN PHASE**: Create full design plan for status effects:
     - What are status effects vs dish effects?
     - How do drinks and synergies give status effects?
     - What status effects exist and what do they do?
     - How do stat modifiers vs behavior modifiers work?
  2. Create status effect components based on design
  3. Extend EffectResolutionSystem with status effect tracking
  4. Add visual indicators for status effects
  5. **MANDATORY CHECKPOINT**: Build: `make`
  6. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  7. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  8. Verify status effects work correctly (manual test)
  9. **MANDATORY CHECKPOINT**: Commit: `git commit -m "implement status effects system"`
  10. **Only after successful commit, proceed to Task 11**
- **Estimated**: 6-8 hours

### Task 11: Implement Duration-Based Effects ✅ COMPLETE
**Files**: Extend `EffectResolutionSystem.h`
- **Plan**: See `plans/duration_based_effects_plan.md`
- **Goal**: Implement effects that last N courses (duration-based effects)
- **Implementation**:
  - `DishEffect.duration` (0 = permanent) + `TriggerEffectBuilder::with_duration`
  - `EffectResolutionSystem` ApplyStatus copies `effect.duration`
  - `AdvanceCourseSystem::expire_status_effects` runs after each `current_index++`
    (both normal and max-courses extension branches); expires when
    `currentCourse - courseApplied >= duration`; removes empty `StatusEffects`
  - GarlicBread Fatigue given duration 2 (all levels)
  - Re-application stacks (Task 10 decision kept); each stack entry expires
    independently by its own `courseApplied`
- **Validation Steps**:
  1. Extend EffectResolutionSystem with duration tracking ✅
  2. Add course tracking for effects ✅ (already recorded in Task 10)
  3. Implement expiration logic ✅
  4. **MANDATORY CHECKPOINT**: Build: `make` ✅
  5. **MANDATORY CHECKPOINT**: Run headless tests: `python3 scripts/run_tests.py` (102/102) ✅
  6. **MANDATORY CHECKPOINT**: Run non-headless test for new test ✅
  7. New test `validate_duration_effect_expires_after_two_courses` covers
     Fatigue present after course 0 (duration 2, baseBody 5→4) and expired
     after course 1 (no StatusEffects, baseBody back to 5)
  8. **MANDATORY CHECKPOINT**: Commit ✅
  9. **Only after successful commit, proceed to Task 12**
- **Estimated**: 4-6 hours

### Task 12: Implement Effect Chains and Dependencies ✅ COMPLETE
**Files**: Extend `EffectResolutionSystem.h`
- **Plan**: See `plans/effect_chains_plan.md`
- **Goal**: Implement effect chains where one effect can trigger another effect
- **Implementation** (hybrid: explicit chains + implicit trigger flow):
  - `DishEffect.chainedEffects` (vector, value semantics) + `requiresStatus`
  - Builder: `TriggerEffectBuilder::then(...)` (any hook) + `if_target_has_status(...)`
  - `EffectResolutionSystem::apply_effect(effect, ev, depth)` resolves chained
    effects after the parent, same source event; chained hooks are not
    re-matched (chain fires because the parent fired)
  - `MAX_EFFECT_CHAIN_DEPTH = 10`: parent + 10 chained apply, deeper links
    dropped with a `log_warn`. Cycles are structurally impossible (value
    semantics = finite trees); depth cap bounds pathological deep trees
  - `requiresStatus` filters targets in the target loop; chained effects get
    their own `conditional` check via `apply_effect`
  - CopyEffect stays terminal: its own chained effects never fire
- **Dogfood**:
  - GarlicBread OnServe Fatigue chains `-1 Zing to Opponent if target has Fatigue`
  - WagyuSteak OnBiteTaken `-1 Zing` chains `-1 Body to Opponent if target has Fatigue`
- **Fun Ideas**:
  - Chain reaction animations when effects trigger other effects (still open)
- **Validation Steps**:
  1. Design effect chain system ✅ (hybrid, see plan)
  2. Implement effect dependency tracking ✅ (explicit `chainedEffects` list)
  3. Add effect chain resolution logic ✅
  4. **MANDATORY CHECKPOINT**: Build: `make` ✅
  5. **MANDATORY CHECKPOINT**: Run headless tests: `python3 scripts/run_tests.py` (106/106) ✅
  6. **MANDATORY CHECKPOINT**: Run non-headless tests for the 4 new tests ✅
  7. New tests in `ValidateEffectChainsTest.h`: GarlicBread chain fires,
     Wagyu chain blocked without Fatigue, Wagyu chain fires with Fatigue,
     15-deep injected chain stops at 11 applied
  8. **MANDATORY CHECKPOINT**: Commit ✅
  9. **Only after successful commit, proceed to Task 13**
- **Estimated**: 6-8 hours
- **Notes**:
  - Depth test uses `OnBiteTaken` (not `OnServe`): `SimplifiedOnServeSystem`
    also fires `OnServe` naturally when a stray dish's phase churns to
    `InQueue`, which double-resolved the chain (22 = 2x11). Manual
    `fire_trigger(OnServe)` + natural OnServe is a pre-existing test-harness
    overlap, not changed here.

### Task 13: Visual Enhancements for Dish Merging ✅ COMPLETE
**Files**: Update rendering systems
- **Plan**: See `plans/dish_merging_visuals_plan.md`
- **Goal**: Add visual feedback for dish merging (merge system already implemented)
- **Note**: Dish merging is already implemented (2 level N dishes make 1 level N+1). This task is just visual polish.
- **Level Scaling**: Uses existing 2x per level system (level 2 = 2x, level 3 = 4x, level 4 = 8x). See `ComputeCombatStatsSystem.h` lines 95-104.
- **Merge Trigger**: Already implemented - user drags dish onto another dish of same type to merge. See `DropWhenNoLongerHeld.cpp` merge_dishes function.
- **Implementation**:
  - Merge animation: `merge_dishes` adds `MergeFlash` (1s) to the target;
    `MergeFlashTimerSystem` expires it, `RenderMergeFlashSystem` draws an
    expanding gold ring + `LEVEL UP!` text only when the merge leveled up.
    Self-contained (Battle-only `AnimationEvent` pipeline not reused: merges
    happen on the Shop screen)
  - Level-up sound: `SoundFile::Merge_LevelUp` (`PlaySoundRequest` from
    `merge_dishes`), reuses the water-drip clip until a dedicated clip exists
  - "Ready to merge!" indicator: `RenderMergeHintSystem` draws a gold outline
    around inventory dishes with a merge partner (any same-type pair is
    mergeable: donor level <= target level always holds one way); hidden
    while dragging (drop highlights take over)
  - Level badge: `RenderDishLevelBadgeSystem` draws a gold circle + level
    number at the top-right for level 2+ dishes; old `L2` text removed from
    `RenderDishProgressBars` (pips kept)
  - Detection: `merge::dish_has_merge_partner` in `src/merge.h`, shared by
    the hint system and tests
- **Validation Steps**:
  1. Add merge animation ✅
  2. Add level-up sound effect ✅
  3. Add "Ready to merge!" visual indicator ✅
  4. Add level display badge on dishes ✅
  5. **MANDATORY CHECKPOINT**: Build: `make` ✅
  6. **MANDATORY CHECKPOINT**: Run headless tests: `python3 scripts/run_tests.py` (108/108) ✅
  7. **MANDATORY CHECKPOINT**: Run non-headless tests for the 2 new tests ✅
  8. New tests in `ValidateDishMergeVisualsTest.h`: real UI drag produces
     `MergeFlash` on target; partner detection true/true/false
  9. **MANDATORY CHECKPOINT**: Commit ✅
- **Estimated**: 4-6 hours
- **Bug fixed along the way**: `SoundLibrary::play` threw `std::out_of_range`
  for unloaded sounds (`load_sounds()` runs only when `!headless`), which
  crashed the first headless merge. `play()` now no-ops for unloaded sounds.

---

## Tier 5: Tooltip Enhancements ✅ COMPLETE (checked item by item)

### Tooltip Improvement Ideas

#### 1. Level Comparison & Progression Preview ✅ IMPLEMENTED
- **Plan**: See `plans/level_comparison_tooltip_plan.md`
- Stat lines show next-level value in parentheses when it differs:
  `Satiety: 1 (Level 2: 2)`; `Next Level:` section shows `[NEW]` effects
  and scaled/changed effects
- Max level uses `MAX_DISH_LEVEL` (10, code truth) not the plan's stale 4;
  max dishes show `(Max Level)` and no preview
- **Files**: `src/tooltip.h`
- **Validation**: full suite `python3 scripts/run_tests.py` (113/113) ✅;
  new tests in `ValidateTooltipTest.h` ✅; new tests run non-headless ✅

#### 2. Tags & Synergy Integration ✅ ALREADY IMPLEMENTED (verified, no change)
- `TooltipSystem` already shows Course/Cuisine/Brand/Archetype tags plus
  synergy counts with all thresholds (achieved normal, future muted)

#### 3. Combat Stats Preview ✅ IMPLEMENTED
- `Combat Stats: Zing: X, Body: Y` line from flavor stats, with level
  scaling `2^(level-1)` (code truth in `ComputeCombatStatsSystem`);
  shows `(base Zing: A, Body: B)` when scaled

#### 4. Effect Formatting Improvements ✅ IMPLEMENTED
- Effects grouped by trigger under `[BOLD]` headers; trigger prefix
  removed from individual lines
- Fixed `ApplyStatus` formatting: it showed only Zing delta (so GarlicBread
  Fatigue rendered as `Apply +0 Zing`). Now shows type + both deltas +
  duration: `Apply Fatigue (-1 Body) to opponent (2 courses)`
- Conditions shown: `If target has Fatigue: ...` (`requiresStatus`) and
  `If adjacent has Freshness: ...` (check is `> 0`; no invented `≥ 2`)
- Chained effects shown as `-> Then: ...` follow-up lines (depth-capped)

#### 5. Multi-Level Effect Preview ✅ IMPLEMENTED
- `Later Unlocks:` section lists effects first appearing at levels beyond
  next, one line per effect: `Level 3: OnServe: +1 Satiety to self`
  (Potato L1's unlock surfaces here; next-level preview covers L+1)

#### 6. Tooltip Positioning Enhancements ✅ MOSTLY ALREADY IMPLEMENTED (verified)
- Per-line width measurement + 4-edge clamping + oversize clamp already in
  `TooltipSystem` (tier doc's complaints were stale)
- Smart positioning to avoid UI elements NOT implemented: no UI-rect
  registry exists to avoid against; deferred rather than guessed

#### 7. Rich Formatting Support ✅ PARTIAL (bold headers; icons deferred)
- `[BOLD]` section headers (formatter already supports `[BOLD]`/`[ITALIC]`);
  color coding for stat types already present; synergy already shows counts
- Icons for effect types NOT implemented: no icon mapping exists for
  tooltip text; deferred

#### 8. Context-Aware Tooltips ✅ IMPLEMENTED (battle gap closed)
- Shop (price), inventory (level + merge progress), synergy hints already shown
- Battle gap closed: `TooltipSystem` appends live
  `Combat: Zing: current (base X), Body: current (base Y)` on Battle screen

---

## Fun Ideas to Make the Game More Engaging

### Visual & Audio Polish
- **Combat Animations**: 
  - Bite animations (dishes "taking a bite" from each other)
  - Damage number popups (floating damage text)
  - Victory/defeat animations per course
  - Critical hit effects (extra sparkles for high damage)
  
- **Sound Design**:
  - Unique sound for each dish type when served
  - Bite sound effects (crunchy, soft, etc. based on dish)
  - Synergy achievement fanfare
  - Shop purchase/sell sounds
  - Background music that changes per screen

- **Particle Effects**:
  - Sparkles when effects trigger
  - Synergy glow around dishes
  - Stat boost visual effects
  - Victory confetti on Results screen

### Gameplay Enhancements
- **Daily Challenges**: 
  - "Win with only Thai dishes"
  - "Use exactly 4 courses"
  - "Win without losing a single course"
  
- **Achievement System**:
  - "First Victory", "Perfect Battle" (7-0), "Synergy Master" (6-piece bonus)
  - Unlockable dish types or cosmetics
  
- **Opponent Variety**:
  - Themed opponents (Italian chef, Thai street vendor, etc.)
  - Opponent backstories/personalities
  - Difficulty scaling based on player progress

- **Team Building Hints**:
  - "This dish synergizes with your current team!"
  - "You're 1 dish away from a 4-piece bonus"
  - Recommended purchases based on current team

### Meta Progression
- **Unlock System**:
  - Unlock new dish types as you play
  - Unlock new cuisines/brands
  - Unlock cosmetic variants
  
- **Collection System**:
  - Dish collection viewer
  - "Seen" vs "Owned" tracking
  - Rarity indicators

- **Statistics Tracking**:
  - Win/loss record
  - Favorite dishes
  - Most used synergies
  - Battle history with replays

---

## Priority Recommendations

**Immediate (This Week)**:
- Tier 3: Replay enhancements and shop UX (7-10 hours)

**Short Term (Next 2 Weeks)**:
- Tier 3: Shop UX improvements (3-4 hours)

**Medium Term (Next Month)**:
- Tier 3: Font standardization, text formatting, tooltip positioning (14-20 hours)
- Tier 4: Start advanced effects (status effects, duration, chains) (16-22 hours)

**Long Term (Future)**:
- Tier 4: Complete advanced systems
- Fun ideas implementation
- Meta progression systems
- Polish and optimization

---

## Success Criteria

- All tests pass in headless and non-headless modes
- Combat flows from battle start to results without crashes
- Win counting works correctly
- Same seed produces identical combat sequence (critical for server sync)
- All 7 courses resolve properly
- Results screen shows correct winner
- Client simulation matches server results exactly when using same seed
- Tags and synergies display correctly
- Effects trigger and apply as expected
- Battle reports save and load correctly

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
1. First ensure deterministic simulation works with local seeds ✅
2. Then add server JSON deserialization ✅
3. Finally add HTTP client for server communication (future task)
