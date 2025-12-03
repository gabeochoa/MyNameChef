# MCP Game Control Integration Plan

## At-a-Glance
- **Sequence:** 21 / 21 — final automation layer enabling Cursor/AI control over the client via MCP.
- **Mission:** Expose deterministic game control (input injection + state observation) through an MCP server so automated workflows/tests can drive the game.
- **Status:** Architecture + requirements documented; implementation pending battle server (Plan 20) progress.
- **Success Metrics:** MCP clients can launch the game, navigate UI, run tests, and verify state without manual intervention.

## Work Breakdown Snapshot
|Phase|Scope|Key Tasks|Exit Criteria|
|---|---|---|---|
|1. Game-side MCP Server|Stdio transport + command processing|Implement `MCPServer`, `MCPServerSystem`, input/state helpers, non-blocking IO|Game responds to JSON commands, processes inputs deterministically|
|2. Python MCP Adapter|Bridge Cursor ↔ game|Build Python MCP server, define tools (get_state, click_button, send_input, etc.)|Cursor can call tools; errors surfaced cleanly|
|3. Configuration + Security|Configs, flags, CORS|Add config file, `--mcp` flag, sanitize logs, add CORS headers|MCP mode opt-in, safe defaults|
|4. Test Observation|Validate automated tests|Implement observe_test_execution tool, integrate with game’s test mode|AI can watch/validate tests and report mismatches|

## Overview

Set up MCP (Model Context Protocol) integration so Cursor AI can control the game client (`my_name_chef.exe`) during interactive runs. The MCP server will communicate with the game via stdio, allowing AI to inject input (UI clicks and keyboard) and observe game state for both gameplay and test validation.

## Architecture

```
Cursor → MCP Server (Python) → stdio → Game Client (C++)
```

The game client will:
- Read JSON commands from stdin
- Write JSON responses to stdout
- Process commands asynchronously during the game loop
- Support both UI clicks and keyboard input injection

## Requirements

1. **Input Control**: Both UI interactions (button clicks) and keyboard input (arrow keys, enter, etc.)
2. **State Observation**: Query current screen, available buttons, game state
3. **Test Validation**: Observe test execution to validate that tests are doing what they claim
4. **Client-Focused**: Works with the game client (not the battle server), simulating a real player

## Implementation Steps

### Step 1: Create MCP Server Infrastructure in C++

**Goal**: Create the core MCP server class that handles stdio communication and command processing.

**Files to Create**:
- `src/mcp/mcp_server.h` - MCP server class declaration
- `src/mcp/mcp_server.cpp` - Implementation

**Key Components**:
- `MCPServer` class that:
  - Reads JSON commands from stdin (non-blocking)
  - Maintains a command queue
  - Processes commands each frame
  - Writes JSON responses to stdout
- Command protocol: JSON-RPC-like format
  ```json
  {"id": 1, "method": "get_state", "params": {}}
  {"id": 1, "result": {...}, "error": null}
  ```

**Commands to Support**:
- `get_state` - return current screen, available buttons, game state
- `click_button` - click UI button by label
- `send_input` - inject keyboard input (InputAction enum)
- `list_buttons` - enumerate all clickable UI elements
- `wait_for_screen` - wait for screen transition (with timeout)

**Implementation Notes**:
- Use `select()` or similar for non-blocking stdin reads
- Process commands in a system that runs each frame
- Thread-safe command queue or frame-based processing
- Handle JSON parsing (can use `nlohmann/json` which is already in the project)

### Step 2: Implement Input Injection System

**Goal**: Create helpers to inject both UI clicks and keyboard input into the game.

**Files to Create**:
- `src/mcp/input_injector.h` - Input injection helpers

**Key Functions**:
- `inject_ui_click(const std::string &button_label)`:
  - Find entity with `HasClickListener` and matching label
  - Call the callback directly (reuse `TestApp::click_clickable` pattern)
  - Handle errors if button not found
  
- `inject_keyboard_input(InputAction action)`:
  - Get `InputCollector` singleton component
  - Create `ActionDone` entry with:
    - `medium = DeviceMedium::Keyboard`
    - `action = to_int(action)`
    - `amount_pressed = 1.0f`
    - `length_pressed = dt` (from current frame)
  - Add to `inputs_pressed` vector
  - Ensure it doesn't conflict with real input (maybe use a flag or separate queue)

