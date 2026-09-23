# Feature Status Table

Refreshed September 2026 against current code (issue 99). Status means code exists and is registered, with tests where noted.

| Feature | Status | Evidence |
|---------|---------------|----------|
| Core Dish System | Implemented | `IsDish`, `DishType`, `FlavorStats`, dish info registry |
| Shop System | Implemented | 7 slots, wallet, purchase/sell, tier generation, `InitialShopFill`, `GenerateShopSlots`, `RerollCost`, `Freezeable` |
| Inventory System | Implemented | 7 slots, drag and drop, `GenerateInventorySlots` |
| Level System | Implemented | `DishLevel` with merge_progress (saved/restored), level scaling `2^(level-1)` capped at `MAX_DISH_LEVEL=10` |
| Deterministic RNG | Implemented | `SeededRng` singleton (mt19937_64), shop/battle seeds, RNG state saved in `rngState` |
| Combat System | Implemented | Simultaneous bites with pre/post pause, survivor carryover, `ComputeCombatStatsSystem`, `ResolveCombatTickSystem`, `AdvanceCourseSystem`. See COMBAT_IMPLEMENTATION_STATUS.md |
| Pairings and Clashes | Implemented | `ApplyPairingsAndClashesSystem`, `PairingClashModifiers` |
| Tags and Synergies | Implemented | Cuisine/course tags on dishes, `SynergyCounts`, `BattleSynergyCounts`, `SynergyCountingSystem`, `BattleSynergyCountingSystem`, set bonuses via `ApplySetBonusesSystem` (2/4/6 piece) |
| Trigger/Effect System | Implemented | `TriggerQueue`, `TriggerDispatchSystem` (strict weak ordering), `EffectResolutionSystem`, hooks OnServe/OnStartBattle/OnCourseStart/OnBiteTaken/OnDishFinished/OnCourseComplete, drink effects |
| Battle Results/Reports | Implemented | `BattleResult`, `SaveBattleReportSystem` writes `output/battles/results/*.json` (`outcomes` schema), `LoadBattleResults`, history screen |
| Replay | Partial | `ReplayState`, pause/play, speed, restart reseeds RNG. Progress total set at processor finish |
| Menu Snapshot Export | Implemented | `ExportMenuSnapshotSystem` exports slot, dishType, level, drink |
| Server Battle/Save | Partial | HTTP `/battle`, `/save-game-state`, `/game-state` exist with validation and durable save file. Production Next Round flow uses the local battle (issue 6 deferred), auth is absent (issue 11 deferred) |
| Judge Scoring | Not implemented, deferred by design | Replaced by head-to-head combat |

## Open items

Priorities live in CODE_REVIEW_100_ISSUES.md, status appendix. Rebuilding items listed as Implemented here is wasted work.
