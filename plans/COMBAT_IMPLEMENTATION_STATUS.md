# Combat Implementation Status

Combat is implemented and tested. This file is the current contract (issue 100). Older phase plans in this directory describe superseded steps.

## Contract

- Teams fight course by course. The dish at queue index 0 on each side is active.
- Active dishes enter (0.45s), then attack simultaneously. Each bite deals the attacker's current Zing as damage to the opponent's current Body, with a pre pause and a post pause around the bite (BattleTiming, 0.35s each). Tick duration is 0.15s.
- Damage is simultaneous. Both dishes can finish in the same tick, which records a tie for that course.
- A finished dish fires OnDishFinished. Its survivor opponent carries over to the next dish at its current Body. Stat modifiers in combat apply deltas to current Body, they do not heal accumulated damage.
- OnCourseComplete fires once per course with each finished dish as source. Pending OnDishFinished effects resolve before the battle ends.
- Triggers order by slot, team Zing total, source Zing, source id, hook, with stable sort preserving emission order for equal keys.
- Results come from the authoritative combat result (`BattleResult.outcomes`). Missing results are a failure, flavor total calculation is not a fallback.
- Level scaling is `2^(level-1)`, capped at level 10. Deferred flavor buffs convert to persistent modifiers at the same scaling.
- Replay pause stops every battle update path, including BattleProcessor.

## Systems

`InitCombatState`, `ComputeCombatStatsSystem`, `StartCourseSystem`, `BattleEnterAnimationSystem`, `SimplifiedOnServeSystem`, `ResolveCombatTickSystem`, `AdvanceCourseSystem`, `TriggerDispatchSystem`, `EffectResolutionSystem`, registered in `src/systems/battle_system_registry.cpp`.

## Known divergence

Two simulators exist, ECS combat (client, authoritative for the visible battle) and BattleProcessor (server and replay data). Unifying them is deferred, issue 62 in CODE_REVIEW_100_ISSUES.md.
