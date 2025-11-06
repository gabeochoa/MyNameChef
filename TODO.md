# COMPLETED FEATURES

## Core Game Systems ✅
- **Basic dish system**: `IsDish` component with `DishType` enum, `FlavorStats` with 7 flavor axes (satiety, sweetness, spice, acidity, umami, richness, freshness)
- **Dish types**: 30+ dishes implemented with tier system (1-5), unified pricing, sprite locations
- **Shop system**: Basic shop with 7 slots, wallet system, purchase/sell functionality
- **Inventory system**: 7-slot inventory with drag-and-drop, slot management
- **Level system**: `DishLevel` component with merge progress tracking (2 merges needed per level)
- **Wallet and health**: Singleton components with gold/health management, battle rewards

## Combat System ✅
- **SAP-style combat**: Head-to-head dish vs dish combat with Zing (attack) and Body (health)
- **Combat stats**: `CombatStats` component with base/current Zing and Body values
- **Battle phases**: `DishBattleState` with InQueue, Entering, InCombat, Finished phases
- **Course-by-course resolution**: 7-course battles with alternating bite mechanics
- **Combat systems**: `InitCombatState`, `ComputeCombatStatsSystem`, `StartCourseSystem`, `ResolveCombatTickSystem`, `AdvanceCourseSystem`
- **Battle results**: `BattleResult` with course outcomes and match results
- **Battle animations**: Slide-in animations, bite timing, visual feedback

## Rendering and UI ✅
- **Multi-pass rendering**: Main render texture, shader support, post-processing
- **Sprite system**: Texture management with spritesheet support
- **Render ordering**: `HasRenderOrder` component with screen-specific rendering
- **Battle team rendering**: Specialized `RenderBattleTeams` system
- **Zing/Body overlays**: `RenderZingBodyOverlay` showing combat stats
- **Progress bars**: `RenderDishProgressBars` for health visualization
- **HUD systems**: Wallet display, health display, FPS counter
- **Tooltip system**: Context-sensitive tooltips
- **UI navigation**: Screen transitions (Shop → Battle → Results)

## Game State Management ✅
- **Screen management**: Shop, Battle, Results screens with proper transitions
- **Entity cleanup**: Systems for cleaning up battle, shop, and dish entities
- **Singleton management**: Proper singleton patterns for global state
- **Input handling**: Drag-and-drop, UI interactions

## Technical Infrastructure ✅
- **ECS architecture**: Component-system design with proper inheritance
- **Animation system**: Integration with afterhours animation plugin
- **Sound system**: Basic sound integration
- **Settings system**: Configuration management
- **Build system**: xmake integration, proper compilation

---

# REMAINING TASKS

- Course/tags and synergy counts (display-only)
  - Components on dishes: `CourseTag`, `CuisineTag`, `BrandTag`, `DietaryTag`, `DishArchetypeTag`.
  - System `SynergyCountingSystem`: scan current inventory once per frame on Shop, compute counts per tag set into `SynergyCounts` singleton for UI.
  - Update tooltips to show tags and current set thresholds reached.

- Judge profile and base scoring (no pairings yet)
  - Singletons: `JudgeProfileRef{ weights[7] }`, `ScoringConfig{ k }`.
  - Systems: `NormalizationSystem` (compute `ComputedFlavor` via `f(x)=x/(x+k)` and weighted base), `HarmonySystem` (compute `HarmonyScore = 1 - stddev(normalized)`), store per-dish for UI.
  - Use these to compute player/opponent totals deterministically in Results (no random variation).

- Persist `BattleReport` for replay
  - Component `BattleReport{ opponentId, seed, perCourse[], totals }`.
  - On Results, write JSON to `output/battles/results/*.json` using the same seed; load it on replay to reproduce scores and UI.

- Basic pairing rule to start the system
  - Add `PairingAndClashSystem` scaffolding with a single rule: `CourseTag=Drink` adjacent to `Meat` adds small +umami multiplier.
  - Compute per-course additive/multiplicative bonuses (`AppliedBonuses`) and include in final score.

- Combine duplicates into levels
  - Component `Level{ value }` on dishes.
  - System `CombineDuplicates`: when 3 of the same `DishType` are in inventory, merge into one entity with `Level+1` and boosted stats/cost; remove the others.

