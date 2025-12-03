# Replay System Enhancement Plan

## At-a-Glance
- **Sequence:** 19 / 21 — UX/system improvements required to make determinism + battle reports usable.
- **Objective:** Add in-battle replay controls, load saved BattleReports, and surface “Watch Again” flows.
- **Status:** Infrastructure exists (ReplayState, controller); UI + loading flows outstanding.
- **Dependencies:** Server/BattleReport persistence (Plan 20), MCP/automation (Plan 21), combat readiness (Plan 07/08).
- **Success Metrics:** Replays load from disk, controls responsive, telemetry shows usage, QA can validate determinism quickly.

## Work Breakdown Snapshot
|Phase|Scope|Key Tasks|Exit Criteria|
|---|---|---|---|
|1. Replay UI Layer|Build dedicated UI system|Create `ReplayUISystem`, render buttons, bind to ReplayState|UI toggles speed/pause reliably, matches spec|
|2. BattleReport Loader|Load JSON + hydrate state|Implement `load_battle_report`, integrate with Results + CLI|Can replay any saved battle via UI or CLI|
|3. Results Integration|“Watch Again” entry point|Add button, file picker, error handling|UX approved, flow covered by tests|
|4. QA + Telemetry|Testing + metrics|Automated tests, scriptable pipelines, telemetry for replay usage + speed selection|Dashboards + QA scripts verifying parity|

## Overview

This plan details enhancing the replay system with speed controls UI and BattleReport loading functionality.

## Goal

Enhance replay system to:
- Add UI buttons for speed controls (0.5x, 1x, 2x, 4x)
- Load BattleReport from saved JSON files
- Integrate with Results screen for "Watch Again" functionality

## Current State

### Existing Replay Infrastructure

- **`ReplayState` component** exists with:
  - `paused` flag
  - `timeScale` (speed multiplier)
  - `seed`, `playerJsonPath`, `opponentJsonPath`
  - `currentFrame`, `totalFrames`

- **`ReplayControllerSystem`** exists with:
  - Pause/play functionality (Space key)
  - Speed controls partially exist (1x, 2x, 4x via number keys)
  - Restart functionality (R key)
  - Time scale updates: `rs.targetMs += static_cast<int64_t>(dt * 1000.0f * rs.timeScale)`

### Missing Features

- ⚠️ UI buttons for speed controls - **PARTIALLY IMPLEMENTED** (speed controls exist in `ui_systems.cpp` lines 714-745, but they control battle speed, not replay `timeScale`. Speed controls are integrated into battle screen UI, not a separate ReplayUISystem)
- ❌ BattleReport loading from JSON files - **NOT IMPLEMENTED**
- ❌ Integration with Results screen - **NOT IMPLEMENTED**
- ❌ "Watch Again" button - **NOT IMPLEMENTED**

## Current Implementation Status

### What Exists:
- ✅ `ReplayControllerSystem.h` with keyboard controls (Space, R, 1-4 keys)
- ✅ Speed controls exist in battle UI (`ui_systems.cpp` lines 714-745) but control `render_backend::timing_speed_scale`, not `ReplayState.timeScale`
- ✅ `ReplayState` component exists with `timeScale`, `paused`, `active` fields
- ✅ Comment in `main.cpp` line 249: "ReplayUISystem is now integrated into ScheduleMainMenuUI::battle_screen"

### What's Missing:
- ❌ Separate `ReplayUISystem` for replay-specific controls
- ❌ BattleReport loading functionality (`load_battle_report()` function)
- ❌ "Watch Again" button on Results screen
- ⚠️ Speed controls need to be connected to `ReplayState.timeScale` when in replay mode (currently they control general battle speed)

## Implementation Details

### 1. Create ReplayUISystem

**File**: `src/systems/ReplayUISystem.h`

**Purpose**: Render UI buttons for replay controls during battle replay.

