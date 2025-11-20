# Shop UX Affordances Plan

## Overview

This plan details UI/UX improvements for the shop: price display, drop target highlights, and reroll cost display.

## Goal

Enhance shop UX with:
- Price display (all items cost 3 gold)
- Faint highlight on legal drop targets
- Reroll cost display (current cost, increment)

## Current State

### Existing Features

- ✅ Shop system exists with 7 slots
- ✅ Purchase/sell functionality works
- ✅ Freeze system exists (`Freezeable` component)
- ✅ "Frozen" badge on frozen items (already done)
- ✅ Freeze toggle (already done)

### Missing Features

- ❌ Price display in shop UI
- ❌ Drop target highlighting
- ❌ Reroll cost display (shows current cost)

## Implementation Details

### 1. Price Display

**Location**: Shop UI (near shop items or in header)

**Display**:
- Show "3 gold" somewhere visible in shop area
- Can be:
  - Header text: "Items: 3 gold each"
  - Small text above shop slots
  - Tooltip on hover over shop area

**Implementation**:
- Add text rendering in shop UI system
- Position: Top of shop area or above shop slots
- Style: Match existing UI text style

**Note**: Eventually drinks will cost 1 gold, but not implementing that yet.

### 2. Drop Target Highlighting

**Purpose**: Show user where they can drop items (empty slots or merge targets)

**Visual**:
- Faint highlight on legal drop targets
- Only show when item is being dragged
- Different highlight for:
  - Empty slots (green/blue faint glow)
  - Merge targets (yellow/orange faint glow)

**Implementation**:
- Detect when item is being dragged (`IsHeld` component)
- Query for legal drop targets:
  - Empty inventory slots (`IsDropSlot` with `occupied == false`)
  - Slots with mergeable dishes (same type, valid level)
- Render faint highlight rectangle/glow on valid targets
- Use semi-transparent color overlay

**System**: Create `RenderDropTargetHighlightsSystem` or extend existing shop rendering

### 3. Reroll Cost Display

**Current**: Button shows "Reroll (1)" - cost is hardcoded in button label

**Enhancement**: 
- Show current cost from `RerollCost` singleton
- Show increment if > 0
- Format: "Reroll (1)" or "Reroll (1, +0)" or "Reroll (1 → 2)"

**Implementation**:
- Read `RerollCost` singleton
- Display `current` cost
- Optionally show `increment` if non-zero
- Update button label dynamically

**Location**: Reroll button (already exists, just update label)

## UI Layout

### Shop Area Layout

```
┌─────────────────────────────────┐
│ Items: 3 gold each              │ ← Price display
├─────────────────────────────────┤
│ [Shop Slot 1] [Shop Slot 2] ... │ ← Shop items
│                                 │
│ [Inventory Slot 1] [Slot 2]... │ ← Inventory (with highlights when dragging)
└─────────────────────────────────┘
[Reroll (1)]                       ← Reroll button with cost
```

### Highlight Colors

- **Empty Slot Highlight**: `Color{100, 200, 100, 100}` (green, faint)
- **Merge Target Highlight**: `Color{200, 200, 100, 100}` (yellow, faint)
- **Opacity**: ~40% (faint, doesn't obscure items)

## Implementation Files

### 1. Price Display

**File**: `src/ui/ui_systems.cpp` or shop rendering system

**Function**: Add text rendering in shop area

### 2. Drop Target Highlights

**File**: `src/systems/RenderDropTargetHighlightsSystem.h` (new)

**Logic**:
- Check if any item has `IsHeld` component
- Find legal drop targets
- Render highlights

### 3. Reroll Cost Display

**File**: `src/ui/ui_systems.cpp` (existing reroll button)

**Change**: Update button label to read from `RerollCost` singleton

## Test Cases

### Test 1: Price Display

**Setup**: Navigate to shop screen

**Expected**: "3 gold" or "Items: 3 gold each" visible in shop area

### Test 2: Drop Target Highlights

**Setup**: 
- Navigate to shop
- Start dragging a shop item

**Expected**:
- Empty inventory slots show faint green highlight
- Slots with mergeable dishes show faint yellow highlight
- Highlights disappear when item released

### Test 3: Reroll Cost Display

**Setup**: Navigate to shop screen

**Expected**: Reroll button shows current cost (e.g., "Reroll (1)")

## Validation Steps

1. **Add Price Display**
   - Add text rendering in shop UI
   - Position and style appropriately

2. **Implement Drop Target Highlights**
   - Create `RenderDropTargetHighlightsSystem`
   - Detect drag state and legal targets
   - Render faint highlights

3. **Update Reroll Cost Display**
   - Read from `RerollCost` singleton
   - Update button label dynamically

4. **MANDATORY CHECKPOINT**: Build
   - Run `xmake` - code must compile successfully

5. **MANDATORY CHECKPOINT**: Run Headless Tests
   - Run `./scripts/run_all_tests.sh` (ALL must pass)

6. **MANDATORY CHECKPOINT**: Run Non-Headless Tests
   - Run `./scripts/run_all_tests.sh -v` (ALL must pass)

7. **Verify UX Improvements**
   - Test price display visible
   - Test drop highlights work
   - Test reroll cost displays correctly

8. **MANDATORY CHECKPOINT**: Commit
   - `git commit -m "ui - enhance shop UX with prices, drop highlights, and reroll cost"`

## Edge Cases

1. **No Empty Slots**: Highlights only show merge targets
2. **No Merge Targets**: Highlights only show empty slots
3. **Full Inventory**: No highlights (can't drop anywhere)
4. **Reroll Cost Changes**: Button label updates when cost changes

## Success Criteria

- ✅ Price display visible in shop
- ✅ Drop target highlights appear when dragging
- ✅ Reroll cost displays current value
- ✅ All tests pass in headless and non-headless modes

## Estimated Time

3-4 hours:
- 1 hour: Price display
- 1.5 hours: Drop target highlights
- 30 min: Reroll cost display
- 1 hour: Testing and polish