**Implementation Notes**:
- Reuse patterns from `TestApp::find_clickable_with` and `TestApp::click_clickable`
- For keyboard, directly manipulate `InputCollector` component
- Consider adding a flag to distinguish injected vs real input if needed

### Step 3: Create State Serialization System

**Goal**: Convert game state to JSON for MCP server responses.

**Files to Create**:
- `src/mcp/state_serializer.h` - State serialization functions

**Key Functions**:
- `serialize_game_state()`:
  - Current screen (`GameStateManager::Screen` as string)
  - Game state (Menu, Playing, Paused)
  - Available buttons (labels and debug names)
  
- `serialize_battle_state()`:
  - Battle progress, current course
  - Player/opponent dish states
  - Combat stats if available
  
- `serialize_shop_state()`:
  - Shop items available
  - Inventory state
  - Wallet balance

**Implementation Notes**:
- Use `nlohmann::json` for serialization
- Reuse query patterns from `TestContext::list_buttons`
- Return empty objects for states that aren't active

### Step 4: Integrate MCP Server into Game Loop

**Goal**: Add MCP support to main game executable with command-line flag.

**Files to Modify**:
- `src/main.cpp` - Add `--mcp` flag and initialization
- `src/systems/MCPServerSystem.h` - System wrapper for MCP server

**Changes to `main.cpp`**:
- Parse `--mcp` flag from command line
- When enabled:
  - Initialize `MCPServer` singleton
  - Register `MCPServerSystem` as update system (runs each frame)
  - Set stdin to non-blocking mode (or use separate thread)
  - Process commands each frame without blocking game loop

**System Implementation**:
- `MCPServerSystem` extends `System<>`
- In `for_each_with`, call `MCPServer::process_commands()`
- Handle command execution and response writing

**Implementation Notes**:
- MCP mode should work alongside normal gameplay
- Don't interfere with existing input handling
- Ensure commands are processed promptly but don't block rendering

### Step 5: Create Python MCP Server

**Goal**: Implement MCP server in Python that communicates with game via stdio.

**Files to Create**:
- `mcp_server/main.py` - Python MCP server
- `mcp_server/requirements.txt` - Python dependencies

**Dependencies**:
- `mcp` - MCP SDK for Python
- `json` - JSON handling (standard library)

**MCP Tools to Implement**:
- `get_game_state`:
  - Sends `get_state` command to game
  - Returns current screen, available actions, game state
  
- `click_button`:
  - Parameters: `button_label` (string)
  - Sends `click_button` command
  - Returns success/failure
  
- `send_keyboard_input`:
  - Parameters: `action` (string, e.g., "WidgetPress", "WidgetLeft")
  - Maps string to `InputAction` enum value
  - Sends `send_input` command
  
- `list_available_actions`:
  - Sends `list_buttons` command
  - Returns list of clickable buttons
  
- `wait_for_screen`:
  - Parameters: `screen` (string), `timeout` (float, seconds)
  - Polls `get_state` until screen matches or timeout
  - Returns success/failure
  
- `observe_test_execution`:
  - Parameters: `test_name` (string)
  - Periodically queries game state during test
  - Reports state changes to Cursor
  - Validates that test actions match expected state

**Communication Protocol**:
- Spawn game process with `--mcp` flag
- Connect stdin/stdout pipes
- Send JSON commands, receive JSON responses
- Handle errors and timeouts gracefully

### Step 6: Configure Cursor MCP Settings

**Goal**: Set up Cursor to connect to the MCP server.

**Files to Create**:
- `.cursor/mcp.json` - Cursor MCP configuration

**Configuration**:
```json
{
  "mcpServers": {
    "my-name-chef": {
      "command": "python",
      "args": ["mcp_server/main.py"],
      "env": {
        "GAME_EXECUTABLE": "./output/my_name_chef.exe"
      }
    }
  }
}
```

**Alternative**: Configure via Cursor settings UI if JSON config isn't supported.