**Features**:
- Speed control buttons: 0.5x, 1x, 2x, 4x
- Pause/Play button
- Restart button (optional)
- Only visible when `ReplayState.active == true` and on Battle screen

**UI Layout**:
- Position: Top-right or bottom-right corner
- Button style: Match existing UI button style
- Active button highlight: Show current speed/timeScale

**Implementation**:
```cpp
struct ReplayUISystem : afterhours::System<ReplayState> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &, ReplayState &rs, float) override {
    if (!rs.active) return;
    
    // Render speed control buttons
    render_speed_buttons(rs);
    render_pause_play_button(rs);
  }
  
private:
  void render_speed_buttons(ReplayState &rs);
  void render_pause_play_button(ReplayState &rs);
};
```

### 2. Speed Control Buttons

**Buttons**: 0.5x, 1x, 2x, 4x

**Behavior**:
- Clicking a button sets `rs.timeScale` to corresponding value
- Active button is highlighted
- Update happens immediately (no frame delay)

**Values**:
- 0.5x → `timeScale = 0.5f`
- 1x → `timeScale = 1.0f`
- 2x → `timeScale = 2.0f`
- 4x → `timeScale = 4.0f`

### 3. BattleReport Loading

**Integration with Task 3**: Use `BattleReport` component from battle report persistence.

**Functionality**:
- Load JSON file from `output/battles/results/`
- Parse into `BattleReport` component
- Extract `seed`, `playerJsonPath`, `opponentJsonPath`
- Initialize `ReplayState` with loaded data
- Set `BattleLoadRequest` to trigger battle load

**Implementation**:
```cpp
void load_battle_report(const std::string& filepath) {
  // Load JSON file
  BattleReport report;
  report.from_json_file(filepath);
  
  // Get or create ReplayState
  auto replay_entity = afterhours::EntityHelper::get_singleton<ReplayState>();
  ReplayState &rs = replay_entity.get().get<ReplayState>();
  
  // Set replay data
  rs.seed = report.seed;
  rs.playerJsonPath = report.playerJsonPath; // May need to reconstruct
  rs.opponentJsonPath = report.opponentJsonPath; // May need to reconstruct
  rs.active = true;
  rs.paused = false;
  rs.timeScale = 1.0f;
  
  // Set BattleLoadRequest
  auto battle_request = afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
  BattleLoadRequest &request = battle_request.get().get<BattleLoadRequest>();
  request.playerJsonPath = rs.playerJsonPath;
  request.opponentJsonPath = rs.opponentJsonPath;
  request.loaded = false;
  
  // Navigate to Battle screen
  GameStateManager::get().to_battle();
}
```

**Note**: May need to reconstruct JSON paths if they're not stored in BattleReport. Consider storing full paths or reconstructing from seed/opponentId.

### 4. Results Screen Integration

**"Watch Again" Button**:
- Add button on Results screen
- Loads most recent battle report
- Navigates to Battle screen with replay active

**Implementation**:
- Find most recent JSON file in `output/battles/results/`
- Load and initialize replay
- Transition to Battle screen

### 5. UI Button Rendering

**Use Existing UI System**: Follow patterns from `src/ui/ui_systems.cpp`.

**Button Creation**:
- Use `button_labeled()` or similar helper
- Position buttons in corner (top-right or bottom-right)
- Style matches existing UI

**Example**:
```cpp
void render_speed_buttons(ReplayState &rs) {
  float button_x = screen_width - 200;
  float button_y = 50;
  float button_spacing = 50;
  
  // 0.5x button
  bool is_05x = (rs.timeScale == 0.5f);
  if (button_labeled("0.5x", button_x, button_y, is_05x)) {
    rs.timeScale = 0.5f;
  }
  
  // 1x button
  bool is_1x = (rs.timeScale == 1.0f);
  if (button_labeled("1x", button_x, button_y + button_spacing, is_1x)) {
    rs.timeScale = 1.0f;
  }
  
  // 2x button
  bool is_2x = (rs.timeScale == 2.0f);
  if (button_labeled("2x", button_x, button_y + button_spacing * 2, is_2x)) {
    rs.timeScale = 2.0f;
  }
  
  // 4x button
  bool is_4x = (rs.timeScale == 4.0f);
  if (button_labeled("4x", button_x, button_y + button_spacing * 3, is_4x)) {
    rs.timeScale = 4.0f;
  }
}
```

