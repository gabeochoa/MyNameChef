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
- **Goal**: Test that dishes that survive a course carry over to the next course correctly
- **Test Cases**:
  - Dish with remaining Body after opponent finishes should retarget next opponent
  - Transform position should update correctly on retarget
  - Multiple survivors should handle correctly
  - Battle should only complete when one team has no active dishes
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

## Tier 2: Core Gameplay Features (4-8hrs each)

### Task 2: Implement Tags and SynergyCountingSystem
**Files**: Create tag components, `src/systems/SynergyCountingSystem.h`
- **Components Needed**:
  - `CourseTag` (Appetizer, Soup, Fish, Meat, Drink, PalateCleanser, Dessert, Mignardise)
  - `CuisineTag` (Thai, Italian, Japanese, Mexican, French, etc.)
  - `BrandTag` (McDonalds, LocalFarm, StreetFood, etc.)
  - `DietaryTag` (Vegan, GlutenFree, Halal, Kosher, Keto)
  - `DishArchetypeTag` (Bread, Dairy, Beverage, Wine, Coffee, Tea, Side, Sauce)
  - `SynergyCounts` singleton (tracks counts per tag type)
- **System**: Scan inventory on Shop screen, count tags, compute set thresholds (2/4/6)
- **UI**: Update tooltips to show tags and current synergy counts
- **Fun Ideas**:
  - Visual indicators on dishes showing active synergies
  - Synergy meter that fills as you add matching dishes
  - Tooltip shows "2/4 Thai dishes" with progress bar
- **Validation Steps**:
  1. Create tag component files (`src/components/course_tag.h`, etc.)
  2. Add tags to dish definitions in `dish_types.cpp`
  3. Create `SynergyCountingSystem` to scan and count
  4. Update tooltip system to display tags and counts
  5. **MANDATORY CHECKPOINT**: Build: `xmake`
  6. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  7. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  8. Verify tags display, counts update correctly, tooltips show synergies (manual test)
  9. **MANDATORY CHECKPOINT**: Commit: `git commit -m "implement tags and SynergyCountingSystem"`
  10. **Only after successful commit, proceed to Task 3**
- **Estimated**: 4-6 hours

### Task 3: Replace random_device with SeededRng in Server
**Files**: `src/server/battle_api.cpp`, `src/server/async/systems/ProcessCommandQueueSystem.h`
- **Issue**: Server code still uses `std::random_device` instead of `SeededRng` for deterministic RNG
- **Locations**:
  - `battle_api.cpp:195` - Battle seed generation
  - `ProcessCommandQueueSystem.h:213` - Command processing
- **Fix**: Replace `std::random_device` calls with `SeededRng::get()` calls
- **Validation Steps**:
  1. Replace random_device in battle_api.cpp with SeededRng
  2. Replace random_device in ProcessCommandQueueSystem.h with SeededRng
  3. Ensure seeds are properly set before use
  4. **MANDATORY CHECKPOINT**: Build: `xmake`
  5. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  6. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  7. Verify server determinism works correctly (test with same seed produces same results)
  8. **MANDATORY CHECKPOINT**: Commit: `git commit -m "be - replace random_device with SeededRng in server code"`
  9. **Only after successful commit, proceed to Task 4**
- **Estimated**: 1-2 hours

### Task 4: Implement BattleReport Persistence
**Files**: Create/update `src/components/battle_report.h`, add JSON serialization
- **Architecture**: Server-authoritative battle system
  - Server returns JSON: `{ seed, opponentId, outcomes[], events[] }`
  - Client receives JSON and uses seed to replay battle visually
  - Store reports locally for later replay
- **Components**: `BattleReport{ opponentId, seed, outcomes[], events[], receivedFromServer, timestamp }`
- **Storage**: Write to `output/battles/results/*.json` on Results screen
- **Fun Ideas**:
  - Battle history viewer showing past matches
  - "Replay Last Battle" quick button
  - Share battle results (export JSON)
- **Validation Steps**:
  1. Create BattleReport component
  2. Add JSON serialization/deserialization (server format compatible)
  3. Integrate with Results screen to save reports
  4. **MANDATORY CHECKPOINT**: Build: `xmake`
  5. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  6. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  7. Verify reports saved and can be loaded (check output/battles/results/ directory)
  8. **MANDATORY CHECKPOINT**: Commit: `git commit -m "implement BattleReport persistence with JSON serialization"`
  9. **Only after successful commit, proceed to Task 5**