- Small UX affordances
  - Show price on shop items; show "frozen" badge, and a faint highlight on legal drop targets.
  - In Results, add a "Replay" button once `BattleReport` exists.

- Zing and Body (SAP-style, simple)
  - Zing (attack): [spice ≥ 1] + [acidity ≥ 1] + [umami ≥ 1] → 0–3
  - Body (health): [satiety ≥ 1] + [richness ≥ 1] + [sweetness ≥ 1] + [freshness ≥ 1] → 0–4
  - Examples: FrenchFries → Zing 0, Body 2; Salmon → Zing 3, Body 2
  - Keep synergy (types/pairings/sets) as multipliers/addends applied after base counts; do not modify base Zing/Body.
  - Level scaling: if `Level.value > 1`, multiply both Zing and Body by 2 (post-base, pre-synergy).

- SAP-style combat (replaces judge score model)
  - Head-to-head resolve: front dish vs front dish. Each "bites" the other: Body is reduced by opponent Zing until one hits 0.
  - Events: `OnBiteTaken` fired on each damage tick; `OnDishFinished` fired when Body reaches 0.
  - Round outcome: last team with any dish standing wins. Later we may add a simple "menu quality" tiebreaker.
  - Pairings/clashes as global modifiers (pre-battle):
    - Pairings (e.g., Bread↔Soup) grant +1 Body to all dishes on your team for this battle.
    - Clashes (e.g., Acid↔Dairy) apply −1 Zing to all dishes on your team for this battle.
    - Keep magnitudes tiny and readable; apply after Level scaling.
  - Triggers: keep trigger hooks, but exact trigger list to be decided later.
  - Visualization: serve both dishes simultaneously to a two-headed judge who alternates bites from each; each bite maps to a damage tick until one dish's Body reaches 0. Keep the animation cute and readable.

- Z/B UI and tooltip updates
  - Overlay: draw Zing as a green rhombus with a number (supports 2 digits) at top-left of the sprite.
  - Overlay: draw Body as a pale yellow square with a number at top-right of the sprite.
  - Tooltip grouping: group flavor stats into "Zing stats" (spice, acidity, umami) and "Body stats" (satiety, richness, sweetness, freshness).

- Plan adjustments and deferrals
  - Deferred: `NormalizationSystem`, `HarmonySystem`, `ComputedFlavor`, and judge-weighted base scoring (replaced by SAP-style combat).
  - Deferred: detailed `EffectResolutionSystem` and large JSON-driven effect library; keep minimal hooks only for now.
  - Note: "Scoring sketch (for model 2)" and related judge UI are obsolete for the current gameplay direction.


# TODO
Food-themed async autobattler — ECS design notes

Core loop (async):
- Shop phase: draft 7-course menu from a shop, reroll/freeze, apply foods/techniques.
- Lock-in menu: produce a deterministic battle seed and snapshot.
- Resolve vs opponent asynchronously (course-by-course tasting). Return a match report and rating change.

ECS components (proposed):
- Identity/placement
  - `MenuSlot{ index: 1..7 }` — logical position in the tasting order (left-to-right = appetizer➜dessert).
  - `OwnedBy{ chefId }` — associates entities to a player.
  - `ShopItem{ price, isFrozen, originPack, tier }` — item sitting in shop inventory.
  - `CourseSlotConstraint{ allowedCourses: bitset }` — optionally enforce canonical 7-course template.
- Dish definition
  - `Dish` — tag to mark entities that are selectable menu items.
  - `CourseTag{ course: Appetizer|Soup|Fish|Meat|Drink|PalateCleanser|Dessert|Mignardise }` — `Drink` explicit; keep `Dessert` and `Mignardise` distinct.
  - `CuisineTag{ cuisines: bitset }` — Thai, Italian, Japanese, Mexican, French, etc.
  - `BrandTag{ brands: bitset }` — McDonalds, LocalFarm, StreetFood, etc.
  - `DietaryTag{ tags: bitset }` — Vegan, GlutenFree, Halal, Kosher, Keto.
  - `DishArchetypeTag{ tags: bitset }` — Bread, Dairy, Beverage, Wine, Coffee, Tea, Side, Sauce; drives pairings/clashes.
  - `RarityTier{ tier: 1..6 }` — unlock cadence and shop odds.
  - `FlavorStats{ satiety, sweetness, spice, acidity, umami, richness, freshness }` — ints (0..n), balance axes.
  - `TechniqueStats{ smoke, ferment, fry, roast, sousVide, raw }` — technique intensities.
