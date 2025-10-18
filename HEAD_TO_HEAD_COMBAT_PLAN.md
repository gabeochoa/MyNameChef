## Head-to-Head Combat Plan (Dish-by-Dish)

### At a glance: what changes
- Remove legacy tug-of-war judging systems and UI.
- Add combat components: `CombatStats`, `PreBattleModifiers`, `CombatQueue`, `CourseOutcome`.
- Update existing components: repurpose `DishBattleState` phases; refit `BattleResult` to wins/outcomes.
- Add trigger plumbing: `TriggerEvent`, `TriggerQueue`, dispatch + effect resolution (phased in).
- Add combat systems: init, compute stats, start course, enter animation, resolve ticks, advance course, pairings/clashes, trigger dispatch, effect resolution.
- Keep determinism via `SeededRng`; add replayable `BattleReport`.

### Goals
- Replace tug-of-war judge model with SAP-style head-to-head combat resolved per slot.
- Preserve current presentation/animation feel: simultaneous serving, alternating "bites" as damage ticks until one dish’s Body reaches 0.
- Keep determinism and replayability; integrate with existing `SeededRng` work.

### High-level Flow
1) Shop/lock-in unchanged for now.
2) On entering Battle, create two combat queues (player/opponent) ordered by slot index 1..7.
3) Resolve slot i vs slot i sequentially (or allow quick-skip for UI):
   - Initialize combat stats for both dishes: `Zing` (attack), `Body` (health), with level scaling and pre-battle global modifiers.
   - Apply global pairings/clashes computed pre-battle into temporary per-team modifiers for this battle only.
   - Run tick loop: alternate attacker per bite; each tick reduces defender Body by attacker Zing; fire hooks; end when Body<=0.
   - Record outcome for the course and proceed to next slot.
4) After all slots, determine match winner by count of per-slot wins; tie rules configurable.
5) Persist a compact `BattleReport` for replay.

### Deterministic Client Re-simulation (server-authoritative outcome)
- Authority: The server decides the official outcome and returns only the minimal inputs to reproduce it (no event stream).
- Client inputs:
  - `seed` (shared deterministic RNG seed), `totalCourses`, and the opponent snapshot (and any needed meta like judge/profile if applicable later).
  - Optional: `rulesHash`/`contentVersion` to ensure client rules/assets match server.
- Client behavior: Runs the exact same resolve locally using the shared seed and snapshots, producing identical bites/outcomes. UI is a replay of the local sim.
- Controls: Pause/Play, Speed (0.5x/1x/2x/4x), Seek to course (by re-simulating up to the target tick deterministically).

Pseudocode (fixed-step sim replay):
```cpp
// Deterministic tick (avoid float drift in logic)
constexpr int kTickMs = 150; // bite cadence

struct ReplayState : BaseComponent {
  bool active = false, paused = false; float timeScale = 1.0f;
  int64_t clockMs = 0; int64_t targetMs = 0; // for seeks
  uint64_t seed = 0; int totalCourses = 7; std::optional<OpponentSnapshot> opp;
  // Internal sim state mirrors combat ECS (entities created from snapshots)
};

// Drives simulation steps; rendering systems read ECS to animate
struct SimReplayControllerSystem : System<ReplayState> {
  void for_each_with(Entity&, ReplayState& rs, float dt) override {
    if (!rs.active || rs.paused) return;
    rs.targetMs += (int64_t)(dt * 1000 * rs.timeScale);
    while (rs.clockMs + kTickMs <= rs.targetMs) {
      step_sim(kTickMs); // run one deterministic logic step; consumes SeededRng
      rs.clockMs += kTickMs;
    }
  }
  void seek(ReplayState& rs, int64_t t_ms) { reset_to_snapshots(rs); rs.clockMs = 0; rs.targetMs = t_ms; }
};

struct ReplayUISystem : System<ReplayState> { /* play/pause, speed, scrub bar */ };
```

