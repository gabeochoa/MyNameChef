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

---

## ⚠️ MANDATORY WORKFLOW BETWEEN TASKS

**CRITICAL: Before moving to the next task, you MUST:**

1. **Build**: Run `xmake` - code must compile successfully
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

### Task 1: Add Survivor Carryover Test
**Files**: Create `src/testing/tests/ValidateSurvivorCarryoverTest.h`
- **Plan**: See `plans/survivor_carryover_test_plan.md`
- **Goal**: Test that dishes that survive a course carry over to the next course correctly
- **Key Behaviors to Test**:
  - When a dish survives (has remaining Body after opponent finishes), it should retarget the next opponent
  - The dish stays in the same slot position, but the opponent moves forward (next opponent in queue)
  - Transform position should update correctly: both visual position (dish appears in correct spot) AND internal slot index should be correct
  - This is important for future features where dishes spawn other dishes - we need to verify spawned dishes are in the right spots too
  - Multiple survivors should handle correctly (each retargets appropriately)
  - Battle should only complete when one team has no active dishes (not when a single course finishes)
- **Test Cases**:
  1. Single survivor retargets next opponent correctly
  2. Transform position (visual and index) updates correctly on retarget
  3. Multiple survivors all retarget correctly
  4. Battle completion only occurs when one team exhausted
- **Validation Steps**:
  1. Create test file with survivor carryover scenarios
  2. **MANDATORY CHECKPOINT**: Build: `xmake`
  3. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  4. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  5. Verify test passes and survivor carryover works correctly
  6. **MANDATORY CHECKPOINT**: Commit: `git commit -m "add survivor carryover test coverage"`
  7. **Only after successful commit, proceed to Task 2**
- **Estimated**: 2-3 hours

---

## Plan Creation Tasks

1. **Combine Duplicates into Levels (3-of-a-kind)**
   - **Plan**: See `plans/three_of_a_kind_merge_plan.md`
   - Implement the merge system changes and tests guided by that plan to ensure entities consolidate correctly and grant level-ups.

2. **Shop UX Affordances (Prices & Drop Highlights)**
   - **Plan**: See `plans/shop_ux_affordances_plan.md`
   - Implement the UI/UX improvements per the plan, including tests or headless verifications that confirm highlights appear only on valid targets.

3. **Set Bonus Effects & Legend UI**
   - **Plan**: See `plans/set_bonus_system_plan.md`
   - **Status**: ✅ Core functionality complete (set bonuses apply, legend displays, tests pass). Optional: visual feedback for threshold achievements.
   - Implement set bonus calculations, PreBattleModifiers plumbing, and the battle legend UI according to the plan with supporting tests/logs.

---

## Tier 2: Core Gameplay Features (4-8hrs each)

### Task 2: Replace random_device with SeededRng in Server
**Files**: `src/server/battle_api.cpp`, `src/server/async/systems/ProcessCommandQueueSystem.h`
- **Plan**: See `plans/server_rng_determinism_plan.md`
- **Issue**: Server code still uses `std::random_device` instead of `SeededRng` for deterministic RNG
- **Locations**:
  - `battle_api.cpp:195` - Battle seed generation
  - `ProcessCommandQueueSystem.h:213` - Command processing
- **Fix**: Replace `std::random_device` calls with `SeededRng::get()` calls
- **Verification Method**: Run the same battle twice with the same seed and compare outputs. Verify:
  - Order of effects is identical
  - Order of dishes entering combat is identical
  - All RNG-dependent behavior is identical
  - Any other deterministic aspects match
- **Validation Steps**:
  1. Replace random_device in battle_api.cpp with SeededRng
  2. Replace random_device in ProcessCommandQueueSystem.h with SeededRng
  3. Ensure seeds are properly set before use
  4. Create test that runs same battle twice with same seed and compares outputs
  5. **MANDATORY CHECKPOINT**: Build: `xmake`
  6. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  7. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  8. Verify server determinism works correctly (test with same seed produces same results)
  9. **MANDATORY CHECKPOINT**: Commit: `git commit -m "be - replace random_device with SeededRng in server code"`
  10. **Only after successful commit, proceed to Task 3**