- Triggers/effects (SAP-like power system)
  - `Trigger{ hook: OnBuy|OnSell|OnPlace|OnAdjacentChange|OnCourseServed|OnCourseResolved|OnTeamSynergyChanged|OnReroll }`.
  - `EffectList{ effects: [EffectId...] }` — references data-driven effect definitions.
  - `Aura{ radius: 1|global, statMods, tagMods }` — passive buffs to neighbors/team.
  - `StatusEffect{ name, stacks, durationCourses }` — e.g., PalateCleanser, Heat, SweetTooth, Fatigue.
  - `OneShot{ consumed }` — single-use (e.g., flambé showpiece).
- Synergy accounting
  - `SynergyCounts{ byCuisine[], byCourse[], byBrand[], byDietary[], byTechnique[] }` — computed each phase.
  - `SetBonus{ id, tierReached, bonusApplied }` — e.g., All Thai 2/4/6 thresholds.
- Evaluation caches (ephemeral per evaluation)
  - `ComputedFlavor{ normalized[7] }` — normalized flavor stats for scoring/UI.
  - `HarmonyScore{ value }` — 1 − stddev of normalized stats.
  - `AppliedBonuses{ additive, multiplicative }` — rollup from sets, pairings, clashes, auras, sequences.
- Shop/phase/state
  - `Wallet{ gold }`, `RerollCost{ base, increment }`, `Freezeable`.
  - `ShopInventory{ slots: [entityIds] }` — references `ShopItem` or `Dish` entities.
  - `PhaseState{ phase: Shop|LockIn|Resolving, turnIndex }`.
  - `SeededRng{ seed }` — ensures determinism across async resolve.
- Match/world context
  - `JudgeProfileRef{ id }`, `ScoringConfig{ k, weights[7] }` — controls normalization and flavor weights.
  - `WeeklyMeta{ judgeProfileId, bannedTags[], packRotationId }`, `SeasonTag{ season }`.
- Battle result
  - `CourseScore{ slotIndex, scoreBreakdown }`, `MatchScore{ total, deltas }`.
  - `BattleReport{ opponentId, seed, outcome }` — persisted for replay.


  drink pairings are how you can boost your dish without replacing it. 
  also we need to add levels so you can combine multiple of the same dish to upgrade the level 
  combile keyboard and controlx into a single sheet 
  replace dish pools with enum_values

## Toast System (not needed at the moment)
- Translation support: Add toast message keys to `strings::i18n` enum (`server_disconnected`, `server_reconnected`), update `ToastMessage` to support translatable strings, update `make_toast()` to accept `strings::i18n` keys or `TranslatableString`, add translations in `translation_manager.cpp`
- Toast stacking/positioning: Multiple toasts may overlap, add vertical stacking (offset each new toast upward) or queue system to show one at a time
- Toast types/styles: Different colors/styles for different message types (error, warning, info, success), add `ToastType` enum and update rendering accordingly
- Additional toast usage: Add toasts for other game events (battle won/lost, purchase success, etc.)
- Polish: Sound effects for toast appearance, different animation styles (slide from different directions), configurable positioning (top, bottom, center, etc.)


ECS systems (proposed):
- Phase & shop
  - `PhaseSystem` — drives Shop→LockIn→Resolving.
  - `ShopGenerationSystem` — populate shop per `turnIndex`, `RarityTier`, and packs.
  - `PurchaseSystem` — spend `Wallet`, move `Dish` from shop to `MenuSlot`.
  - `SwapPlaceSystem` — reorder dishes, enforce 7-slot constraint.
  - `RerollFreezeSystem` — reroll shop using `SeededRng`, toggle freeze.
  - `ConstraintValidationSystem` — enforce `CourseSlotConstraint` on placement/swap.
  - `WeeklyMetaSystem` — apply bans/weights/pack rotations from `WeeklyMeta`.
