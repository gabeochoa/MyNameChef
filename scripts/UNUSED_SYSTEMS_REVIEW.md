# Unused Systems Review - Final List

## Summary
- **Total Systems**: 66
- **Registered**: 46
- **Actually Unused**: 9 (excluding utilities)
- **Utilities**: 3 (exclude from removal)
- **Needs Investigation**: 7

## 7 Verification Methods Used

1. ✅ **Direct Registration** - grep main.cpp for make_unique
2. ✅ **Helper Function Registration** - extract from register_* functions
3. ✅ **Include File Check** - check if header included
4. ✅ **Component Dependency** - check if unique components exist
5. ✅ **Planning Document Search** - search planning docs
6. ✅ **Code Reference Check** - grep entire codebase
7. ⚠️ **File Content Analysis** - manual review

## 9 Actually Unused Systems (Need Planning Doc Review)

### Confirmed Dead Code:
1. **BeginPostProcessingShader** - Separate file, not registered (SetupPostProcessingShader is used instead)
2. **DishGenerationSystem** - Not registered, generation infrastructure
3. **ProcessDishGenerationSystem** - Not registered, generation infrastructure
4. **ProcessCollisionAbsorption** - Not registered, uses CollisionAbsorber (component doesn't exist)
5. **UpdateTrackingEntities** - Not registered, uses TracksEntity (component doesn't exist)
6. **RenderAnimationsWithShaders** - Not registered
7. **RenderEntities** - Not registered
8. **RenderDebugGridOverlay** - Not registered
9. **RenderPlayerHUD** - Not registered

### Legacy System:
10. **JudgeAnimations** - Legacy judging (HEAD_TO_HEAD_COMBAT_PLAN.md Phase 0 removal)

## 7 Systems Needing Investigation

1. **EnterAnimationSystem** - Included, duplicate of BattleEnterAnimationSystem?
2. **UnifiedAnimationSystem** - Included but not registered
3. **TooltipSystem** - Included, check if same as RenderTooltipSystem
4. **TagShaderRender** - Included, systems inside are registered
5. **RenderSystemHelpers** - Header file, systems inside are registered
6. **PostProcessingSystems** - Header file, systems inside are registered
7. **ExportMenuSnapshotSystem** - Used directly (not registered), intentional?

## 3 Utility Files (Not Systems - Exclude from Removal)

- **LetterboxLayout** - Utility struct
- **PausableSystem** - Base class template
- **PostProcessingSystems** - Header file (not a system itself)

## Next Steps for Review

### Step 1: Planning Document Review
For each of the 9 unused systems, search:
```bash
grep -i "SystemName" HEAD_TO_HEAD_COMBAT_PLAN.md next_todo.md TODO.md FEATURE_STATUS.md
```

### Step 2: Component Check
- Verify CollisionAbsorber and TracksEntity components don't exist
- Check if they're planned for future features

### Step 3: File Content Review
- Read each unused system file
- Understand purpose and dependencies
- Determine if intentionally unused

### Step 4: Create Removal Recommendations
Categorize each as:
- **Remove Now**: Confirmed dead code, not in planning docs
- **Remove Later**: Part of planned cleanup (e.g., legacy judging)
- **Keep**: Needed for future features
- **Investigate**: Unclear status, needs more research