- **Estimated**: 1-2 hours

### Task 3: Implement BattleReport Persistence
**Files**: Create/update `src/components/battle_report.h`, add JSON serialization
- **Plan**: See `plans/battle_report_persistence_plan.md`
- **Architecture**: Server-authoritative battle system
  - Server returns JSON: `{ seed, opponentId, outcomes[], events[] }`
  - Client receives JSON and uses seed to replay battle visually
  - Store reports locally for later replay
- **Components**: `BattleReport{ opponentId, seed, outcomes[], events[], receivedFromServer, timestamp }`
- **Storage**: Write to `output/battles/results/*.json` on Results screen
- **File Naming**: Same as server format: `YYYYMMDD_HHMMSS_<battle_id>.json`
- **When to Save**: Auto-save after every battle completes (on Results screen)
- **File Retention**: Keep only the last 20 battle reports (delete oldest when limit reached)
- **Future Enhancement**: Will add "History" button on main menu to show last 20 battles and allow replaying simulations
- **Note**: Server already saves results to `output/battles/results/` with format `YYYYMMDD_HHMMSS_<battle_id>.json`. Client should be able to load these files.
- **Fun Ideas**:
  - Battle history viewer showing past matches (future: main menu "History" button)
  - "Replay Last Battle" quick button
  - Share battle results (export JSON)
- **Validation Steps**:
  1. Create BattleReport component
  2. Add JSON serialization/deserialization (server format compatible)
  3. Integrate with Results screen to auto-save reports (format: `YYYYMMDD_HHMMSS_<battle_id>.json`)
  4. Implement file retention (keep last 20, delete oldest when limit reached)
  5. **MANDATORY CHECKPOINT**: Build: `make`
  6. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  7. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  8. Verify reports saved and can be loaded (check output/battles/results/ directory)
  9. **MANDATORY CHECKPOINT**: Commit: `git commit -m "implement BattleReport persistence with JSON serialization"`
  10. **Only after successful commit, proceed to Task 4**
- **Estimated**: 4-5 hours

---

## Tier 3: Enhanced Features & Polish (6-12hrs each)

### Task 4: Enhance Replay System
**Files**: Enhance `src/systems/ReplayControllerSystem.h`, create `src/systems/ReplayUISystem.h`
- **Plan**: See `plans/replay_system_enhancement_plan.md`
- **Current State**: Basic ReplayState exists, pause/play works
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
  4. **MANDATORY CHECKPOINT**: Build: `xmake`
  5. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  6. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  7. Verify can replay any saved battle, speed controls work (manual test)
  8. **MANDATORY CHECKPOINT**: Commit: `git commit -m "enhance replay system with speed controls and battle history"`
  9. **Only after successful commit, proceed to Task 5**
- **Estimated**: 4-6 hours

### Task 5: Implement Set Bonus Effects (Optional: Visual Feedback)
**Files**: Add visual feedback systems
- **Plan**: See `plans/set_bonus_system_plan.md`
- **Status**: ✅ Core functionality complete (set bonuses apply, legend displays, tests pass)
- **Remaining Work**: Optional visual feedback for threshold achievements
- **Current State**:
  - ✅ Set bonus definitions exist for all 10 cuisines
  - ✅ Bonuses apply correctly via `ApplySetBonusesSystem`
  - ✅ Battle synergy legend displays active synergies
  - ✅ Effect-based bonuses work (e.g., OnBiteTaken triggers)
  - ✅ Tests pass and verify functionality
- **Optional Enhancement**: Visual feedback when thresholds achieved
  - Celebration animation when hitting 4-piece or 6-piece bonus
  - Particle effects or glow around dishes
  - Sound effects (optional)
- **Validation Steps** (if implementing visual feedback):
  1. Add animation/particle system for threshold achievements
  2. Detect threshold crossing in `ApplySetBonusesSystem`
  3. Trigger visual feedback on threshold achievement
  4. **MANDATORY CHECKPOINT**: Build: `xmake`
  5. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  6. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  7. Verify visual feedback appears correctly (manual test)
  8. **MANDATORY CHECKPOINT**: Commit: `git commit -m "ui - add visual feedback for set bonus thresholds"`
  9. **Only after successful commit, proceed to Task 6**
