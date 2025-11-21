# Battle Report Persistence Plan

## Overview

This plan details implementing `BattleReport` component with JSON serialization to persist battle results locally for replay functionality.

## Goal

Implement battle report persistence system that:
- Stores battle results in JSON format (compatible with server format)
- Auto-saves reports after each battle completes
- Manages file retention (keep last 20 reports)
- Enables future replay functionality

## Current State

### Server Format

Server already saves results to `output/battles/results/` with format:
- Filename: `YYYYMMDD_HHMMSS_<battle_id>.json`
- JSON structure:
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

### Missing Components

- `BattleReport` component doesn't exist
- No JSON serialization/deserialization
- No auto-save on Results screen
- No file retention logic

## Architecture

### Component Design

**File**: `src/components/battle_report.h`

```cpp
struct BattleReport : afterhours::BaseComponent {
  std::string opponentId;
  uint64_t seed;
  std::vector<CourseOutcome> outcomes;
  std::vector<BattleEvent> events;
  bool receivedFromServer = false;
  std::chrono::system_clock::time_point timestamp;
  std::string filePath; // Path to saved JSON file
};
```

### JSON Serialization

Use existing JSON library (likely `nlohmann/json` or similar based on codebase patterns).

**Serialization**:
- Convert `BattleReport` to JSON matching server format
- Include all required fields: `seed`, `opponentId`, `outcomes[]`, `events[]`
- Add optional metadata: `timestamp`, `receivedFromServer`

**Deserialization**:
- Load JSON from file
- Parse into `BattleReport` component
- Validate required fields exist

## Implementation Details

### 1. Create BattleReport Component

**File**: `src/components/battle_report.h`

- Define `BattleReport` struct with all required fields
- Include helper methods: `to_json()`, `from_json()`, `get_filename()`
- Use existing `BattleResult::CourseOutcome` if available, or define new struct

### 2. JSON Serialization

**Approach**: Use existing JSON library in codebase.

**Functions**:
- `std::string BattleReport::to_json_string() const` - Serialize to JSON string
- `void BattleReport::from_json_string(const std::string& json)` - Deserialize from JSON string
- `std::string BattleReport::get_filename() const` - Generate filename: `YYYYMMDD_HHMMSS_<battle_id>.json`

### 3. Auto-Save on Results Screen

**System**: Create `SaveBattleReportSystem` or extend existing Results screen system.

**Trigger**: When Results screen is displayed and battle is complete.

**Logic**:
1. Check if `BattleResult` singleton exists
2. Extract battle data (seed, outcomes, events, opponentId)
3. Create `BattleReport` component
4. Serialize to JSON
5. Generate filename with timestamp
6. Write to `output/battles/results/` directory
7. Store file path in `BattleReport.filePath`
8. Apply file retention (delete oldest if > 20 files)

### 4. File Retention

**Logic**:
- On save, check `output/battles/results/` directory
- Count existing `.json` files
- If count >= 20:
  - Sort files by modification time (oldest first)
  - Delete oldest files until count < 20
  - Keep newest 20 files

**Implementation**:
- Use filesystem library (C++17 `std::filesystem` or platform-specific)
- List files matching pattern `*.json` in `output/battles/results/`
- Sort by `last_write_time()`
- Delete oldest files

### 5. Directory Management

**Ensure directory exists**:
- On first save, create `output/battles/results/` if it doesn't exist
- Use `std::filesystem::create_directories()` or equivalent

## Integration Points

### Results Screen

**Location**: Results screen rendering/system

**Integration**:
- After battle completes and Results screen displays
- Extract data from `BattleResult` singleton
- Create and save `BattleReport`

### BattleResult Component

**Check existing structure**:
- Review `src/components/battle_result.h`
- Reuse `CourseOutcome` and event structures if compatible
- Map `BattleResult` data to `BattleReport` format

## File Format Details

### Filename Format

`YYYYMMDD_HHMMSS_<battle_id>.json`

**Example**: `20240115_143022_abc123.json`