### Remove Legacy Judging (do this first)
- Disable legacy tug-of-war judging systems and UI to prevent conflicts before adding H2H:
  - Stop registering systems from `src/systems/JudgingSystems.h` (`InitJudgingState`, `AdvanceJudging`).
  - Remove/guard includes of `JudgingSystems.h` anywhere in the battle setup (e.g., `main.cpp`, battle pipeline registrars).
  - Mark `src/components/judging_state.h` as legacy and stop querying it; keep the header temporarily to ease rollback.
  - Remove judge total updates and tug-of-war bar rendering from Results/Battle UI (comment or compile-guard behind `LEGACY_JUDGING` flag).
  - Update `BattleResult` uses to not expect judge totals; callers should read per-course outcomes/wins once H2H is wired.
  - Verify build compiles clean after removal before proceeding.

### Component Changes
- New: `CombatStats { int zing; int body; }`
  - Derived per dish from existing `FlavorStats` using TODO.md mapping (Zing from spice+acidity+umami≥ thresholds 0..3; Body from satiety+richness+sweetness+freshness≥ thresholds 0..4), with level scaling: if `DishLevel.level>1`, multiply both by 2 post-base.
- New: `PreBattleModifiers { int zingDelta; int bodyDelta; }`
  - Aggregated per dish prior to combat from pairings/clashes/global rules; defaults 0.
- New: `CourseOutcome { int slotIndex; enum Winner { Player, Opponent, Tie }; int ticks; }`
  - Stored in `BattleReport` as an array for UI playback.
- Update: `DishBattleState`
  - Replace `Phase::Presenting/Judged` with combat states: `enum Phase { InQueue, Entering, InCombat, Finished }`.
  - Keep `team_side`, `queue_index`, add `float enter_progress` and optional `float bite_timer` for animation pacing.
- New: `CombatQueue { int current_index; int total_courses; bool complete; }` singleton
  - Drives course-by-course advancement similar to `JudgingState` but for combat.
- Deprecate: `JudgingState` and judge-weighted totals in `BattleResult` (retain file for now, but mark unused during combat model rollout).
- Update: `BattleResult`
  - Replace judge totals with: `int playerWins; int opponentWins; int ties; std::vector<CourseOutcome> outcomes;`.

### System Changes
- New: `InitCombatState`
  - On transition to `Battle`, reset `CombatQueue`, zero wins/ties in `BattleResult`, collect slot-ordered queues for both teams, set all dishes to `InQueue`.
- New: `ComputeCombatStatsSystem`
  - For all dishes in queue, compute base `CombatStats` from `FlavorStats` and `DishLevel`; store component.
- New: `ApplyPairingsAndClashesSystem`
  - Pre-battle pass; evaluates simple global rules (from TODO.md: tiny +Body for pairings, −Zing for clashes) and writes `PreBattleModifiers` per dish.
- New: `StartCourseSystem`
  - When no dish is `Entering`/`InCombat`, pick next slot i for both sides; set both to `Entering`, reset `enter_progress`.
- New: `EnterAnimationSystem`
  - Advance `enter_progress`; when both reach 1.0, transition to `InCombat`, initialize `bite_timer`, decide which side bites first (alternate or player-first configurable).
- New: `ResolveCombatTickSystem`
  - While `InCombat`, alternate bites based on a fixed tick rate; subtract Zing from opponent Body; if Body<=0, set both to `Finished`, record `CourseOutcome`, increment wins, and mark course complete.
  - Fire lightweight hooks: `OnBiteTaken` and `OnDishFinished` (stubs for now).
- New: `AdvanceCourseSystem`
  - After `Finished`, increment `CombatQueue.current_index`; if past `total_courses`, mark complete and trigger transition to Results.
- Update: `TransitionToResultsSystem` (or reuse existing) to look at `CombatQueue.complete`.
- Remove/Replace: `AdvanceJudging` presentation increments; keep any UI bits that are reusable for entering animation.

### Triggers and Effects (supported by design, phased implementation)
- Trigger hooks to support: `OnServe`, `OnBiteTaken`, `OnDishFinished`, `OnCourseStart`, `OnCourseComplete`.
- Event pipeline:
  - New `TriggerEvent { hook, sourceEntityId, slotIndex, teamSide, payload }` and `TriggerQueue` singleton.
  - Deterministic processing order: same-tick events are ordered by `(slotIndex asc, Player then Opponent, sourceEntityId asc)`; any random tie-break uses `SeededRng`.
