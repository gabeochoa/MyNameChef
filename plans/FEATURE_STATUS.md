# Feature Status Table

| Feature | Current Status | Next State | Longer Term Items |
|---------|---------------|------------|-------------------|
| **Core Dish System** | ✅ `IsDish`, `DishType` enum, `FlavorStats` (7 axes), dish info registry | Minor fixes/cleanup | Full tag system: `CourseTag`, `CuisineTag`, `BrandTag`, `DietaryTag`, `DishArchetypeTag`, `RarityTier`; replace static dish pool with `magic_enum::enum_values<DishType>()` |
| **Shop System** | ✅ 7 slots, wallet, purchase/sell, tier-based generation, `InitialShopFill`, `GenerateShopSlots` | Add `SeededRng` singleton, seed in `ExportMenuSnapshotSystem`, replace `std::random_device` usage with seeded RNG | Full shop economy: `RerollCost` component with incrementing costs, pack rotations, weekly meta rotations |
| **Inventory System** | ✅ 7 slots, drag-and-drop, slot management, `GenerateInventorySlots`, purchase from shop | UX improvements (price display, drop target highlights) | Advanced inventory: filters, sorting, search, expanded inventory size |
| **Level System** | ✅ `DishLevel` component with merge progress (2 merges per level), level scaling in combat (2x per level), merge logic in `DropWhenNoLongerHeld` | Fix level calculation bug (currently uses 2^(level-1) instead of simple 2x), validate merge triggers correctly | Combine 3 duplicates into 1 (not just 2), alternative merge mechanics, level cap system |
| **Reroll/Freeze** | ⚠️ Reroll button exists (costs 5 gold, hardcoded), uses `std::random_device` (non-deterministic) | Add `RerollCost{ base=1, increment=0 }` component, `Freezeable{ isFrozen }` component, replace random calls with `SeededRng`, `RerollFreezeSystem` to handle frozen items | Incrementing reroll costs, freeze badges/UI, freeze count limits |
| **Deterministic RNG** | ❌ Not implemented (TODO mentions it, tests expect it) | Add singleton `SeededRng{ std::mt19937_64 gen; uint64_t seed; }`, seed it in `ExportMenuSnapshotSystem` using snapshot seed, replace all `std::random_device` calls | Server-authoritative seed distribution, replay consistency validation, seed debugging tools |
| **Combat System (SAP-style)** | ✅ Head-to-head dish vs dish, Zing/Body from FlavorStats, course-by-course resolution (7 courses), alternating bites, `CombatStats`, `DishBattleState` phases, `ComputeCombatStatsSystem`, `ResolveCombatTickSystem`, `AdvanceCourseSystem` | Fix level scaling bug (2^(level-1) vs 2x), validate Zing/Body formula matches TODO.md spec exactly | Advanced combat: status effects, trigger hooks during combat, combat modifiers mid-battle |
| **Pre-Battle Modifiers** | ✅ `PreBattleModifiers` component exists (`zingDelta`, `bodyDelta`), used in `ComputeCombatStatsSystem` | Implement `ApplyPairingsAndClashesSystem` to populate `PreBattleModifiers` based on team composition | Full pairing/clash rule system: adjacency rules, global team modifiers, rule library, JSON-driven rules |
| **Pairings and Clashes** | ❌ No system exists (TODO mentions simple rules) | Add `ApplyPairingsAndClashesSystem`: scan team for pairings/clashes, apply +Body/-Zing modifiers, write to `PreBattleModifiers` | JSON-driven pairing/clash library, complex adjacency rules, sequence bonuses, aura effects, data-driven content |
| **Tags and Synergies** | ❌ No tag components exist (CourseTag, CuisineTag, etc.), no synergy counting | Add tag components (`CourseTag`, `CuisineTag`, `BrandTag`, `DietaryTag`, `DishArchetypeTag`), add `SynergyCounts` singleton, implement `SynergyCountingSystem` (display-only counts) | Set bonuses (2/4/6 thresholds), synergy UI with tooltips, set bonus effects on dishes, weekly meta tag bans |
| **Battle Results** | ✅ `BattleResult` component with course outcomes, match results (wins/losses/ties), `BattleProcessorSystem` records outcomes | Add `BattleReport` component for persistence, write results to `output/battles/results/*.json`, add replay button on Results screen | Full replay system: `ReplayState`, `ReplayControllerSystem`, `ReplayUISystem`, scrub timeline, pause/speed controls, event log playback |
| **Battle Report Persistence** | ❌ Not implemented (component exists conceptually) | Create `BattleReport{ opponentId, seed, perCourse[], totals }` component, write JSON on Results screen, basic file persistence | Full replay system with event logs, server-side persistence, replay viewer with UI controls, battle history |
| **Zing/Body UI** | ✅ `RenderZingBodyOverlay` shows Zing (green rhombus) and Body (yellow square) with numbers | Validate rendering matches TODO.md spec exactly (top-left Zing, top-right Body, 2-digit support) | Enhanced UI: animations on stat changes, stat tooltips, stat history display |
| **Tooltip System** | ✅ Basic tooltips exist, shows dish info | Update tooltips to show tags (when implemented), show synergy counts (when implemented), group flavor stats into Zing/Body categories | Rich tooltips: stat comparisons, synergy breakdowns, pairing/clash indicators, effect descriptions |
| **Judge Profile & Scoring** | ❌ Not implemented (deferred per TODO.md) | Deferred - replaced by SAP-style combat | Judge profiles with weights, `NormalizationSystem` (f(x)=x/(x+k)), `HarmonySystem` (1-stddev), judge-weighted base scoring, multiple judge types |
| **Data-Driven Effects** | ❌ No effect system exists | Deferred - keep minimal hooks for now | JSON-driven effect library: `EffectList`, `Trigger` hooks (OnServe, OnTaste, OnPair, etc.), `EffectResolutionSystem`, effect op system (AddStat, AddBonus, etc.) |
| **Trigger System** | ⚠️ Basic trigger hooks mentioned in code (`onServe` in `DishInfo`), `TriggerQueue` component exists | Implement `TriggerDispatchSystem` for OnServe/OnTaste hooks, basic effect resolution | Full trigger system: all hook types (OnServe, OnTaste, OnPair, OnSequence, OnCourseComplete, etc.), effect queue processing |
| **Menu Snapshot Export** | ✅ `ExportMenuSnapshotSystem` exports player inventory to JSON with seed, creates `BattleLoadRequest` | Replace `std::random_device` with `SeededRng` singleton, store seed before transition to Battle | Enhanced snapshots: include all modifiers, team metadata, version tracking, network serialization |
| **Battle Loading** | ✅ `BattleLoadRequest` component, `BattleTeamLoaderSystem` loads teams from JSON | Validate loading includes all necessary components, ensure determinism | Network battle loading, opponent discovery, async battle resolution, server-side validation |
| **Animation System** | ✅ Slide-in animations, bite timing, enter animations, `DishBattleState` tracks animation phases | Validate animation timing matches combat tick cadence | Enhanced animations: bite effects, victory/defeat animations, transition effects, particle effects |
| **Rendering Systems** | ✅ Multi-pass rendering, battle team rendering, Zing/Body overlays, progress bars, HUD systems | Minor polish and validation | Advanced rendering: shader effects, post-processing, visual feedback on stat changes, enhanced battle visualization |

## Implementation Priority Recommendations

### High Priority (Foundation)
1. **SeededRng** - Required for determinism and replay
2. **ApplyPairingsAndClashesSystem** - Core gameplay mechanic, component exists but unused
3. **Replace random_device with SeededRng** - In shop generation and menu export

### Medium Priority (Features)
4. **RerollCost and Freezeable** - Improve shop UX
5. **Tags and SynergyCountingSystem** - Foundation for set bonuses
6. **BattleReport persistence** - Enable replay system
7. **Fix level scaling bug** - Current 2^(level-1) may be too aggressive

### Low Priority (Polish)
8. **Replace static dish pool** - Code cleanup
9. **Enhanced tooltips** - UX improvement
10. **Animation polish** - Visual refinement

### Deferred (Per TODO.md)
- Judge profile and normalization systems (replaced by combat)
- Full data-driven effect system (keep minimal hooks for now)
- Complex trigger system (basic hooks only)