- **Estimated**: 4-5 hours

---

## Tier 3: Enhanced Features & Polish (6-12hrs each)

### Task 5: Verify/Fix Deterministic Turn Alternation
**Files**: `src/systems/ResolveCombatTickSystem.h`
- **Status**: ✅ Verified - No static variables found, system uses deterministic simultaneous attack cadence
- **Finding**: ResolveCombatTickSystem uses a cadence-based approach with simultaneous attacks (both dishes attack at the same time), which is fully deterministic. No static `player_turn` variable exists.
- **Note**: System is already deterministic - no changes needed. This task can be considered complete.
- **Estimated**: 0 hours (verification only)

### Task 6: Enhance Replay System
**Files**: Enhance `src/systems/ReplayControllerSystem.h`, create `src/systems/ReplayUISystem.h`
- **Current State**: Basic ReplayState exists, pause/play works
- **Enhancements Needed**:
  - Speed controls (0.5x/1x/2x/4x) - partially exists, needs UI
  - Seek to course (jump to specific course in battle)
  - Load BattleReport from saved JSON files
  - Event log viewer (show all events in battle)
- **Fun Ideas**:
  - Timeline scrubber showing battle progress
  - Slow-motion replay for epic moments
  - "Watch Again" button on Results screen
  - Battle highlights (auto-detect exciting moments)
- **Validation Steps**:
  1. Enhance ReplayControllerSystem with seek functionality
  2. Create ReplayUISystem for controls (speed, seek, pause)
  3. Add BattleReport loading from JSON
  4. Integrate with Results screen
  5. **MANDATORY CHECKPOINT**: Build: `xmake`
  6. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  7. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  8. Verify can replay any saved battle, seek works, speed controls work (manual test)
  9. **MANDATORY CHECKPOINT**: Commit: `git commit -m "enhance replay system with seek, speed controls, and battle history"`
  10. **Only after successful commit, proceed to Task 7**
- **Estimated**: 6-8 hours

### Task 7: Implement Set Bonus Effects
**Files**: Extend `ApplyPairingsAndClashesSystem.h`, create set bonus definitions
- **Goal**: Apply bonuses when synergy thresholds are reached (2/4/6)
- **Examples**:
  - 2 Thai dishes: +1 spice cap, +0.5 freshness
  - 4 Thai dishes: +2 spice cap, +1 freshness, unlock "Herb Garden" trigger
  - 6 Thai dishes: +3 spice cap, +2 freshness, +1 umami, "Herb Garden" enhanced
- **Implementation**:
  - Read SynergyCounts singleton
  - Apply bonuses to PreBattleModifiers based on thresholds
  - Visual feedback when thresholds reached
  - Display active synergies in battle as totals in a legend box (shows current synergy counts and thresholds)
- **Fun Ideas**:
  - Celebration animation when hitting 4-piece or 6-piece bonus
  - Set bonus tooltips showing what you'll get
  - "Almost there!" indicators (1 away from next threshold)
  - Battle synergy legend box showing active synergies with counts (e.g., "American: 4/6", "Bread: 2/4")
- **Validation Steps**:
  1. Create set bonus definition system (JSON or code)
  2. Extend ApplyPairingsAndClashesSystem to read SynergyCounts
  3. Apply bonuses to PreBattleModifiers
  4. Add visual feedback for threshold achievements
  5. **MANDATORY CHECKPOINT**: Build: `xmake`
  6. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  7. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  8. Verify set bonuses apply correctly, visual feedback works (manual test)
  9. **MANDATORY CHECKPOINT**: Commit: `git commit -m "implement set bonus effects with threshold system"`
  10. **Only after successful commit, proceed to Task 8**
- **Estimated**: 6-8 hours

### Task 8: Enhanced Shop UX
**Files**: Update shop systems and UI
- **Features**:
  - Show price on shop items (hover or always visible)
  - "Frozen" badge on frozen items
  - Faint highlight on legal drop targets
  - Reroll cost display (current cost, increment)
  - Freeze toggle with visual feedback