- State surfaces for effects:
  - `CombatStats { baseZing, baseBody, currentZing, currentBody }` where `current*` are mutable in combat.
  - `DeferredFlavorMods { satiety, sweetness, spice, acidity, umami, richness, freshness }` applied to dishes that have not yet served (affects their later CombatStats computation).
  - `PendingCombatMods { deltaZing, deltaBody }` applied immediately to a dish currently in combat.
  - Team/global auras: optional `TeamFlavorAura { ... }` to model team-wide future dish adjustments.
- Effect ops (initial set):
  - `AddFlavorStat { stat, amount, targetScope }` → writes into `DeferredFlavorMods` for targeted future dishes.
  - `AddCombatZing { amount, targetScope }`, `AddCombatBody { amount, targetScope }` → immediate adjustments on currently fighting target(s).
  - `AddTeamFlavorStat { stat, amount, teamSide }` → aura for not-yet-served dishes on a team.
  - `GrantStatus { name, stacks, durationCourses, targetScope }` → placeholder for short status effects.
- Targeting scopes:
  - `Self`, `Opponent`, `AllAllies`, `AllOpponents`, `Next`, `DishesAfterSelf`, `FutureAllies`, `FutureOpponents`, `Global` with optional filters (by course/tag).
- Timing and mapping rules:
  - At `StartCourseSystem`, compute each dish’s `CombatStats` just-in-time from `FlavorStats + DeferredFlavorMods + PreBattleModifiers`, then clear consumed deferred mods for that dish.
  - In combat, `OnBiteTaken` may apply `AddCombatBody/Zing` immediately; if it targets future dishes (e.g., "increase sweetness by 1 for all dishes after this"), emit `AddFlavorStat` to their `DeferredFlavorMods`.
  - `OnServe` rules that reduce/anoint global stats (e.g., reduce spiciness for all dishes) should enqueue `AddTeamFlavorStat` to both teams; already-fighting dishes are unaffected unless explicitly targeted via combat ops.
- Persistence (optional at first): capture a minimal event log `{ time, hook, source, targets, op }` to enrich replay later.

### Determinism and Replay
- Server-side: Source all randomness (first-bite, procs) from `SeededRng` and record `seed` in the report.
- Client-side: Use only `events[]` to animate; do not recompute outcomes. Optional debug re-sim may assert against the report.
- `BattleReport` includes `seed`, `outcomes[]`, and `events[]` sufficient for faithful visual playback.
- Dev serialization: `output/battles/results/*.json`; production fetch over network into `ReplayState`.

### UI/Animation
- Keep current simultaneous serve animation; map `enter_progress` to slide-in.
- Overlay Z/B numbers on sprites per TODO: green rhombus (Zing) top-left, pale yellow square (Body) top-right.
- During combat, animate alternating bite effects and decrementing Body; on finish, brief win tick.
- Results screen: show 7 mini-panels with per-slot winner icons.

### Implementation Phases (hand-off checklists + pseudocode)

Phase 0 — Remove legacy judging (conflict-free baseline)
- Deliverables:
  - Legacy judge systems unregistered, UI and totals guarded/removed, build passes.
- Files:
  - Edit `src/main.cpp` (or registrar) to stop registering `JudgingSystems.h` systems.
  - Guard/remove uses of `JudgingState` and judge totals in battle/results UI.
- Acceptance:
  - Game enters Battle and transitions to Results without judge totals; compiles clean.

Phase 1 — Components and scaffolding
- Deliverables:
  - New components: `CombatStats`, `PreBattleModifiers`, `CourseOutcome`, `CombatQueue`.
  - Updated `DishBattleState` phases; updated `BattleResult` fields.
- Files:
  - Add `src/components/combat_stats.h`, `pre_battle_modifiers.h`, `course_outcome.h`, `combat_queue.h`.
  - Update `src/components/dish_battle_state.h`, `src/components/battle_result.h`.