- Team evaluation
  - `NormalizationSystem` — compute `ComputedFlavor` using `ScoringConfig.k` and judge weights.
  - `HarmonySystem` — compute `HarmonyScore` per dish.
  - `SynergyCountingSystem` — compute `SynergyCounts` from current menu.
  - `AuraApplicationSystem` — apply `Aura` to neighbors/team; generate temporary stat mods.
  - `PairingAndClashSystem` — evaluate pairing/clash rules to modify `AppliedBonuses`.
  - `SequenceBonusSystem` — evaluate chain rules (e.g., Heat Crescendo) and modify `AppliedBonuses`.
  - `BonusAggregationSystem` — merge set, adjacency, pairings, clashes, sequences, and auras into `AppliedBonuses`.
  - `TriggerDispatchSystem` — fire `Trigger` hooks and enqueue effects.
  - `EffectResolutionSystem` — deterministic queue runner for data-driven effects.
- Battle/resolve
  - `CoursePairingSystem` — align slot i vs slot i, inject `OnCourseServed` hooks.
  - `ScoringSystem` — compute per-course and total using explicit scoring flow.
  - `JudgeSummarySystem` — produce readable report and deltas (ELO/health/hearts).
  - `PersistenceSystem` — store `BattleReport` and allow replay.
- UI glue (later)
  - `ShopUISystem`, `MenuLayoutSystem` (7-slot line), `ReportUISystem` (surface `ComputedFlavor`, `HarmonyScore`, `AppliedBonuses`).

Battle/comparison models (choose one to start):
1) SAP-style “attack/health” analogue (tactical, order matters)
   - Interpret `FlavorStats` to derived `punch` (offense) and `stability` (defense).
   - On serve, a dish “hits” the opposing dish: spice/acidity reduce stability; sweetness/richness buffer.
   - Pros: emergent interactions; familiar. Cons: abstraction leap from food to combat.

2) Pairwise tasting score (recommended starting point)
   - Slot i vs slot i. Compute `courseScore = baseFlavorScore × synergyMultiplier + triggerBonuses − clashPenalties`.
   - Fire hooks: `OnCourseServed` (adjacency/pairing), `OnCourseResolved` (chain effects to next slot).
   - Pros: very readable, food-first fantasy, easier to balance.

3) Judge panel weighting
   - Three judges with different weights (e.g., Satiety/Technique/Novelty). Total = weighted average.
   - Pros: variety; weekly metas. Cons: more numbers to surface.

Judging System Implementation:
- Three celebrity chef judges (references to real chefs) will evaluate dishes
- Layout: [Your Team] [Judges] [Opponent Team] - arranged left to right
- Animation: Feed judges one dish at a time, going left to right through the 7-course menu
- Scoring visualization: Bar along left column with handle that moves:
  - Down away from your team if opponent is winning after that dish
  - Up towards your team if you are winning after that dish
- Winner determination: Final handle position after all dishes determines winner
- Victory animation: Screen pushes off loser team and judges, revealing winner

Scoring sketch (for model 2):
- Normalization: `f(x) = x / (x + k)` using `ScoringConfig.k` per tier/season.
- Judge-weighted base: `base = Σ(weights[i] * f(stat[i]))` from `JudgeProfileRef`.
- Harmony addend: `harmony = 1 − stddev(normalizedStats)`; add scaled harmony to base.
- Additive/multiplicative bonuses: per dish, compute `AppliedBonuses{ additive, multiplicative }` from sets, adjacency pairings, sequences; clashes contribute negative multiplicative.
- Final per-dish: `(base + harmonyAdd + additive) × (1 + multiplicative)`.

Food synergies (examples):
- Set synergies (thresholds 2/4/6)
  - All Thai: +spice cap, +freshness; unlock “Herb Garden” trigger.
  - All Dessert: +sweetness scaling; “Sugar Rush” triggers at slot 6–7.
  - All McDonalds (brand): cheap, consistent; +satiety, −freshness.
  - Dietary: Vegan set grants freshness/harmony; Keto adds satiety/richness.
- Cross-course chains
  - Heat Crescendo: spice increases across slots → extra harmony points if monotonic.
  - Palette Arc: Rich→Acidic→Fresh sequence gives a cleansing bonus at 5th slot.
- Pairings/adjacency
  - Beverage pair: `CourseTag=Drink` next to `Meat` grants +umami and removes clash penalties.
  - Bread Service: `Raw` technique next to `Soup` yields satiety buffer.
- Technique sets
  - Two sous-vide dishes: +tenderness (stability); Add `OnCourseResolved` mini-buff.
- Seasonal/locale
  - Summer produce: +freshness; conflicts with `Ferment` unless mitigated by `CuisineTag=Korean`.