### Step 7: Add Test Observation Support

**Goal**: Enable MCP server to observe and validate test execution.

**Enhancements to Python MCP Server**:
- `observe_test_execution` tool:
  - Start game with `--run-test <test_name> --mcp`
  - Poll game state at regular intervals
  - Track state transitions (screen changes, button clicks)
  - Compare observed state with test assertions
  - Report discrepancies to Cursor

**Game Client Support**:
- Ensure test system works with MCP enabled
- State queries should reflect test execution state
- Don't interfere with test timing or execution

## Files to Create/Modify

### New Files:
- `src/mcp/mcp_server.h` - MCP server class
- `src/mcp/mcp_server.cpp` - Implementation
- `src/mcp/input_injector.h` - Input injection helpers
- `src/mcp/state_serializer.h` - State serialization
- `src/systems/MCPServerSystem.h` - System wrapper
- `mcp_server/main.py` - Python MCP server
- `mcp_server/requirements.txt` - Python dependencies
- `.cursor/mcp.json` - Cursor MCP configuration (if supported)

### Modified Files:
- `src/main.cpp` - Add `--mcp` flag and initialization
- `makefile` - Add MCP source files to build

## Key Design Decisions

1. **stdio over HTTP**: Client doesn't have HTTP server, stdio is simpler and doesn't require network setup
2. **Frame-based processing**: Process commands each frame to avoid blocking game loop
3. **Reuse test infrastructure**: Leverage `TestApp` and `TestContext` patterns for UI interaction
4. **Input injection**: Add `ActionDone` entries directly to `InputCollector` for keyboard, call callbacks for UI clicks
5. **Non-blocking reads**: Use `select()` or similar to check stdin without blocking
6. **JSON protocol**: Simple JSON-RPC-like protocol for command/response
7. **Separate process**: MCP server runs as separate Python process, spawns game client

## Validation Steps

1. **Basic Communication**:
   - Start game with `--mcp` flag
   - MCP server connects and can query state
   - Verify JSON commands/responses work correctly

2. **Input Injection**:
   - AI can click buttons via MCP
   - AI can send keyboard input
   - Game responds correctly to injected input
   - Real input still works alongside injected input

3. **State Observation**:
   - AI can query current screen
   - AI can list available buttons
   - State reflects actual game state accurately

4. **Test Observation**:
   - Run test with MCP enabled
   - AI can observe test execution
   - State queries show test progress
   - Validation works correctly

5. **Cursor Integration**:
   - Cursor can control game during interactive runs
   - AI can navigate menus, play game, observe state
   - Test validation works as expected

## Implementation Order

1. Create MCP server infrastructure (Step 1)
2. Implement input injection (Step 2)
3. Create state serialization (Step 3)
4. Integrate into game loop (Step 4)
5. Create Python MCP server (Step 5)
6. Configure Cursor (Step 6)
7. Add test observation (Step 7)

## Estimated Difficulty

**Overall**: Medium (3-5 days)

- Steps 1-4: Medium (C++ integration, ~2 days)
- Step 5: Medium (Python MCP server, ~1 day)
- Steps 6-7: Easy (Configuration and enhancements, ~1 day)

## Dependencies

- `nlohmann/json` - Already in project (vendor/nlohmann/)
- Python `mcp` SDK - Need to install via pip
- Cursor MCP support - Should be available in Cursor IDE

## Notes

- MCP server should handle game process lifecycle (start/stop)
- Consider adding logging for debugging MCP communication
- May need to handle edge cases (game crashes, stdin/stdout errors)
- Test observation should not interfere with test execution timing

## Outstanding Questions
1. **Security / Sandbox:** Do we need authentication or rate limiting on the MCP command stream to avoid misuse?
2. **Process Lifecycle:** Should the Python adapter spawn the game process or attach to an already running instance?
3. **State Schema:** What JSON schema do we commit to for `get_state` (fields, nesting) so downstream tools remain stable?
4. **Error Surfacing:** How do we report game-side exceptions back through MCP—standard JSON-RPC errors or custom payloads?
5. **Test Observation Granularity:** Do we need per-step callbacks/events, or is polling state sufficient for validating scripted tests?