- Pseudocode (components):
```cpp
// combat_stats.h
struct CombatStats : BaseComponent {
  int baseZing = 0, baseBody = 0;
  int currentZing = 0, currentBody = 0;
};

// pre_battle_modifiers.h
struct PreBattleModifiers : BaseComponent {
  int zingDelta = 0;
  int bodyDelta = 0;
};

// combat_queue.h (singleton)
struct CombatQueue : BaseComponent {
  int current_index = 0; // 1..7
  int total_courses = 7;
  bool complete = false;
};

// course_outcome.h
struct CourseOutcome : BaseComponent {
  int slotIndex = 0;
  enum struct Winner { Player, Opponent, Tie } winner = Winner::Tie;
  int ticks = 0; // number of bites
};

// dish_battle_state.h (update Phase)
enum struct Phase { InQueue, Entering, InCombat, Finished };
```

Phase 2 — Basic course-by-course combat loop (no pairings/clashes)
- Deliverables:
  - Systems: `InitCombatState`, `ComputeCombatStatsSystem`, `StartCourseSystem`, `EnterAnimationSystem`, `ResolveCombatTickSystem`, `AdvanceCourseSystem`.
  - Course i vs i resolves with alternating bites; wins tracked in `BattleResult`.
- Files:
  - Add systems under `src/systems/` with UpperCamelCase filenames.
- Pseudocode (systems skeletons):
```cpp
// InitCombatState.h
struct InitCombatState : System<CombatQueue> {
  void for_each_with(Entity&, CombatQueue& cq, float) override {
    cq.current_index = 1; cq.total_courses = 7; cq.complete = false;
    // set all DishBattleState::phase = InQueue
  }
};

// ComputeCombatStatsSystem.h
struct ComputeCombatStatsSystem : System<IsDish, DishLevel, CombatStats, PreBattleModifiers> {
  void for_each_with(Entity& e, IsDish& dish, DishLevel& lvl, CombatStats& cs, PreBattleModifiers& pre, float) override {
    auto f = get_dish_info(dish.type).flavor;
    int zing = calc_zing(f); int body = calc_body(f);
    if (lvl.level > 1) { zing *= 2; body *= 2; }
    cs.baseZing = cs.currentZing = std::max(0, zing + pre.zingDelta);
    cs.baseBody = cs.currentBody = std::max(0, body + pre.bodyDelta);
  }
};

// StartCourseSystem.h
struct StartCourseSystem : System<CombatQueue> {
  void for_each_with(Entity&, CombatQueue& cq, float) override {
    if (cq.complete) return;
    // pick player and opponent with queue_index == cq.current_index
    // set both states to Entering and reset enter_progress
  }
};

// EnterAnimationSystem.h
struct EnterAnimationSystem : System<DishBattleState> {
  void for_each_with(Entity& e, DishBattleState& s, float dt) override {
    if (s.phase != Phase::Entering) return;
    s.enter_progress = min(1.0f, s.enter_progress + dt / 0.45f);
    if (s.enter_progress >= 1.0f) { s.phase = Phase::InCombat; s.bite_timer = 0.0f; }
  }
};

// ResolveCombatTickSystem.h
struct ResolveCombatTickSystem : System<DishBattleState, CombatStats> {
  void for_each_with(Entity& e, DishBattleState& s, CombatStats& cs, float dt) override {
    if (s.phase != Phase::InCombat) return;
    s.bite_timer += dt; if (s.bite_timer < kTick) return; s.bite_timer = 0.0f;
    Entity opponent = find_opponent(e);
    auto& oc = opponent.get<CombatStats>();
    bool playerTurn = decide_turn(e); // alternate deterministically
    if (playerTurn) { oc.currentBody -= cs.currentZing; emit_on_bite_taken(opponent); }
    else { cs.currentBody -= oc.currentZing; emit_on_bite_taken(e); }
    if (cs.currentBody <= 0 || oc.currentBody <= 0) { finish_course(e, opponent); }
  }
};

// AdvanceCourseSystem.h
struct AdvanceCourseSystem : System<CombatQueue> {
  void for_each_with(Entity&, CombatQueue& cq, float) override {
    if (both_finished(cq.current_index)) { cq.current_index++; if (cq.current_index > cq.total_courses) cq.complete = true; }
  }
};
```

Phase 2b — Trigger plumbing (no gameplay effects yet)
- Deliverables:
  - Components: `TriggerEvent`, `TriggerQueue`.
  - System: `TriggerDispatchSystem` emitting `OnServe`, `OnBiteTaken`, `OnDishFinished`.
