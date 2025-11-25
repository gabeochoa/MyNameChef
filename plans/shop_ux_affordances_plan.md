# Shop UX Affordances Plan

## Overview

This plan details UI/UX improvements for the shop: price display, drop target highlights, and reroll cost display.

## Goal

Enhance shop UX with:
- Price display (all items cost 3 gold)
- Faint highlight on legal drop targets (empty slots, merge targets, sell slot)
- Star indicator on shop items that match inventory items
- Reroll cost display (format: "Reroll (X gold)")

## Current State

### Existing Features

- ✅ Shop system exists with 7 slots
- ✅ Purchase/sell functionality works
- ✅ Freeze system exists (`Freezeable` component)
- ✅ "Frozen" badge on frozen items (already done)
- ✅ Freeze toggle (already done)

### Missing Features

- ✅ Price display in shop UI - **COMPLETE** (`RenderShopPriceDisplaySystem.h` exists)
- ✅ Drop target highlighting (empty slots, merge targets, sell slot) - **COMPLETE** (`RenderDropTargetHighlightsSystem.h` exists)
- ✅ Star indicator on shop items matching inventory - **COMPLETE** (`RenderShopItemMatchIndicatorSystem.h` exists)
- ✅ Reroll cost display format update - **COMPLETE** (shows "Reroll (X gold)" in `ui_systems.cpp` line 510)

## Status: ✅ COMPLETE

All shop UX features have been implemented:
- Price display system exists and renders "Items: 3 gold each"
- Drop target highlights work for empty slots, merge targets, and sell slot
- Star indicators appear on shop items matching inventory
- Reroll button displays cost with "gold" suffix

This plan can be considered complete.

## Implementation Details

### 1. Price Display

**Location**: Shop UI (near shop items or in header)

**Display**:
- Show "3 gold" somewhere visible in shop area
- Recommended: Header text "Items: 3 gold each" above shop slots
- Alternative: Small text label above shop area

**Implementation**:
- **Method**: Use `render_backend::DrawTextWithActiveFont()` (similar to `RenderWalletHUD`)
- **Font Size**: Use `font_sizes::Normal` (20.f) or `font_sizes::Medium` (24.f) from `src/font_info.h`
- **Position**: `SHOP_START_X, SHOP_START_Y - font_size * 1.5f` (1.5x font size above shop slots)
- **Color**: `raylib::WHITE` or `raylib::LIGHTGRAY` (match existing UI text)
- **System**: Create `RenderShopPriceDisplaySystem` or add to existing shop render system
- **Render Order**: Use `RenderOrder::UI` with `RenderScreen::Shop`

**Note**: Eventually drinks will cost 1 gold, but not implementing that yet.

### 2. Drop Target Highlighting

**Purpose**: Show user where they can drop items (empty slots, merge targets, sell slot)

**Visual**:
- Faint highlight on legal drop targets
- Only show when item is being dragged
- Different highlight for:
  - Empty slots (green/blue faint glow)
  - Merge targets (yellow/orange faint glow)
  - Sell slot (red/orange faint glow)

**Implementation**:
- **System**: Create `RenderDropTargetHighlightsSystem` (new render system)
- **Render Order**: Use `RenderOrder::UI` with `RenderScreen::Shop` (render above items but below tooltips)
- **Detection**: Query for entities with `IsHeld` component to find dragged item
  - If no `IsHeld` entities found, don't render highlights
  - Get dragged item's `IsDish.type` and `DishLevel.level` for merge checks
- **Find Legal Drop Targets**:
  - **Empty inventory slots**: Query `IsDropSlot` where `occupied == false` and `slot_id < SELL_SLOT_ID`
  - **Sell slot**: Query `IsDropSlot` where `slot_id == SELL_SLOT_ID`
  - **Merge targets**: 
    - Loop through all `IsInventoryItem` entities (max 7 items)
    - For each inventory item, check:
      - Has `IsDish` component
      - `IsDish.type == dragged item's type`
      - Has `DishLevel` component
      - `DishLevel.level >= dragged item's level` (can merge)
    - If match found, get item's slot ID and find corresponding `IsDropSlot` to highlight
- **Rendering**:
  - Use `raylib::DrawRectangle()` with semi-transparent color overlay
  - Draw rectangle at slot's `Transform.position` with slot's `Transform.size`
  - Use colors specified in "Highlight Colors" section below
  - Set blend mode if needed for transparency

**System Pattern**: Follow `RenderFreezeIcon` or `RenderSellSlot` as examples

### 3. Reroll Cost Display

**Current**: Button shows "Reroll (1)" - already reads from `RerollCost` singleton

**Enhancement**: 
- Update format to show cost with "gold" suffix
- Format: "Reroll (1 gold)" or "Reroll (3 gold)" based on current cost
- Button already reads from `RerollCost` singleton, just need to update label format