- **Estimated**: 2-4 hours (if implementing visual feedback)

### Task 6: Enhanced Shop UX
**Files**: Update shop systems and UI
- **Plan**: See `plans/shop_ux_affordances_plan.md` (from Plan Creation Tasks #4)
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
  4. **MANDATORY CHECKPOINT**: Build: `xmake`
  5. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  6. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  7. Verify all UX improvements work correctly (manual test)
  8. **MANDATORY CHECKPOINT**: Commit: `git commit -m "ui - enhance shop UX with prices, drop highlights, and reroll cost"`
  9. **Only after successful commit, proceed to Task 7**
- **Estimated**: 3-4 hours

## Tier 4: Advanced Systems (10+ hours each)

### Task 10: Implement Status Effects
**Files**: Create status effect components, extend `EffectResolutionSystem.h`
- **Plan**: See `plans/status_effects_plan.md` (explicitly marked as "NEEDS FULL DESIGN PLAN" in task)
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
  5. **MANDATORY CHECKPOINT**: Build: `xmake`
  6. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  7. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  8. Verify status effects work correctly (manual test)
  9. **MANDATORY CHECKPOINT**: Commit: `git commit -m "implement status effects system"`
  10. **Only after successful commit, proceed to Task 11**
- **Estimated**: 6-8 hours

### Task 11: Implement Duration-Based Effects
**Files**: Extend `EffectResolutionSystem.h`
- **Plan**: See `plans/duration_based_effects_plan.md`
- **Goal**: Implement effects that last N courses (duration-based effects)
- **Implementation**:
  - Add duration tracking to effects
  - Effects expire after N courses
  - Track which course an effect was applied
  - Remove effects when duration expires
- **Validation Steps**:
  1. Extend EffectResolutionSystem with duration tracking
  2. Add course tracking for effects
  3. Implement expiration logic
  4. **MANDATORY CHECKPOINT**: Build: `xmake`
  5. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  6. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  7. Verify duration-based effects work correctly (manual test)
  8. **MANDATORY CHECKPOINT**: Commit: `git commit -m "implement duration-based effects"`
  9. **Only after successful commit, proceed to Task 12**
- **Estimated**: 4-6 hours

### Task 12: Implement Effect Chains and Dependencies
**Files**: Extend `EffectResolutionSystem.h`
- **Plan**: See `plans/effect_chains_plan.md`
- **Goal**: Implement effect chains where one effect can trigger another effect
- **Implementation**:
  - Add effect dependency tracking
  - Implement effect chains (effect A triggers effect B)
  - Handle conditional effects with complex logic
- **Fun Ideas**:
  - Chain reaction animations when effects trigger other effects
- **Validation Steps**:
  1. Design effect chain system
  2. Implement effect dependency tracking
  3. Add effect chain resolution logic
  4. **MANDATORY CHECKPOINT**: Build: `xmake`
  5. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  6. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  7. Verify effect chains work correctly (manual test)
  8. **MANDATORY CHECKPOINT**: Commit: `git commit -m "implement effect chains and dependencies"`
  9. **Only after successful commit, proceed to Task 13**
- **Estimated**: 6-8 hours

### Task 13: Visual Enhancements for Dish Merging
**Files**: Update rendering systems
- **Plan**: See `plans/dish_merging_visuals_plan.md`
- **Goal**: Add visual feedback for dish merging (merge system already implemented)
- **Note**: Dish merging is already implemented (2 level N dishes make 1 level N+1). This task is just visual polish.
- **Level Scaling**: Uses existing 2x per level system (level 2 = 2x, level 3 = 4x, level 4 = 8x). See `ComputeCombatStatsSystem.h` lines 95-104.
- **Merge Trigger**: Already implemented - user drags dish onto another dish of same type to merge. See `DropWhenNoLongerHeld.cpp` merge_dishes function.
- **Implementation**:
  - Add merge animation showing dishes combining
  - Add level-up sound effect
  - Add "Ready to merge!" indicator when you have 2/3 (visual hint)
  - Add level display on dishes (small number badge showing level)
- **Fun Ideas**:
  - Merge animation showing dishes combining
  - Level-up sound effect
  - "Ready to merge!" indicator when you have 2/3
  - Level display on dishes (small number badge)
- **Validation Steps**:
  1. Add merge animation
  2. Add level-up sound effect
  3. Add "Ready to merge!" visual indicator
  4. Add level display badge on dishes
  5. **MANDATORY CHECKPOINT**: Build: `xmake`
  6. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  7. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  8. Verify visual enhancements work correctly (manual test)
  9. **MANDATORY CHECKPOINT**: Commit: `git commit -m "ui - add visual enhancements for dish merging"`
  10. **Task complete - all tasks finished!**
- **Estimated**: 4-6 hours

---

## Tier 5: Tooltip Enhancements (Future Improvements)

### Tooltip Improvement Ideas

These are potential enhancements to the tooltip system that can be implemented as needed:

#### 1. Level Comparison & Progression Preview
- **Plan**: See `plans/level_comparison_tooltip_plan.md`
- Show stat/effect changes between current level and next level
- Example: "Level 2: +2 Freshness (Level 3: +3 Freshness)"
- Highlight which effects unlock at higher levels
- **Files**: `src/tooltip.h`
- **Estimated**: 2-3 hours

#### 2. Tags & Synergy Integration (Requires Task 2)
- Display dish tags (Course, Cuisine, Brand, etc.) in tooltips
- Show synergy progress: "Thai: 2/4 dishes (1 away from bonus)"
- Indicate which synergies this dish contributes to
- **Files**: `src/tooltip.h`, `src/systems/TooltipSystem.h`
- **Estimated**: 2-3 hours

#### 3. Combat Stats Preview
- Show calculated Zing/Body from flavor stats in tooltip
- Example: "Combat Stats: Zing: 8, Body: 12"
- Include level scaling: "Body: 12 (base 6 × level 2)"
- **Files**: `src/tooltip.h`
- **Estimated**: 1-2 hours

#### 4. Effect Formatting Improvements
- Group effects by trigger type (OnServe, OnBiteTaken, etc.)
- Better conditional formatting: "If adjacent has Freshness ≥ 2: +1 Spice"
- Visual separators between sections
- **Files**: `src/tooltip.h`
- **Estimated**: 2-3 hours

#### 5. Multi-Level Effect Preview
- Show all effects across all levels in one tooltip
- Indicate which effects unlock at which level
- Example: "Level 1: No effects | Level 2: +1 Freshness | Level 3: +2 Freshness + Chain"
- **Files**: `src/tooltip.h`
- **Estimated**: 3-4 hours

#### 6. Tooltip Positioning Enhancements
- Better edge detection (currently basic left/right/up/down)
- Multi-line width calculation (currently uses single-line width which is incorrect)
- Smart positioning to avoid UI elements
- **Files**: `src/systems/TooltipSystem.h`
- **Estimated**: 2-3 hours

#### 7. Rich Formatting Support
- Bold/italic for emphasis
- Icons for effect types
- Progress bars for synergy counts
- Better color coding for stat types
- **Files**: `src/tooltip.h`, `src/systems/TooltipSystem.h`
- **Estimated**: 4-6 hours

#### 8. Context-Aware Tooltips
- Shop context: Show price, purchase info, synergy hints
- Battle context: Show current combat stats with modifiers
- Inventory context: Show merge progress, level info
- **Files**: `src/tooltip.h`, `src/systems/TooltipSystem.h`
- **Estimated**: 3-4 hours

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
- Tier 1: Survivor carryover test (2-3 hours)
- Tier 2: Server determinism fixes (1-2 hours)

**Short Term (Next 2 Weeks)**:
- Tier 2: BattleReport persistence (4-5 hours)
- Tier 3: Replay enhancements and shop UX (7-10 hours)

**Medium Term (Next Month)**:
- Tier 3: Set bonuses, font standardization, text formatting, tooltip positioning (14-20 hours)
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
2. Then add server JSON deserialization (Task 3)
3. Finally add HTTP client for server communication (future task)