- Pseudocode:
```cpp
struct TriggerEvent : BaseComponent { Hook hook; int sourceId; int slot; TeamSide side; /*payload*/ };
struct TriggerQueue : BaseComponent { std::vector<TriggerEvent> q; };
struct TriggerDispatchSystem : System<TriggerQueue> { /* drain q in deterministic order */ };
```

**CURRENT STATUS: Phase 2b - Trigger System Implementation**

### Step-by-Step Implementation Plan

#### Step 1: Add OnStartBattle Hook ✅ COMPLETED
- [x] Add `OnStartBattle` to `TriggerHook` enum in `src/components/trigger_event.h`
- [x] Verify existing `TriggerEvent` and `TriggerQueue` components are ready

#### Step 2: Create TriggerDispatchSystem
- [ ] Create `src/systems/TriggerDispatchSystem.h`
- [ ] Implement deterministic event processing order: `(slotIndex asc, Player then Opponent, sourceEntityId asc)`
- [ ] Add handler methods for each trigger hook:
  - `handleOnStartBattle()` - Log battle initialization
  - `handleOnServe()` - Log dish entering combat
  - `handleOnBiteTaken()` - Log damage dealt
  - `handleOnDishFinished()` - Log dish defeat
  - `handleOnCourseStart()` - Log course beginning
  - `handleOnCourseComplete()` - Log course completion

#### Step 3: Integrate TriggerQueue with BattleProcessor
- [ ] Add `TriggerQueue` singleton creation to `make_battle_processor_manager()`
- [ ] Update `BattleProcessor::startBattle()` to fire `OnStartBattle` trigger
- [ ] Update `BattleProcessor::processCourse()` to fire `OnCourseStart` trigger
- [ ] Update `BattleProcessor::resolveCombatTick()` to fire `OnBiteTaken` trigger
- [ ] Update `BattleProcessor::finishCourse()` to fire `OnDishFinished` and `OnCourseComplete` triggers

#### Step 4: Register TriggerDispatchSystem
- [ ] Add `#include "systems/TriggerDispatchSystem.h"` to `src/main.cpp`
- [ ] Register `TriggerDispatchSystem` in the update systems list
- [ ] Ensure it runs after `BattleProcessorSystem` but before legacy systems

#### Step 5: Test Trigger System
- [ ] Verify `OnStartBattle` fires when battle begins
- [ ] Verify `OnCourseStart` fires for each course
- [ ] Verify `OnBiteTaken` fires for each bite
- [ ] Verify `OnDishFinished` fires when dishes are defeated
- [ ] Verify `OnCourseComplete` fires when courses finish
- [ ] Check deterministic ordering of events

#### Step 6: Add Effect System Foundation
- [ ] Create `src/components/deferred_flavor_mods.h`
- [ ] Create `src/components/pending_combat_mods.h`
- [ ] Create `src/systems/EffectResolutionSystem.h` (stub for now)
- [ ] Register `EffectResolutionSystem` to run after `TriggerDispatchSystem`

### Integration Points with BattleProcessor

The trigger system will integrate with `BattleProcessor` at these key points:

1. **Battle Start**: `BattleProcessor::startBattle()` → Fire `OnStartBattle`
2. **Course Start**: `BattleProcessor::processCourse()` → Fire `OnCourseStart` 
3. **Combat Tick**: `BattleProcessor::resolveCombatTick()` → Fire `OnBiteTaken`
4. **Course End**: `BattleProcessor::finishCourse()` → Fire `OnDishFinished` + `OnCourseComplete`

### Next Phase Preview: Effect System (Phase 3b)
Once triggers are working, we'll add:
- `AddFlavorStat` effects for future dishes
- `AddCombatBody/Zing` effects for current combat
- Sample effects like "increase sweetness for dishes after this one"

Phase 2c — Client replay foundation
- Deliverables:
  - `ReplayState`, `ReplayControllerSystem`, `ReplayUISystem` with playback of a local/dev `BattleReport`.
- Acceptance:
  - Can load a JSON report and play with Pause/Play and Speed; basic seek to course.

Phase 3 — Minimal pairings/clashes (pre-battle global)
- Deliverables:
  - `ApplyPairingsAndClashesSystem` applies +Body/-Zing team-wide based on simple rules before combat.