**Implementation**:
- Update button label format in `src/ui/ui_systems.cpp`
- Change from `fmt::format("Reroll ({})", reroll_cost)` to `fmt::format("Reroll ({} gold)", reroll_cost)`
- Button already dynamically reads from `RerollCost` singleton

**Location**: Reroll button (already exists, just update label format)

### 4. Shop Item Match Indicator (Star)

**Purpose**: Show which shop items match items already in inventory (helps identify merge opportunities)

**Visual**:
- Small star icon/indicator on shop items that match inventory items
- Position: Top-right corner of shop item sprite
- Color: Yellow/gold star to indicate match

**Implementation**:
- **System**: Create `RenderShopItemMatchIndicatorSystem` (new render system)
- **Render Order**: Use `RenderOrder::UI` with `RenderScreen::Shop` (render above shop item sprites)
- **Pattern**: Follow `RenderFreezeIcon` as example (similar positioning logic)
- **For each shop item** (`IsShopItem` with `Transform`):
  - Loop through all `IsInventoryItem` entities (max 7 items)
  - Check if shop item has `IsDish` component
  - For each inventory item:
    - Check if inventory item has `IsDish` component
    - If `shop_item.IsDish.type == inventory_item.IsDish.type`, match found
  - **If match found**, render star indicator
- **Star Rendering**:
  - **Option 1 (Recommended)**: Use text character "⭐" with `render_backend::DrawTextWithActiveFont()`
    - **Font Size**: Use `font_sizes::Small` (20.f) from `src/font_info.h`
    - **Measure text**: Use `render_backend::MeasureTextWithActiveFont("⭐", font_size)` to get star width/height
    - **Position**: 
      - X: `transform.position.x + transform.size.x - star_width - font_size * 0.25f` (0.25x font size padding from right edge)
      - Y: `transform.position.y + font_size * 0.25f` (0.25x font size padding from top edge)
  - **Option 2**: Use star texture if available in `TextureLibrary`
    - Similar to `RenderFreezeIcon` but positioned at top-right
    - **Size**: Use `font_sizes::Small` (20.f) as base size for texture scaling
- **Update**: System runs every frame, so automatically updates when inventory changes

**Note**: This makes it obvious which shop items can be merged with existing inventory items

## UI Layout

### Shop Area Layout

```
┌─────────────────────────────────┐
│ Items: 3 gold each              │ ← Price display
├─────────────────────────────────┤
│ [⭐Shop 1] [Shop 2] [⭐Shop 3]... │ ← Shop items (⭐ = matches inventory)
│                                 │
│ [Inventory Slot 1] [Slot 2]... │ ← Inventory (with highlights when dragging)
│                                 │
│ [Sell Slot]                     │ ← Sell slot (highlighted when dragging)
└─────────────────────────────────┘
[Reroll (1 gold)]                  ← Reroll button with cost
```

### Highlight Colors