## Removed Features (Not Needed)

- **Seek to course**: Skip this feature (not in scope)
- **Event log viewer**: Not needed (removed from requirements)

## Test Cases

### Test 1: Speed Control Buttons

**Setup**:
- Start battle replay
- Click speed control buttons

**Expected**:
- Buttons visible during replay
- Clicking button changes `ReplayState.timeScale`
- Battle speed changes accordingly
- Active button is highlighted

### Test 2: Load BattleReport

**Setup**:
- Save a battle report (from Task 3)
- Load report from file

**Expected**:
- Report loads successfully
- `ReplayState` initialized with correct data
- Battle starts with correct seed and teams

### Test 3: Results Screen "Watch Again"

**Setup**:
- Complete a battle
- Click "Watch Again" on Results screen

**Expected**:
- Most recent battle report loads
- Battle replay starts
- Speed controls available

## Validation Steps

1. **Create ReplayUISystem**
   - Create `src/systems/ReplayUISystem.h`
   - Implement speed control button rendering
   - Register system in system registry

2. **Add BattleReport Loading**
   - Implement `load_battle_report()` function
   - Integrate with `ReplayState` initialization
   - Test loading from JSON files

3. **Integrate with Results Screen**
   - Add "Watch Again" button
   - Implement load and replay logic
   - Test navigation flow

4. **MANDATORY CHECKPOINT**: Build
   - Run `make` - code must compile successfully

5. **MANDATORY CHECKPOINT**: Run Headless Tests
   - Run `./scripts/run_all_tests.sh` (ALL must pass)

6. **MANDATORY CHECKPOINT**: Run Non-Headless Tests
   - Run `./scripts/run_all_tests.sh -v` (ALL must pass)

7. **Verify Replay Functionality**
   - Test speed controls work (manual test)
   - Test loading saved battles (manual test)
   - Test "Watch Again" button (manual test)

8. **MANDATORY CHECKPOINT**: Commit
   - `git commit -m "enhance replay system with speed controls and battle history"`

9. **Only after successful commit, proceed to Task 5**

## Edge Cases

1. **No Battle Reports**: "Watch Again" button disabled or shows message
2. **Invalid JSON**: Handle parse errors gracefully
3. **Missing Files**: Handle file not found errors
4. **Replay During Active Battle**: Ensure replay state doesn't interfere with active battles

## Future Enhancements

- Timeline scrubber showing battle progress
- Slow-motion replay for epic moments
- Battle highlights (auto-detect exciting moments)
- Replay from specific course (seek functionality)

## Outstanding Questions
1. **BattleReport Source:** Should “Watch Again” only load the last match, or provide a file picker/history UI?
2. **Performance:** Do we stream large reports to avoid loading the full JSON into memory at once?
3. **Automation:** What CLI hooks does QA need to trigger replays headlessly for regression tests?
4. **Telemetry:** Which replay usage metrics (button clicks, speed selection, watch rates) should be piped into dashboards?
5. **Error UX:** How should we surface missing/invalid BattleReports to players without causing confusion?

## Success Criteria

- ✅ Speed control buttons (0.5x, 1x, 2x, 4x) visible and functional
- ✅ BattleReport loading from JSON files works
- ✅ "Watch Again" button on Results screen functional
- ✅ Replay system integrates with battle report persistence
- ✅ All tests pass in headless and non-headless modes

## Estimated Time

4-6 hours:
- 2 hours: ReplayUISystem and speed control buttons
- 1.5 hours: BattleReport loading integration
- 1 hour: Results screen integration
- 30 min: Testing and validation