**Generation**:
- Use `std::chrono::system_clock::now()` for timestamp
- Format: `%Y%m%d_%H%M%S` (e.g., `20240115_143022`)
- Battle ID: Use `opponentId` or generate unique ID (e.g., first 6 chars of seed as hex)

### JSON Structure

Match server format exactly:

```json
{
  "seed": 123456789,
  "opponentId": "opponent_abc123",
  "outcomes": [
    {
      "slotIndex": 1,
      "winner": "Player",
      "ticks": 3
    }
  ],
  "events": [
    {
      "t_ms": 0,
      "course": 1,
      "type": "Enter",
      "side": "Player",
      "entityId": 101
    }
  ]
}
```

## Test Cases

### Test 1: Auto-Save After Battle

**Setup**:
- Complete a battle
- Navigate to Results screen

**Expected**:
- `BattleReport` JSON file created in `output/battles/results/`
- Filename matches format `YYYYMMDD_HHMMSS_*.json`
- File contains valid JSON with all required fields

### Test 2: File Retention

**Setup**:
- Create 25 battle reports (more than 20 limit)

**Expected**:
- Only 20 most recent files remain
- Oldest 5 files deleted
- Newest files preserved

### Test 3: JSON Serialization/Deserialization

**Setup**:
- Create `BattleReport` with sample data
- Serialize to JSON
- Deserialize from JSON

**Expected**:
- Deserialized data matches original
- All fields preserved correctly

### Test 4: Load Server-Generated Reports

**Setup**:
- Place server-generated JSON file in `output/battles/results/`
- Load file

**Expected**:
- File loads successfully
- `BattleReport` component created with correct data
- Compatible with server format

## Validation Steps

1. **Create BattleReport Component**
   - Define struct in `src/components/battle_report.h`
   - Include required fields and helper methods

2. **Add JSON Serialization**
   - Implement `to_json_string()` and `from_json_string()`
   - Test serialization/deserialization

3. **Integrate with Results Screen**
   - Create `SaveBattleReportSystem` or extend existing system
   - Trigger save when Results screen displays
   - Extract data from `BattleResult` singleton

4. **Implement File Retention**
   - Add logic to count and delete oldest files
   - Test with > 20 files

5. **MANDATORY CHECKPOINT**: Build
   - Run `make` - code must compile successfully

6. **MANDATORY CHECKPOINT**: Run Headless Tests
   - Run `./scripts/run_all_tests.sh` (ALL must pass)

7. **MANDATORY CHECKPOINT**: Run Non-Headless Tests
   - Run `./scripts/run_all_tests.sh -v` (ALL must pass)

8. **Verify Reports Saved**
   - Complete a battle
   - Check `output/battles/results/` directory
   - Verify JSON file exists and is valid

9. **MANDATORY CHECKPOINT**: Commit
   - `git commit -m "implement BattleReport persistence with JSON serialization"`

10. **Only after successful commit, proceed to Task 4**

## Edge Cases

1. **Directory Doesn't Exist**: Create directory on first save
2. **File Write Failure**: Log error, don't crash
3. **Invalid JSON**: Validate before saving, handle parse errors on load
4. **Concurrent Saves**: Use file locking or atomic operations if needed
5. **Disk Full**: Handle write failures gracefully

## Future Enhancements

- "History" button on main menu to show last 20 battles
- "Replay Last Battle" quick button on Results screen
- Battle history viewer with filters/search
- Share battle results (export JSON)
- Battle highlights (auto-detect exciting moments)

## Success Criteria

- ✅ `BattleReport` component created with all required fields
- ✅ JSON serialization/deserialization works correctly
- ✅ Reports auto-save after each battle completes
- ✅ File retention keeps only last 20 reports
- ✅ JSON format matches server format
- ✅ All tests pass in headless and non-headless modes

## Estimated Time

4-5 hours:
- 1 hour: Component and JSON serialization
- 1 hour: Auto-save integration
- 1 hour: File retention logic
- 1 hour: Testing and validation
- 30 min: Edge case handling