- **Fun Ideas**:
  - Item rarity glow (tier 1-5 different colors)
  - "Recommended" tag for dishes that synergize with current team
  - Shop refresh animation
  - Sound effects for purchase/sell
- **Validation Steps**:
  1. Add price display to shop items
  2. Add freeze badge and toggle
  3. Add drop target highlighting
  4. Update reroll cost display
  5. **MANDATORY CHECKPOINT**: Build: `xmake`
  6. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  7. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  8. Verify all UX improvements work correctly (manual test)
  9. **MANDATORY CHECKPOINT**: Commit: `git commit -m "ui - enhance shop UX with prices, freeze badges, and drop highlights"`
  10. **Only after successful commit, proceed to Task 9**
- **Estimated**: 4-6 hours

### Task 9: Update Font Across the Game
**Files**: `src/font_info.h`, `src/render_utils.h`, all UI rendering systems
- **Goal**: Standardize and improve font usage across all game screens
- **Implementation**:
  - Review current font usage and identify inconsistencies
  - Select appropriate font(s) for different UI contexts (UI text, tooltips, combat text, etc.)
  - Update font loading and management system
  - Replace hardcoded font references with centralized font system
  - Ensure font rendering is consistent across all screens (Shop, Battle, Results, etc.)
- **Fun Ideas**:
  - Different fonts for different contexts (e.g., decorative font for titles, readable font for tooltips)
  - Font size scaling for different screen resolutions
  - Font fallback system for missing glyphs
- **Validation Steps**:
  1. Audit current font usage across all systems
  2. Update font_info.h with standardized font definitions
  3. Replace hardcoded font references with centralized system
  4. Update all UI rendering systems to use new font system
  5. **MANDATORY CHECKPOINT**: Build: `xmake`
  6. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  7. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  8. Verify fonts render correctly across all screens (manual test)
  9. **MANDATORY CHECKPOINT**: Commit: `git commit -m "ui - standardize font usage across game"`
  10. **Only after successful commit, proceed to Task 10**
- **Estimated**: 3-5 hours

### Task 10: Better System for Coloring Text
**Files**: Create `src/ui/text_color.h`, update all text rendering systems
- **Goal**: Create a centralized, maintainable system for text coloring throughout the game
- **Implementation**:
  - Create text color system with semantic color names (e.g., `TEXT_COLOR_PRIMARY`, `TEXT_COLOR_SECONDARY`, `TEXT_COLOR_WARNING`, `TEXT_COLOR_SUCCESS`, etc.)
  - Define color palette for different contexts (UI, combat, tooltips, etc.)
  - Replace hardcoded color values with semantic color system
  - Support for color themes/variants
  - Ensure consistent color usage across all screens
- **Fun Ideas**:
  - Color-coded stat changes (green for positive, red for negative)
  - Contextual colors (e.g., dish rarity colors, synergy colors)
  - Smooth color transitions for dynamic text
  - Accessibility: ensure sufficient contrast ratios
- **Validation Steps**:
  1. Create text_color.h with color system and semantic names
  2. Define color palette for different contexts
  3. Replace hardcoded colors with semantic color system across all UI
  4. Update text rendering utilities to use new color system
  5. **MANDATORY CHECKPOINT**: Build: `xmake`
  6. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  7. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  8. Verify text colors are consistent and appropriate across all screens (manual test)
  9. **MANDATORY CHECKPOINT**: Commit: `git commit -m "ui - implement centralized text coloring system"`
  10. **Only after successful commit, proceed to Task 11**
- **Estimated**: 3-4 hours

### Task 11: Fix Tooltip Offscreen Positioning
**Files**: `src/systems/TooltipSystem.h`
- **Goal**: Ensure tooltips are always fully visible on screen by repositioning them when they would go offscreen
- **Implementation**:
  - Detect when tooltip box would extend beyond screen boundaries (left, right, top, bottom)
  - Calculate tooltip dimensions (width and height) including all text content
  - Reposition tooltip to fit within screen bounds:
    - If tooltip extends past right edge: shift left
    - If tooltip extends past left edge: shift right
    - If tooltip extends past bottom edge: move above cursor/target
    - If tooltip extends past top edge: move below cursor/target
  - Ensure entire tooltip box is visible and readable
  - Handle edge cases (very large tooltips, screen corners)