Data-driven content plan:
- Author dishes, triggers, and effects in JSON using `nlohmann/json`.
- Effect schema: `{ id, when: TriggerHook, ops: [ { op: "AddStat", stat: "spice", amount: 2 }, { op: "AddStatus", name: "PalateCleanser", durationCourses: 1 } ] }`.
- Dish schema: `{ name, tier, tags: { cuisine: ["Thai"], course: "Soup" }, flavor: {...}, technique: {...}, effects: ["thai_herbs_aura"] }`.
 - Judge profiles: `{ id, weights: { satiety, umami, sweetness, spice, acidity, richness, freshness }, k }`.
 - Pairing rules: `{ id, when: Adjacent|SameCourse|CrossSlot, require: { left: [tags], right: [tags] }, add: { mult:+0.03, add:+0.00 }, mitigatesClashIds: ["acid_dairy"] }`.
 - Clash rules: `{ id, when: Adjacent|Paired, require: { a: [tags], b: [tags] }, add: { mult:-0.05 } }`.
 - Sequence rules: `{ id, window: { start:1, end:4 }, stat: "spice", monotonic: "nondecreasing", add: { mult:+0.04 } }`.
 - Dish additions: include `archetypeTags: ["Bread", "Dairy", "Beverage", ...]`.

Implementation milestones:
1) Add components: `Dish`, `MenuSlot`, `CourseSlotConstraint`, `FlavorStats`, `CuisineTag`, `CourseTag`, `DishArchetypeTag`, `BrandTag`, `DietaryTag`, `RarityTier`, `Trigger`, `EffectList`, `Aura`, `Wallet`, `ShopItem`, `PhaseState`, `SeededRng`, `JudgeProfileRef`, `ScoringConfig`.
2) Implement `ShopGenerationSystem`, `PurchaseSystem`, `SwapPlaceSystem`, `RerollFreezeSystem`, `ConstraintValidationSystem`, `WeeklyMetaSystem` with deterministic RNG.
3) Implement evaluation core: `NormalizationSystem`, `HarmonySystem`, `SynergyCountingSystem`, `AuraApplicationSystem`.
4) Implement bonuses and scoring: `PairingAndClashSystem`, `SequenceBonusSystem`, `BonusAggregationSystem`, `CoursePairingSystem`, `ScoringSystem`, `JudgeSummarySystem`.
5) Author content: initial pack (20–30 dishes), 6–8 effects, 6–10 pairing/clash rules, 1–2 sequence rules, and 1 judge profile.
6) Wire `ReportUISystem` to display per-course breakdown and totals with breakdowns.

Async/determinism notes:
- All shop rolls and effect resolutions must come from `SeededRng` to allow replays and server-side validation.
- Persist `BattleReport{ opponentSnapshot, seed, events }` to reproduce outcomes.

Open questions to decide soon:
- Health model: hearts like SAP vs rating points vs tournament scoring?
- Rounds: fixed 7-course always, or variable with side pairings/beverages?
- Weekly metas: rotate judge weights, banned tags, or pack rotations.

Runnable incremental plan (10+ steps, each build runnable)
1) Basic buying and selling dishes
- Goal: Navigate a shop, buy dishes into an inventory, sell back for gold; no menu slots yet.
- Add: `Dish`, `ShopItem`, `ShopInventory`, `Wallet`, `PhaseState=Shop`, minimal `ShopGenerationSystem` (static items), `PurchaseSystem`, `SellSystem`.
- UI: Text/UI list of shop items and gold; on buy/sell, update counts; compile and run.

2) Menu placement and swapping
- Goal: Place bought dishes into 7 slots and reorder.
- Add: `MenuSlot`, `SwapPlaceSystem`, optional `CourseSlotConstraint` (warn-only).
- UI: Simple 7-slot strip; drag/drop or keyboard swap; compile and run.

3) Reroll and freeze with deterministic RNG
- Goal: Reroll shop and freeze items across turns with stable randomness.
- Add: `SeededRng`, `RerollFreezeSystem`, `RerollCost`.
- UI: Reroll button, freeze toggle badges; compile and run.

4) Lock-in snapshot and stub resolve
- Goal: Lock the menu and create a deterministic snapshot for battle; return a stub report.
- Add: `PhaseState=LockIn|Resolving`, snapshot serialization, `BattleReport` skeleton.
- UI: Lock-in button; resolver produces a placeholder per-course line; compile and run.

