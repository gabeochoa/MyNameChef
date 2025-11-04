# Comprehensive System Verification Strategy

## Verification Methods for All Systems

### Method 1: Direct Registration Check
**How**: Grep `src/main.cpp` for `make_unique<SystemName>`
**Use For**: Systems registered directly in main.cpp
**Command**: `grep -E "make_unique<.*System>" src/main.cpp`

**Already Verified Systems** (registered directly):
- All systems in main.cpp lines 145-261 (update systems)
- All systems in main.cpp lines 211-261 (render systems)

### Method 2: Helper Function Registration Check
**How**: Check helper registration functions
**Use For**: Systems registered via helper functions
**Verification Points**:
- `register_shop_update_systems()` in `src/shop.cpp` (lines 203-207)
- `register_shop_render_systems()` in `src/shop.cpp` (line 209 - currently empty)
- `register_ui_systems()` in `src/ui/ui_systems.cpp` (line 590)
- `register_sound_systems()` in `src/sound_systems.cpp`
- `texture_manager::register_update_systems()` (vendor plugin)
- `afterhours::animation::register_update_systems<>()` (vendor plugin)
- `ui::register_render_systems<>()` (vendor plugin)

**Verified via Helpers**:
- `GenerateShopSlots` - via `register_shop_update_systems()`
- `GenerateInventorySlots` - via `register_shop_update_systems()`

### Method 3: Include File Check
**How**: Check if system header is included in main.cpp
**Use For**: Systems that might be used but not registered (e.g., utility classes)
**Command**: `grep "#include.*systems/" src/main.cpp`

**Special Cases**:
- `ExportMenuSnapshotSystem` - included but used directly (not as registered system)
- `PostProcessingSystems.h` - header file containing multiple systems
- `LetterboxLayout.h` - utility header, not a system

### Method 4: Component Dependency Check
**How**: Check if system's unique components are used elsewhere
**Use For**: Systems with unique components that might indicate dead code
**Command**: `grep "ComponentName" src/`

**Systems to Check**:
- `ProcessCollisionAbsorption` uses `CollisionAbsorber`
- `UpdateTrackingEntities` uses `TracksEntity`
- `DishGenerationSystem` uses `DishGenerationAbility`
- `ProcessDishGenerationSystem` uses `PendingDishGeneration`

### Method 5: Planning Document Search
**How**: Search planning documents for system names
**Use For**: Determine if systems are planned for future features
**Documents**: HEAD_TO_HEAD_COMBAT_PLAN.md, next_todo.md, TODO.md, FEATURE_STATUS.md
**Command**: `grep -i "SystemName" HEAD_TO_HEAD_COMBAT_PLAN.md next_todo.md TODO.md`

### Method 6: Code Reference Check
**How**: Check if system is referenced by other systems (not just registered)
**Use For**: Systems that might be used indirectly
**Command**: `grep -r "SystemName" src/`

### Method 7: File Content Analysis
**How**: Read the system file to understand its purpose
**Use For**: Systems where purpose is unclear
**What to Check**:
- System comments/header
- Component dependencies
- Whether it's a base class vs concrete system
- Whether it's a utility vs actual system

## System Categories

### Category A: Confirmed Registered (False Positives)
**Action**: Update script to detect these

**Systems**:
- `CleanupBattleEntities`, `CleanupShopEntities`, `CleanupDishesEntities`
- `GenerateDishesGallery`, `DropWhenNoLongerHeld`, `HandleFreezeIconClick`
- `MarkIsHeldWhenHeld`, `InitCombatState`, `InitialShopFill`
- `LoadBattleResults`, `ProcessBattleRewards`, `UpdateRenderTexture`
- `MarkEntitiesWithShaders`, `RenderEntitiesByOrder`, `RenderBattleTeams`
- `RenderSpritesByOrder`, `RenderSpritesWithShaders`, `RenderZingBodyOverlay`
- `RenderAnimations`, `RenderDishProgressBars`, `RenderWalletHUD`
- `RenderRenderTexture`, `RenderLetterboxBars`, `RenderSellSlot`
- `RenderFreezeIcon`, `RenderBattleResults`, `RenderTooltipSystem`
- `RenderFPS`, `RenderDebugWindowInfo`
- All post-processing and camera systems
- `GenerateShopSlots`, `GenerateInventorySlots` (via helpers)

### Category B: Utility/Header Files (Not Systems)
**Action**: Exclude from system check

**Files**:
- `LetterboxLayout.h` - utility struct
- `RenderSystemHelpers.h` - helper functions
- `PausableSystem.h` - base class template

### Category C: Actually Unused Systems
**Action**: Review against planning docs

**Systems**:
- `DishGenerationSystem` - not registered
- `ProcessDishGenerationSystem` - not registered
- `ProcessCollisionAbsorption` - not registered
- `UpdateTrackingEntities` - not registered
- `EnterAnimationSystem` - duplicate/wrong filename
- `JudgeAnimations` - legacy judging
- `UnifiedAnimationSystem` - included but not registered
- `RenderAnimationsWithShaders` - included but not registered
- `RenderEntities` - not registered
- `RenderDebugGridOverlay` - not registered
- `RenderPlayerHUD` - not registered
- `BeginPostProcessingShader` - check if different from SetupPostProcessingShader

### Category D: Needs Investigation
**Action**: Deep dive required

**Systems**:
- `ExportMenuSnapshotSystem` - used directly, not registered
- `TooltipSystem` - check if different from RenderTooltipSystem
- `PostProcessingSystems.h` - header file, check contents

## Verification Execution Plan

### Step 1: Fix Script False Positives
Update `find_unused_systems.sh` to:
- Parse render system registrations
- Check helper function implementations
- Exclude utility/header files

### Step 2: Component Dependency Analysis
For each Category C system:
- Check if unique components exist
- Check if components used elsewhere
- Document component usage

### Step 3: Planning Document Review
For each Category C system:
- Search all planning documents
- Document findings
- Create recommendation

### Step 4: Code Reference Analysis
For Category D systems:
- Full codebase search
- Check test files
- Check if intentionally used directly