- **Empty Slot Highlight**: `Color{100, 200, 100, 100}` (green, faint)
- **Merge Target Highlight**: `Color{200, 200, 100, 100}` (yellow, faint)
- **Sell Slot Highlight**: `Color{200, 100, 100, 100}` (red/orange, faint)
- **Opacity**: ~40% (faint, doesn't obscure items)

### Star Indicator

- **Star Color**: `Color{255, 215, 0, 255}` (gold/yellow)
- **Star Font Size**: Use `font_sizes::Small` (20.f) from `src/font_info.h`
- **Star Position**: Top-right corner of shop item sprite
- **Padding**: Use `font_size * 0.25f` (0.25x font size) for padding from edges

## Implementation Files

### 1. Price Display

**File**: `src/systems/RenderShopPriceDisplaySystem.h` (new)

**System Pattern**:
```cpp
#include "font_info.h"

struct RenderShopPriceDisplaySystem : System<> {
  virtual bool should_run(float) override {
    return GameStateManager::get().active_screen == GameStateManager::Screen::Shop;
  }
  void once(float) const override {
    float font_size = font_sizes::Normal; // 20.f
    float y_pos = SHOP_START_Y - font_size * 1.5f; // 1.5x font size above shop slots
    render_backend::DrawTextWithActiveFont("Items: 3 gold each", SHOP_START_X, static_cast<int>(y_pos), font_size, raylib::WHITE);
  }
};
```

**Registration**: Add to `register_shop_render_systems()` in `src/shop.cpp`

### 2. Drop Target Highlights

**File**: `src/systems/RenderDropTargetHighlightsSystem.h` (new)

**System Pattern**:
```cpp
struct RenderDropTargetHighlightsSystem : System<> {
  virtual bool should_run(float) override {
    return GameStateManager::get().active_screen == GameStateManager::Screen::Shop;
  }
  void once(float) const override {
    // Find dragged item
    auto dragged_item = EQ().whereHasComponent<IsHeld>().gen_first();
    if (!dragged_item) return;
    
    // Get dragged item's type and level
    // Query for drop targets and render highlights
  }
};
```

**Registration**: Add to `register_shop_render_systems()` in `src/shop.cpp`

**Key Implementation Details**:
- Query `IsHeld` to get dragged item entity
- Get dragged item's `IsDish.type` and `DishLevel.level`
- For merge targets: Loop `IsInventoryItem` entities, check type/level match, then find their slot's `IsDropSlot` to highlight
- Use `raylib::DrawRectangle()` with semi-transparent colors

### 3. Shop Item Match Indicator

**File**: `src/systems/RenderShopItemMatchIndicatorSystem.h` (new)

**System Pattern**:
```cpp
#include "font_info.h"
#include "render_backend.h"

struct RenderShopItemMatchIndicatorSystem : System<IsShopItem, Transform, IsDish> {
  virtual bool should_run(float) override {
    return GameStateManager::get().active_screen == GameStateManager::Screen::Shop;
  }
  void for_each_with(const Entity &shop_item, const IsShopItem &, const Transform &transform, const IsDish &dish, float) const override {
    // Loop through IsInventoryItem entities
    // Check if any match shop_item's dish type
    // If match:
    float font_size = font_sizes::Small; // 20.f
    float star_width = render_backend::MeasureTextWithActiveFont("⭐", font_size);
    float padding = font_size * 0.25f; // 0.25x font size
    float x = transform.position.x + transform.size.x - star_width - padding;
    float y = transform.position.y + padding;
    render_backend::DrawTextWithActiveFont("⭐", static_cast<int>(x), static_cast<int>(y), font_size, raylib::Color{255, 215, 0, 255});
  }
};
```

**Registration**: Add to `register_shop_render_systems()` in `src/shop.cpp`

**Key Implementation Details**:
- Use `System<IsShopItem, Transform, IsDish>` to iterate shop items
- Loop through `IsInventoryItem` entities (max 7) to find matches
- Render star using `raylib::DrawText("⭐", x, y, size, color)` or texture

### 4. Reroll Cost Display

**File**: `src/ui/ui_systems.cpp` (existing reroll button, line ~488)

**Change**: Update button label format from `fmt::format("Reroll ({})", reroll_cost)` to `fmt::format("Reroll ({} gold)", reroll_cost)`

**No system registration needed** - already in UI system

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
- Sell slot shows faint red/orange highlight
- Highlights disappear when item released

### Test 3: Shop Item Match Indicator

**Setup**:
- Navigate to shop
- Have at least one dish in inventory (e.g., Potato)
- Shop should have matching dish type (e.g., Potato)

**Expected**:
- Shop items matching inventory items show star indicator (⭐)
- Star appears at top-right corner of matching shop items
- Star disappears when matching inventory item is removed

### Test 4: Reroll Cost Display

**Setup**: Navigate to shop screen

**Expected**: Reroll button shows current cost with "gold" suffix (e.g., "Reroll (1 gold)")

## Validation Steps

1. **Add Price Display**
   - Create `src/systems/RenderShopPriceDisplaySystem.h`
   - Use `render_backend::DrawTextWithActiveFont()` with `font_sizes::Normal` (20.f)
   - Position: `SHOP_START_X, SHOP_START_Y - font_size * 1.5f`
   - Register in `register_shop_render_systems()` in `src/shop.cpp`

2. **Implement Drop Target Highlights**
   - Create `src/systems/RenderDropTargetHighlightsSystem.h`
   - Query for `IsHeld` to find dragged item
   - Get dragged item's `IsDish.type` and `DishLevel.level`
   - Query for empty slots (`IsDropSlot` with `occupied == false` and `slot_id < SELL_SLOT_ID`)
   - Query for sell slot (`IsDropSlot` with `slot_id == SELL_SLOT_ID`)
   - Loop through `IsInventoryItem` entities to find merge targets (check type/level match)
   - Use `raylib::DrawRectangle()` with semi-transparent colors on slot transforms
   - Register in `register_shop_render_systems()` in `src/shop.cpp`

3. **Implement Shop Item Match Indicator**
   - Create `src/systems/RenderShopItemMatchIndicatorSystem.h`
   - Use `System<IsShopItem, Transform, IsDish>` pattern
   - Loop through `IsInventoryItem` entities (max 7) to find type matches
   - Render star using `render_backend::DrawTextWithActiveFont("⭐", x, y, font_size, color)`
   - Font size: Use `font_sizes::Small` (20.f)
   - Measure star width: `render_backend::MeasureTextWithActiveFont("⭐", font_size)`
   - Position: 
     - X: `transform.position.x + transform.size.x - star_width - font_size * 0.25f`
     - Y: `transform.position.y + font_size * 0.25f`
   - Register in `register_shop_render_systems()` in `src/shop.cpp`

4. **Update Reroll Cost Display**
   - Edit `src/ui/ui_systems.cpp` line ~488
   - Change `fmt::format("Reroll ({})", reroll_cost)` to `fmt::format("Reroll ({} gold)", reroll_cost)`
   - No system registration needed (already in UI system)

5. **MANDATORY CHECKPOINT**: Build
   - Run `make` - code must compile successfully

5. **MANDATORY CHECKPOINT**: Run Headless Tests
   - Run `./scripts/run_all_tests.sh` (ALL must pass)

6. **MANDATORY CHECKPOINT**: Run Non-Headless Tests
   - Run `./scripts/run_all_tests.sh -v` (ALL must pass)

7. **Verify UX Improvements**
   - Test price display visible
   - Test drop highlights work (empty slots, merge targets, sell slot)
   - Test star indicators appear on matching shop items
   - Test reroll cost displays correctly

8. **MANDATORY CHECKPOINT**: Commit
   - `git commit -m "ui - enhance shop UX with prices, drop highlights, match indicators, and reroll cost"`

## Edge Cases

1. **No Empty Slots**: Highlights only show merge targets and sell slot
2. **No Merge Targets**: Highlights only show empty slots and sell slot
3. **Full Inventory**: Highlights only show sell slot (can't drop in inventory)
4. **Multiple Matches**: Star shows on shop item if ANY inventory item matches (not all)
5. **Inventory Changes**: Star indicators update when items added/removed from inventory
6. **Reroll Cost Changes**: Button label updates when cost changes

## Success Criteria

- ✅ Price display visible in shop
- ✅ Drop target highlights appear when dragging (empty slots, merge targets, sell slot)
- ✅ Star indicators appear on shop items matching inventory
- ✅ Reroll cost displays current value with "gold" suffix
- ✅ All tests pass in headless and non-headless modes

## Common Questions / FAQ

### Q: How do I get the dragged item's type and level?
**A**: Query for entities with `IsHeld` component: `EQ().whereHasComponent<IsHeld>().gen_first()`. Then check if it has `IsDish` and `DishLevel` components.

### Q: How do I find which slot an inventory item is in?
**A**: Inventory items have `IsInventoryItem.slot` field. Use that to find the corresponding `IsDropSlot` with matching `slot_id`.

### Q: What render order should I use?
**A**: Use `RenderOrder::UI` with `RenderScreen::Shop`. Highlights should render above items but below tooltips. Star indicators should render above shop item sprites.

### Q: How do I register a new render system?
**A**: Add to `register_shop_render_systems()` function in `src/shop.cpp`. See existing systems like `RenderDrinkShopSlots` for example.

### Q: Should I use `should_run()` to check shop screen?
**A**: Yes, always check `GameStateManager::get().active_screen == GameStateManager::Screen::Shop` in `should_run()`.

### Q: How do I render semi-transparent rectangles?
**A**: Use `raylib::DrawRectangle()` with `Color{r, g, b, alpha}` where alpha is 100-150 for faint highlights. Or use `raylib::DrawRectangleLines()` for outlines.

### Q: What font sizes should I use?
**A**: Use constants from `font_sizes` namespace in `src/font_info.h`:
- `font_sizes::Small` = 20.f
- `font_sizes::Normal` = 20.f  
- `font_sizes::Medium` = 24.f
- `font_sizes::Large` = 36.f
Never use hardcoded pixel values - always use font size multipliers (e.g., `font_size * 1.5f` for spacing).

### Q: How do I measure text for positioning?
**A**: Use `render_backend::MeasureTextWithActiveFont(text, font_size)` to get text width. Use this to calculate positions relative to font size, not fixed pixels.

### Q: What if there are multiple items being dragged?
**A**: Typically only one item can be dragged at a time. If multiple `IsHeld` entities exist, highlight targets for the first one found (or all of them).

### Q: How do I check if a shop item matches inventory?
**A**: Loop through all `IsInventoryItem` entities, check if any have `IsDish.type == shop_item.IsDish.type`. No need to check level for the star indicator (just type match).

### Q: Should the star update in real-time?
**A**: Yes, the system runs every frame, so it automatically updates when inventory changes (items added/removed).

### Q: What if a shop item doesn't have `IsDish` component?
**A**: Skip it - only shop items with `IsDish` can match inventory items. Use `System<IsShopItem, Transform, IsDish>` to ensure `IsDish` exists.

## Estimated Time

4-5 hours:
- 1 hour: Price display
- 1.5 hours: Drop target highlights (including sell slot)
- 1 hour: Shop item match indicator (star)
- 30 min: Reroll cost display format update
- 1 hour: Testing and polish