5) Tags and synergy counting (display-only)
- Goal: Track cuisines/courses/brands/dietary/archetypes and count set thresholds; no bonuses yet.
- Add: `CuisineTag`, `CourseTag` (includes `Drink`), `BrandTag`, `DietaryTag`, `DishArchetypeTag`, `SynergyCounts`, `SynergyCountingSystem`.
- UI: Show counts and thresholds reached; compile and run.

6) Normalization and harmony
- Goal: Compute judge-weighted base and harmony per dish (no multipliers yet).
- Add: `JudgeProfileRef`, `ScoringConfig`, `ComputedFlavor`, `HarmonyScore`, `NormalizationSystem`, `HarmonySystem`.
- UI: Per-course card shows base and harmony; compile and run.

7) Pairings and clashes
- Goal: Apply adjacency pair bonuses and clash penalties into per-dish bonuses.
- Add: `AppliedBonuses`, `PairingAndClashSystem`, JSON rules for initial pairings/clashes.
- UI: Breakdown shows +pairings and −clashes; compile and run.

8) Sequence bonuses and auras
- Goal: Add sequence rules (e.g., Heat Crescendo) and auras to modify bonuses.
- Add: `SequenceBonusSystem`, `AuraApplicationSystem`, extend `BonusAggregationSystem`.
- UI: Show sequence/auras contributions; compile and run.

9) Full scoring and course pairing
- Goal: Compute final per-course and total using the full flow.
- Add: `CoursePairingSystem`, complete `ScoringSystem`, `JudgeSummarySystem`.
- UI: Match report with per-course scores and total; compile and run.

10) Content pack v0
- Goal: Load 20–30 dishes and 6–8 effects from JSON into the shop.
- Add: JSON loading for dishes/effects; minimal `EffectList` usage; `EffectResolutionSystem` supporting a few ops.
- UI: Shop populated from content; compile and run.

11) Weekly meta and judges
- Goal: Rotate judge profiles, bans, and pack rotations; affect shop odds and scoring weights.
- Add: `WeeklyMetaSystem`, `SeasonTag`; integrate into shop gen and scoring.
- UI: Badge showing active meta/judge; compile and run.

12) Async resolve and replay
- Goal: Resolve against opponent snapshots with a shared seed and store full reports; add replay.
- Add: Deterministic resolve path, persistence for `BattleReport`, replay viewer.
- UI: Play a saved report to reproduce outcomes; compile and run.

Example menus and scoring (pairwise tasting model)

Assumptions for examples (for clarity, not final numbers):
- Normalization: f(x) = x/(x+3). Equal weights for 7 flavor stats.
- Harmony bonus: +0.10 × harmony (1 − stddev of normalized stats) added to base before multiplier.
- Synergy multiplier: 1 + set bonuses + adjacency pair bonuses − clash penalties.
  - Set bonuses (by cuisine thresholds): 2-piece +0.04, 4-piece +0.09, 6-piece +0.15.
  - Good adjacency pair (e.g., Soup next to Bread, Wine next to Meat): +0.03 each involved dish.
  - Clash (e.g., very acidic vs dairy, very spicy vs delicate dessert): −0.05 each involved dish.

Early game (Turn 3, Tiers 1–2)
- Menu (left→right):
  1) Garlic Bread [Appetizer, Italian] — satiety/richness leaning
  2) Tomato Soup [Soup, Italian] — acidity/freshness
  3) Grilled Cheese [Fish slot as simple Sandwich, StreetFood] — richness/satiety
  4) Chicken Skewer [Meat, StreetFood] — spice/umami
  5) Cucumber Salad [PalateCleanser, Vegan] — freshness
  6) Vanilla Soft Serve [Dessert] — sweetness/richness
  7) Mint Chocolate [Mignardise] — sweetness/freshness
- Synergies: 2× Italian → +0.04 team multiplier applied per Italian dish; Soup↔Bread adjacency +0.03 each; Soft Serve↔Mint adjacency +0.03 each.
- Per-course scoring (approx):
  1) Garlic Bread: base 0.19, harmony +0.06 → 0.25 × 1.03 = 0.26
  2) Tomato Soup: base 0.22, harmony +0.07 → 0.29 × 1.07 = 0.31  (Italian set +0.04, adjacency +0.03)
  3) Grilled Cheese: base 0.21, harmony +0.05 → 0.26 × 1.00 = 0.26
  4) Chicken Skewer: base 0.24, harmony +0.05 → 0.29 × 1.00 = 0.29
  5) Cucumber Salad: base 0.20, harmony +0.08 → 0.28 × 1.00 = 0.28
  6) Soft Serve: base 0.23, harmony +0.05 → 0.28 × 1.03 = 0.29
  7) Mint Chocolate: base 0.22, harmony +0.06 → 0.28 × 1.03 = 0.29
