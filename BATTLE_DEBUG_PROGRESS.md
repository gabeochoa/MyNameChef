## Battle Debug Progress and Next Steps

### What we changed/tried this session
- Per-dish turn tracking
  - Added `first_bite_decided` and `next_turn_is_player` to `DishBattleState` and reset them on enter/advance.
  - Removed static turn variable in `ResolveCombatTickSystem`.

- Trigger plumbing hardening
  - Ensure the combat manager has a `TriggerQueue` before enqueuing events (create if missing) in `ResolveCombatTickSystem` and `StartCourseSystem`.
  - Fixed `PendingCombatMods` logging field names in `TriggerEffectDebugSystem`.

- Combat flow and slot progression
  - `StartCourseSystem`: if a slot is missing one side, advance to next slot instead of ending the battle.
  - `AdvanceCourseSystem`: also advance when a slot has no dishes remaining.
  - `ResolveCombatTickSystem` defeat handling:
    - Double KO ends course immediately.
    - If only one side is defeated, mark that dish finished and advance the surviving dish to the next opponent in the same slot.
  - `find_next_available_opponent` now restricts to the same `queue_index` (slot).

- Initialization/entry
  - `BattleEnterAnimationSystem`: on entry completion, reset bite timer and per-pairing turn flags.
  - `InitCombatState`: resets all dishes to `InQueue`, zero bite timer, and resets the new turn flags.
  - `BattleTeamLoaderSystem`: initializes battle dishes with `CombatStats` and `PreBattleModifiers`; sets turn flags.

- Determinism/turn order
  - First attacker decided by stats (higher baseZing, then baseBody, then fallback) at first bite of pairing, then alternate per pairing.

- Build/run status
  - Fixed compile errors (`damage_dealt` scope, incorrect field names).
  - Main game runs. Debug logs show `TriggerQueue` exists but has 0 events repeatedly; also shows a steady list of `PreBattleModifiers`. This suggests bites/triggers aren’t firing.

### Observed issues (current)
- First slot with no valid pairing previously caused an immediate transition to results; this was corrected to skip to the next slot.
- Even after corrections, logs still show 0 trigger events and no visible bite processing.
- It appears dishes may not be transitioning to `InCombat`, or `ResolveCombatTickSystem` is not meeting its preconditions to fire bites.

### Instrumentation ideas (next)
- Add one-time logs to confirm phase transitions per dish:
  - When `BattleEnterAnimationSystem` sets `Phase::InCombat`, log entity id/slot/team.
  - In `ResolveCombatTickSystem` at the beginning of `for_each_with`, log when an entity in `InCombat` passes the `kTickMs` gate.
  - Log each bite with attacker/defender, damage, and resulting `currentBody`.

- Validate preconditions for bite resolution:
  - Confirm both paired dishes are actually in `Phase::InCombat` for the same `queue_index`.
  - Confirm `ComputeCombatStatsSystem` runs before `ResolveCombatTickSystem` and sets `currentZing/currentBody` > 0.
  - Verify `dbs.bite_timer` is accumulating (dt flow) and `kTickMs` threshold is being crossed.

- Pairing/advancement logic checks:
  - Confirm `StartCourseSystem` starts both sides entering for the current `cq.current_index` when present.
  - After a dish finishes, verify the surviving dish remains in `InCombat` and the opponent swap to next in-queue dish for the same slot.
  - Ensure `AdvanceCourseSystem` only increments `current_index` once both sides of the slot are `Finished` or the slot is empty.

- Trigger pipeline validation:
  - Temporarily add a direct enqueue of a synthetic event after `InCombat` begins to confirm `TriggerDispatchSystem`/`EffectResolutionSystem` drain it.
  - If events drain correctly, the problem is before enqueue (no bites). If not, investigate dispatch ordering and `should_run` guards for those systems.

- Data consistency checks:
  - Reconfirm all battle dishes receive `CombatStats` and values are non-zero (especially `currentBody`).
  - Confirm the JSON loader generates the expected initial teams and slots (no mismatch between `total_courses` and available dishes).

### Structural cleanups (later)
- Throttle or gate `TriggerEffectDebugSystem` logging to reduce spam while debugging combat.
- Add a minimal headless test harness for `ResolveCombatTickSystem` with fake dt to validate bite alternation and advancement.

### Quick checklist to run next
1) Add/enable logs for: enter→`InCombat`, tick gate pass, and each bite.
2) Verify `ComputeCombatStatsSystem` ordering before `ResolveCombatTickSystem`.
3) Confirm both dishes in the active slot reach `InCombat`.
4) If still no bites, temporarily lower `kTickMs` or log dt to ensure the timer threshold is crossed.
5) Validate `AdvanceCourseSystem` increments the slot only after both dishes are finished or slot is empty.


