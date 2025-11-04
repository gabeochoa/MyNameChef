# System Verification Methods - Complete Guide

## 7 Verification Methods

### Method 1: Direct Registration Check ✅
**Command**: `grep "make_unique<SystemName>" src/main.cpp`
**What it checks**: Systems registered directly in main.cpp
**Status**: Working - detects 40+ systems

### Method 2: Helper Function Registration ✅
**Command**: Extracts from `register_*_systems()` functions
**Files checked**:
- `src/shop.cpp` - `register_shop_update_systems()`
- `src/sound_systems.cpp` - `register_sound_systems()`
- `src/ui/ui_systems.cpp` - `register_ui_systems()`
**Status**: Working - detects GenerateShopSlots, GenerateInventorySlots, ScreenTransitionSystem, UI systems

### Method 3: Include File Check ✅
**Command**: `grep "#include.*System.h" src/main.cpp`
**What it checks**: Systems whose headers are included
**Special handling**: Checks if defined in PostProcessingSystems.h or RenderSystemHelpers.h
**Status**: Working - identifies systems that are included but may not be registered

### Method 4: Component Dependency Check ✅
**Command**: `grep "ComponentName" src/components/*.h`
**What it checks**: If systems use unique components that don't exist
**Systems checked**:
- ProcessCollisionAbsorption → CollisionAbsorber
- UpdateTrackingEntities → TracksEntity
**Status**: Working - identifies systems with missing components

### Method 5: Planning Document Search ✅
**Command**: `grep -i "SystemName" HEAD_TO_HEAD_COMBAT_PLAN.md next_todo.md TODO.md`
**What it checks**: If system is mentioned in planning documents
**Status**: Working - identifies planned systems

### Method 6: Code Reference Check ✅
**Command**: `grep -r "SystemName" src/`
**What it checks**: If system is referenced anywhere in codebase
**Status**: Working - identifies indirect references

### Method 7: File Content Analysis ⚠️
**Action**: Manually read system file
**What it checks**: System purpose, base class vs concrete, utility vs system
**Status**: Manual process required

## Verified Unused Systems (11 total)

### Confirmed Dead Code (Need Planning Doc Review):
1. **BeginPostProcessingShader** - Separate file, not registered (different from SetupPostProcessingShader)
2. **DishGenerationSystem** - Not registered, generation infrastructure
3. **ProcessDishGenerationSystem** - Not registered, generation infrastructure
4. **ProcessCollisionAbsorption** - Not registered, uses CollisionAbsorber (component doesn't exist)
5. **UpdateTrackingEntities** - Not registered, uses TracksEntity (component doesn't exist)
6. **RenderAnimationsWithShaders** - Not registered
7. **RenderEntities** - Not registered
8. **RenderDebugGridOverlay** - Not registered
9. **RenderPlayerHUD** - Not registered

### Legacy Systems (Need Planning Doc Review):
10. **JudgeAnimations** - Legacy judging system (HEAD_TO_HEAD_COMBAT_PLAN.md Phase 0)

### Utility Files (Not Systems):
- **LetterboxLayout** - Utility struct, not a system
- **PausableSystem** - Base class template, not concrete system
- **PostProcessingSystems** - Header file containing multiple systems

## Systems Needing Investigation (7 total)

1. **EnterAnimationSystem** - Included but not registered (duplicate of BattleEnterAnimationSystem?)
2. **UnifiedAnimationSystem** - Included but not registered
3. **TooltipSystem** - Included but registered as RenderTooltipSystem?
4. **TagShaderRender** - Included but systems inside are registered
5. **RenderSystemHelpers** - Header file with helper systems
6. **PostProcessingSystems** - Header file with multiple systems
7. **ExportMenuSnapshotSystem** - Used directly, not registered (intentional?)

## Next Steps

1. Review 10 unused systems against planning docs
2. Investigate 7 systems that are included but usage unclear
3. Determine if utility files should be excluded from checks