- **Fun Ideas**:
  - Smooth animation when tooltip repositions
  - Visual indicator when tooltip is repositioned (subtle fade)
  - Smart positioning that prefers keeping tooltip near cursor when possible
- **Validation Steps**:
  1. Update TooltipSystem to calculate tooltip box dimensions accurately
  2. Add screen boundary detection logic
  3. Implement repositioning algorithm (check all edges)
  4. Test with tooltips near screen edges (all four corners)
  5. Test with very long tooltip text that would overflow
  6. **MANDATORY CHECKPOINT**: Build: `xmake`
  7. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  8. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  9. Verify tooltips never go offscreen, all text is readable (manual test at screen edges)
  10. **MANDATORY CHECKPOINT**: Commit: `git commit -m "ui - fix tooltip offscreen positioning"`
  11. **Only after successful commit, proceed to Task 12**
- **Estimated**: 2-3 hours

---

## Tier 4: Advanced Systems (10+ hours each)

### Task 12: Advanced Effect System Extensions
**Files**: Extend `EffectResolutionSystem.h`, create status effect components
- **Advanced Features**:
  - Status effects (PalateCleanser, Heat, SweetTooth, Fatigue)
  - Duration-based effects (effects that last N courses)
  - Effect chains and dependencies
  - Conditional effects with complex logic
- **Fun Ideas**:
  - Status effect icons on dishes (small badges)
  - Effect tooltips explaining what each status does
  - Chain reaction animations when effects trigger other effects
- **Validation Steps**:
  1. Create status effect components
  2. Extend EffectResolutionSystem with duration tracking
  3. Implement effect chains
  4. Add visual indicators for status effects
  5. **MANDATORY CHECKPOINT**: Build: `xmake`
  6. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  7. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  8. Verify complex effects work correctly, no edge cases (extensive manual testing)
  9. **MANDATORY CHECKPOINT**: Commit: `git commit -m "extend EffectResolutionSystem with status effects and duration tracking"`
  10. **Only after successful commit, proceed to Task 13**
- **Estimated**: 10-15 hours

### Task 13: Combine Duplicates into Levels
**Files**: Create `src/systems/CombineDuplicatesSystem.h`, update level system
- **Goal**: When 3 of the same DishType are in inventory, merge into one entity with Level+1
- **Implementation**:
  - Detect 3+ duplicates in inventory
  - Merge into single entity with boosted stats/cost
  - Remove the other entities
  - Visual feedback (merge animation)
- **Fun Ideas**:
  - Merge animation showing dishes combining
  - Level-up sound effect
  - "Ready to merge!" indicator when you have 2/3
  - Level display on dishes (small number badge)
- **Validation Steps**:
  1. Create CombineDuplicatesSystem
  2. Detect duplicates in inventory
  3. Merge logic with stat/cost scaling
  4. Add merge animation
  5. **MANDATORY CHECKPOINT**: Build: `xmake`
  6. **MANDATORY CHECKPOINT**: Run headless tests: `./scripts/run_all_tests.sh` (ALL must pass)
  7. **MANDATORY CHECKPOINT**: Run non-headless tests: `./scripts/run_all_tests.sh -v` (ALL must pass)
  8. Verify merging works correctly, stats scale properly (manual test)
  9. **MANDATORY CHECKPOINT**: Commit: `git commit -m "implement combine duplicates system with level scaling"`
  10. **Task complete - all tasks finished!**
- **Estimated**: 8-10 hours

---

## Tier 5: Tooltip Enhancements (Future Improvements)

### Tooltip Improvement Ideas

These are potential enhancements to the tooltip system that can be implemented as needed:

#### 1. Level Comparison & Progression Preview
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
- Tier 1: Survivor carryover test, server determinism fixes (3-5 hours)
- Tier 2: Start with Tags/Synergy system (foundation for other features)

**Short Term (Next 2 Weeks)**:
- Tier 2: Complete tags, tier 1 effects, and BattleReport persistence (12-17 hours)
- Tier 3: Start replay enhancements and shop UX (10-14 hours)

**Medium Term (Next Month)**:
- Tier 3: Complete set bonuses, replay system, shop UX (16-22 hours)
- Tier 4: Start advanced effects and combine system (18-25 hours)

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
2. Then add server JSON deserialization (Task 5)
3. Finally add HTTP client for server communication (future task)
