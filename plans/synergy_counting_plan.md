# Synergy Counting Plan

## Overview

This plan documents the current state of synergy counting system and identifies any gaps or enhancements needed. **Note**: According to `next_todo.md`, tags and SynergyCountingSystem are already complete. This plan serves as documentation and verification.

## Current State (Already Complete)

### Existing Components

- ✅ `CourseTag` - exists in `src/components/course_tag.h`
- ✅ `CuisineTag` - exists in `src/components/cuisine_tag.h`
- ✅ `BrandTag` - exists in `src/components/brand_tag.h`
- ✅ `DishArchetypeTag` - exists in `src/components/dish_archetype_tag.h`
- ✅ `SynergyCounts` - exists in `src/components/synergy_counts.h` (singleton)

### Existing Systems

- ✅ `SynergyCountingSystem` - exists in `src/systems/SynergyCountingSystem.h`
  - Counts tags in shop (inventory items)
  - Updates `SynergyCounts` singleton
  - Runs on Shop screen

- ✅ `BattleSynergyCountingSystem` - exists in `src/systems/BattleSynergyCountingSystem.h`
  - Counts tags in battle (dishes in queue)
  - Updates `BattleSynergyCounts` singleton
  - Runs on Battle screen

### UI Integration

- ✅ Tooltips display tags and synergy counts (from `next_todo.md` line 15)

## Verification Tasks

Since the system is marked complete, this plan focuses on verification:

1. **Verify Tag Components Exist**
   - Check all tag component files exist
   - Verify tag types are properly defined

2. **Verify SynergyCountingSystem Works**
   - Test that counts update correctly when dishes added/removed
   - Test that counts reset properly
   - Test that counts are accessible from UI

3. **Verify UI Integration**
   - Check tooltips show tags
   - Check tooltips show synergy counts
   - Verify counts update in real-time

## Potential Enhancements (Future)

If gaps are found during verification:

1. **Battle-Time Synergy Counting**
   - Ensure `BattleSynergyCountingSystem` counts correctly during battle
   - Verify counts update as dishes enter/exit combat

2. **Threshold Tracking**
   - Verify threshold tracking works (2/4/6)
   - Check threshold data structures in `SynergyCounts`

3. **UI Hooks**
   - Verify tooltips read from `SynergyCounts` singleton
   - Check overlay systems can access counts

## Validation Steps

1. **Verify Existing Implementation**
   - Review `SynergyCountingSystem.h` code
   - Review `SynergyCounts.h` structure
   - Test tag counting in shop

2. **Test UI Integration**
   - Verify tooltips show tags
   - Verify tooltips show synergy counts
   - Test real-time updates

3. **Document Any Gaps**
   - If issues found, document and create follow-up tasks
   - If working correctly, mark as verified

## Success Criteria

- ✅ All tag components exist and are properly defined
- ✅ SynergyCountingSystem counts tags correctly
- ✅ UI displays tags and synergy counts
- ✅ System is ready for set bonus implementation (Task 5)

## Estimated Time

1-2 hours (verification only, since system is complete):
- 30 min: Code review and verification
- 30 min: Testing tag counting
- 30 min: UI verification
- 30 min: Documentation