- Total ≈ 1.98

Mid game (Turn 7, Tiers 3–4)
- Menu (left→right):
  1) Caprese Salad [Appetizer, Italian] — freshness/acidity/umami
  2) Minestrone [Soup, Italian] — satiety/freshness
  3) Seared Salmon [Fish, Japanese] — umami/richness
  4) Steak Florentine [Meat, Italian] — satiety/umami/richness
  5) Lemon Sorbet [PalateCleanser] — acidity/freshness
  6) Tiramisu [Dessert, Italian] — sweetness/richness
  7) Espresso + Biscotti [Mignardise, Italian] — bitterness/sweetness (modeled in stats)
- Synergies: 4× Italian → +0.09 to Italian dishes; pairings: Salad↔Soup +0.03, Steak↔Red Wine (implicit beverage) treat as +0.03, Sorbet↔Dessert +0.03, Espresso↔Tiramisu +0.03.
- Per-course scoring (approx):
  1) Caprese: base 0.28, harmony +0.09 → 0.37 × 1.12 = 0.41  (set+adj)
  2) Minestrone: base 0.27, harmony +0.08 → 0.35 × 1.12 = 0.39
  3) Seared Salmon: base 0.30, harmony +0.07 → 0.37 × 1.00 = 0.37
  4) Steak Florentine: base 0.33, harmony +0.07 → 0.40 × 1.12 = 0.45
  5) Lemon Sorbet: base 0.26, harmony +0.10 → 0.36 × 1.03 = 0.37
  6) Tiramisu: base 0.31, harmony +0.06 → 0.37 × 1.12 = 0.41 (set+adj)
  7) Espresso+Biscotti: base 0.27, harmony +0.07 → 0.34 × 1.12 = 0.38
- Total ≈ 2.78

Late game (Turn 11, Tiers 5–6)
- Menu (left→right):
  1) Som Tum (Green Papaya Salad) [Appetizer, Thai] — spice/freshness/acidity
  2) Tom Yum [Soup, Thai] — spice/acidity/umami
  3) Pla Pao (Salt-Grilled Fish) [Fish, Thai] — umami/richness/freshness
  4) Massaman Curry [Meat, Thai] — richness/satiety/sweetness/spice
  5) Lychee Granita [PalateCleanser] — sweetness/freshness
  6) Mango Sticky Rice [Dessert, Thai] — sweetness/richness
  7) Thai Tea Panna Cotta [Mignardise, Thai] — sweetness/richness/tea bitterness
- Synergies: 6× Thai → +0.15 to Thai dishes; Heat Crescendo (1→4) yields +0.04 chain bonus across first four courses; beverage/dessert adjacency +0.03; minimal clashes due to cuisine coherence.
- Per-course scoring (approx):
  1) Som Tum: base 0.31, harmony +0.09 → 0.40 × 1.19 = 0.48 (set+chain)
  2) Tom Yum: base 0.33, harmony +0.08 → 0.41 × 1.19 = 0.49
  3) Pla Pao: base 0.34, harmony +0.08 → 0.42 × 1.19 = 0.50
  4) Massaman: base 0.37, harmony +0.07 → 0.44 × 1.19 = 0.52
  5) Lychee Granita: base 0.29, harmony +0.10 → 0.39 × 1.03 = 0.40
  6) Mango Sticky Rice: base 0.36, harmony +0.07 → 0.43 × 1.18 = 0.51 (set+adj)
  7) Thai Tea Panna Cotta: base 0.33, harmony +0.07 → 0.40 × 1.18 = 0.47
- Total ≈ 3.37

Notes:
- These numbers are illustrative to show how base, harmony, and multipliers combine. The exact weights and thresholds should be tuned in content.

// Section merged into top design; the separate refinements list was inlined above.


## Flavor-trigger model v0 (SAP-inspired, chef-unique)