- Pseudocode:
```cpp
struct ApplyPairingsAndClashesSystem : System<IsDish, PreBattleModifiers> {
  void run(float) override { /* scan lineups, compute team deltas, write into PreBattleModifiers */ }
};
```

Phase 3b — First effects via triggers
- Deliverables:
  - `EffectResolutionSystem` with ops: `AddFlavorStat`, `AddCombatBody`.
  - Two sample effects:
    - `OnBiteTaken`: `AddFlavorStat(sweetness,+1)` to `DishesAfterSelf` (same team).
    - `OnServe`: `AddTeamFlavorStat(spice,-1)` for both teams.
- Pseudocode:
```cpp
struct EffectResolutionSystem : System<TriggerQueue> {
  void apply(const TriggerEvent& ev) { /* map to ops and mutate DeferredFlavorMods or CombatStats */ }
};
```

Phase 4 — UI overlays and feedback
- Deliverables:
  - Z/B overlays, bite tick feedback, per-course win markers; replay controls (pause, speed, scrub to course).
- Acceptance:
  - Numbers match `CombatStats.current*`; animations align with tick timing.

Phase 5 — Persistence and replay
- Deliverables:
  - Server generates `BattleReport` (seed, outcomes, events); client loads into `ReplayState`.
  - JSON write/read at `output/battles/results/*.json` (dev); Results "Replay" button and in-battle replay entry point.

Phase 6 — Remove legacy files/paths
- Deliverables:
  - Delete/disable `JudgingSystems.h` and any judge-only UI; remove judge totals from `BattleResult` permanently.

### File Touch List (planned)
- Add: `src/components/combat_stats.h`, `src/components/pre_battle_modifiers.h`, `src/components/course_outcome.h`.
- Add: `src/components/combat_queue.h` (singleton).
- Update: `src/components/dish_battle_state.h` (phases and fields).
- Update: `src/components/battle_result.h` (wins and outcomes).
- Add: `src/components/deferred_flavor_mods.h`, `src/components/pending_combat_mods.h`, `src/components/trigger_event.h`, `src/components/trigger_queue.h`.
- Add Systems: `src/systems/InitCombatState.h`, `ComputeCombatStatsSystem.h`, `ApplyPairingsAndClashesSystem.h`, `StartCourseSystem.h`, `EnterAnimationSystem.h`, `ResolveCombatTickSystem.h`, `AdvanceCourseSystem.h`, `TriggerDispatchSystem.h`, `EffectResolutionSystem.h`.
- Retire: `src/systems/JudgingSystems.h` usage in battle loop.
- Remove/Guard: Registration and UI code that updates judge totals/tug-of-war bar; references to `JudgingState` and judge fields in `BattleResult`.
 - Add Replay Components: `src/components/replay_state.h`, `src/components/replay_event.h`.
 - Add Replay Systems: `src/systems/ReplayControllerSystem.h`, `src/systems/ReplayUISystem.h`.

Example `BattleReport` (JSON):
```json
{
  "seed": 123456789,
  "totalCourses": 7,
  "outcomes": [ { "slotIndex": 1, "winner": "Player", "ticks": 3 } ],
  "events": [
    { "t_ms": 0,   "course": 1, "type": "Enter", "side": "Player",   "entityId": 101 },
    { "t_ms": 0,   "course": 1, "type": "Enter", "side": "Opponent", "entityId": 201 },
    { "t_ms": 450, "course": 1, "type": "Serve" },
    { "t_ms": 700, "course": 1, "type": "Bite", "side": "Player",   "payload": { "damage": 2 } },
    { "t_ms": 950, "course": 1, "type": "Bite", "side": "Opponent", "payload": { "damage": 1 } },
    { "t_ms": 1100, "course": 1, "type": "DishFinished", "side": "Opponent" },
    { "t_ms": 1200, "course": 1, "type": "CourseComplete", "winner": "Player" }
  ]
}
```

### Risks/Notes
- Keep numbers tiny for readability (Zing 0–3, Body 0–4, level doubling).
- Ensure tick rate and animation pacing feel snappy; allow skip/fast-forward.
- Determinism: bite alternation and any random tiebreaks must read from `SeededRng`.