Goals:
- Make flavorful, readable abilities that feel culinary, not combat.
- Keep effects data-driven (JSON) and judged per-course with clear hooks.

Triggers (battle-phase oriented):
- OnServe — when this dish is presented to judges.
- OnTaste — when judges taste/evaluate this dish.
- OnPair — when placed adjacent to a complementary neighbor (resolved at serve).
- OnSequence — when a planned flavor progression condition holds (e.g., spice rising across slots).
- OnCourseComplete — after the course resolves (safe point for chain buffs).
- OnSynergyChanged — when cuisine/course/technique set thresholds change (pre-battle/shop time).
- OnClash — when adjacent pair hits a known clash rule.
- OnOverwhelm — when a dish exceeds a stat threshold (e.g., richness ≥ 3).

Effect ops (JSON, composable):
- AddStat{ stat, amount } — modify raw `FlavorStats` for this evaluation window.
- AddBonusAdditive{ amount } — adds to per-dish additive bonus before multiplier.
- AddBonusMultiplicative{ amount } — modifies per-dish multiplier (e.g., +0.03).
- StealStat{ stat, amount, from: Opponent|Adjacent } — transfer along axis.
- RedistributeStat{ from, to, amount } — preserve sum, shift profile.
- CleanseNext{ statuses:[...], count } — remove negative statuses from following dishes.
- CopyStatFromAdjacent{ stat, scale } — model pair carryover.
- GrantStatus{ name, stacks, durationCourses } — e.g., PalateCleanser, Heat, SweetTooth, Fatigue.
- QueueTrigger{ when, payload } — schedule a delayed effect (e.g., next course).

Status effects (short, flavorful):
- PalateCleanser — removes one negative status from the next dish; small freshness add.
- Heat — small +spice; may penalize delicate desserts if unpaired.
- SweetTooth — small +sweetness; reduces harmony if stacked too high.
- Fatigue — slight −freshness, −harmony; decays after 1 course.

Pairings and clashes (rules, not per-dish effects):
- Beverage↔Meat (or Rich mains) → +umami, mitigates smoke/bitter clashes.
- Soup↔Bread → satiety buffer; early-course comfort pairing.
- Acid↔Dairy clash → small multiplicative penalty unless buffered by richness.

JSON sketches (consistent with data plan):
```json
{
  "effects": [
    {
      "id": "ramen_spice_bloom",
      "when": "OnServe",
      "ops": [ { "op": "AddStat", "stat": "spice", "amount": 1, "target": "Adjacent" } ]
    },
    {
      "id": "ramen_sweet_rich_link",
      "when": "OnSequence",
      "require": { "nextDish": { "sweetnessAtLeast": 2 } },
      "ops": [ { "op": "AddStat", "stat": "richness", "amount": 2, "target": "Next" } ]
    }
  ]
}
```

```json
{
  "effects": [
    {
      "id": "sushi_fresh_cut",
      "when": "OnTaste",
      "require": { "opponent": { "freshnessBelow": 2 } },
      "ops": [ { "op": "StealStat", "stat": "freshness", "amount": 1, "from": "Opponent" } ]
    },
    {
      "id": "sushi_umami_pair",
      "when": "OnPair",
      "ops": [ { "op": "AddBonusAdditive", "amount": 0.02, "target": "SelfAndAdjacent" } ]
    }
  ]
}
```

```json
{
  "effects": [
    {
      "id": "cheesecake_table_service",
      "when": "OnCourseComplete",
      "ops": [ { "op": "AddStat", "stat": "sweetness", "amount": 1, "target": "AllDesserts" } ]
    },
    {
      "id": "cheesecake_sugar_rush",
      "when": "OnOverwhelm",
      "require": { "self": { "richnessAtLeast": 3 } },
      "ops": [ { "op": "GrantStatus", "name": "SweetTooth", "stacks": 1, "durationCourses": 2, "target": "NextTwo" } ]
    }
  ]
}
```

Mapping to current dishes (examples using existing `DishType`s):
- Ramen — spice bloom OnServe; sweetness→richness link OnSequence.
- Sushi — fresh cut advantage OnTaste; umami pair addend OnPair.
- Cheesecake — dessert table service OnCourseComplete; sugar rush OnOverwhelm.

Notes:
- Keep numbers small (+1 stat, +0.02–0.04 mult) and scale with tiers.
- Prefer adjacency and sequence readability over opaque global auras early on.

