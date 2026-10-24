# Afterhours gaps identified from the 101-issue review

Reviewed September 23, 2026. Every item in [the original review](CODE_REVIEW_100_ISSUES.md) was considered against the checked-out Afterhours source at `12a45712c8c15395532021434a5ed90a05f6f715`. The proposals below identify reusable library changes that would make those fixes easier. The 14 proposals cover 47 original issues; the remaining 54 have local fixes with no justified library addition. The final table records a decision for all 101 issues.

Issue numbers refer to the original findings, not a claim that every finding remains open. The current game already contains mitigations for several items, including temporary-file writes (#36–37), runner checks (#82–83), honoring headless mode (#88), and test identities (#91). Those changes can still benefit from shared library support. This document is based on source inspection; the proposed APIs and acceptance checks have not been implemented or run.

Afterhours already provides independent `EntityCollection`s, thread-local collection selection, pointer-free snapshot projections, fixed updates, timers, synthetic input, an E2E runner, and offscreen rendering. Each proposal describes the additional contract or integration support needed. Game schemas, combat rules, authentication, HTTP behavior, and test process management remain application responsibilities.

## AH-01 — Make collection ownership explicit and safe to scope

**Helps original issues:** #1, #4, #44, #47, #66, #101. **Suggested priority:** high.

**Existing support and gap:** [EntityHelper](vendor/afterhours/src/core/entity_helper.h#L40) already has thread-local `set_default_collection`, and [EntityQuery](vendor/afterhours/src/core/entity_query.h#L483) accepts an explicit collection. However, changing the default is a raw pointer assignment. [SystemManager](vendor/afterhours/src/core/system.h#L469) accepts an entity vector while its merge/cleanup operations use the ambient collection. Callers must keep those two choices aligned and restore the previous pointer on every exit. Game [server contexts](src/server/server_context.cpp#L17) still use ambient ECS state and application singletons.

**Proposed library change:** add an RAII collection binding that restores the previous binding, including nested scopes and exceptions; provide collection-bound system execution that consistently performs queries, merges, cleanup, and singleton access against that collection. Document one owning thread per collection and provide a two-world server/test example. Scope any plugin mutable state used by that example to its world, or explicitly reject concurrent use.

**Game adoption:** create a battle world per request or serialize work onto a dedicated owner. Move `SeededRng`, screen/session state, and other application globals into the owned context. A thread-local ECS pointer alone does not isolate those globals or protect a shared save store.

**Acceptance:** two worlds can contain the same singleton types without cross-talk; nested bindings restore correctly after an exception; destroying one world leaves the other usable; system execution cannot accidentally merge another world's pending entities. Exercise the documented concurrent pattern with a race detector.

## AH-02 — Provide a session reset contract for systems and caches

**Helps original issues:** #4, #46, #47, #53, #66, #69. **Suggested priority:** high.

**Existing support and gap:** [SystemBase](vendor/afterhours/src/core/system.h#L91) exposes per-iteration hooks, but no session start/end/reset hook. [Collection deletion](vendor/afterhours/src/core/entity_collection.h#L399) clears entities and singleton registrations; it cannot reset retained system members or application static maps. The game has loader latches, replay latches, and entity-ID caches that outlive their intended operation.

**Proposed library change:** add an opt-in system-group lifecycle with a generation token and documented reset/dispose callbacks. Supply examples showing how request state is recreated for each generation, entity-local bookkeeping lives in components, and external caches are invalidated on entity/session teardown. Include a screen-entry or explicit invalidation example for caches such as history.

**Game adoption:** reset the Continue loader and battle loaders on each new request/replay generation; make New Team recreate session state; remove static combat bookkeeping or explicitly expire it. History still needs an application signal when a report is created or deleted.

**Acceptance:** repeat a successful load, failed load followed by retry, New Team, and replay restart without restarting the process. No prior result/latch/cache survives its declared lifetime, and bookkeeping remains bounded after repeated battles.

## AH-03 — Offer a simulation driver with one clock and explicit startup/completion

**Helps original issues:** #2, #62, #65, #67, #71, #75, #101. **Suggested priority:** high.

**Existing support and gap:** [SystemManager](vendor/afterhours/src/core/system.h#L437) already has fixed updates and an accumulator; [timer](vendor/afterhours/src/plugins/timer.h#L13) supports individual timer pause/reset. There is no common simulation-session contract for initialization, fixed stepping, pause, speed, completion, or resetting accumulated time. `SystemManager::run` also includes rendering. The game's [client loop](src/main.cpp#L258), server loop, timing helpers, and parallel battle processor make their own choices.

**Proposed library change:** add an update-only simulation driver around the existing system manager, with explicit initialize/ready/running/finished/failed states, configurable fixed step, one speed multiplier, a pauseable system group, elapsed tick count, and resettable accumulator. Accept application completion callbacks only after initialization and required deferred work. Provide a bounded `run_until` helper for tests/server jobs, using a separate wall-clock timeout so paused simulation time cannot disable the timeout.

**Game adoption:** run one authoritative combat system group on both client and server. Supply battle readiness/completion rules and publish the final tick count for replay progress; remove the conflicting processor. The library should not decide whether an empty team wins or when rewards are earned.

**Acceptance:** equivalent inputs produce equivalent state after N ticks in client and server harnesses; 2× speed advances twice as much simulation time; pause freezes every combat update; restart resets elapsed time; an uninitialized empty world is not reported as a completed battle; bounded runs time out cleanly.

## AH-04 — Add ordered event phases and deferred mutation helpers

**Helps original issues:** #54, #55, #57, #59. **Suggested priority:** high.

**Existing support and gap:** [SystemManager::tick](vendor/afterhours/src/core/system.h#L469) runs systems in registration order and merges newly created entities between systems. That does not drain gameplay event queues or make mutation of a component's vector safe. The [battle registry](src/systems/battle_system_registry.cpp#L55) resolves effects before the combat tick can generate new death events, while effect copying can append into the list currently being traversed.

**Proposed library change:** provide an optional event/command queue with monotonic emission sequence, an application-defined ordering key, separate current/next batches, and explicit flush points between named system phases. Support draining cascaded events before completion with a work limit that reports an error if exceeded. Document deferred additions to component-owned collections; an ECS merge is not a substitute for that deferral.

**Game adoption:** define one consistent trigger ordering tuple, preserve same-source hook order, enqueue copied effects for a safe boundary, and resolve pending death effects before testing battle completion. Combat targeting and effect-copy eligibility remain game rules.

**Acceptance:** same-key events retain emission order; events emitted while processing a batch are processed at the documented boundary; copying effects cannot invalidate an active iterator; a last-dish death summon resolves before terminal completion; a cyclic cascade ends with a diagnostic rather than an infinite loop.

## AH-05 — Support canonical snapshots with application-defined stable identities

**Helps original issues:** #30, #89. **Suggested priority:** high.

**Existing support and gap:** [snapshot.h](vendor/afterhours/src/core/snapshot.h#L19) exports records keyed by runtime `EntityHandle`; [EntityHandle](vendor/afterhours/src/core/entity_handle.h#L8) is a slot/generation reference within a runtime collection. Neither is a cross-process battle identity. The game [fingerprint](src/utils/battle_fingerprint.h#L71) sorts and hashes process-local entity IDs.

**Proposed library change:** extend snapshot projection with a caller-supplied stable identity and ordering function, plus deterministic encoding/hash and field-level diff helpers for registered value types. Specify numeric encoding, field order, and schema version; exclude allocator IDs, pointers, handles, and presentation-only state unless explicitly selected. Detect duplicate stable identities.

**Game adoption:** define battle identities that also distinguish summons, and choose all state needed to establish simulation agreement. Use the same canonical projection in the client/server checksum test. A team/slot pair alone may not distinguish every entity during combat.

**Acceptance:** identical battles hash equally after different unrelated entity allocations and creation orders; changing selected gameplay state changes the digest; duplicate identities fail explicitly; a mismatch reports the first differing identity/field. Re-enable the game's agreement test after its HTTP integration is repaired.

## AH-06 — Extend snapshots with reusable, versioned state codecs

**Helps original issues:** #3, #7, #8, #10, #31, #35, #39, #40, #41, #42, #49, #70. **Suggested priority:** medium; optional persistence module.

**Existing support and gap:** [snapshot::take_components](vendor/afterhours/src/core/snapshot.h#L59) provides caller-defined pointer-free DTO projections. [settings](vendor/afterhours/src/plugins/settings.h#L70) delegates conversion to user types. These are useful foundations, but neither provides a complete round-trip state format, migration contract, canonical checksum envelope, or shared serializer/deserializer registry. The game separately implements battle export, HTTP snapshots, saves, and reports, which lose fields or disagree about their envelopes.

**Proposed library change:** provide opt-in named/versioned codecs and a shared envelope that separates payload from integrity metadata. Support caller-defined validation and migrations, typed decode errors, checksum verification before use, and encode/decode round-trip helpers. Keep JSON support optional. Allow explicit snapshot profiles so battle, save, and report payloads share component converters without serializing every runtime component.

**Game adoption:** define the team/save/report DTOs once and register level, drink pairing, merge progress, and shop stock explicitly. Reuse the opponent DTO in the HTTP response. Implement legal dish/slot/economy validation in the application. The library cannot infer a missing field or choose the game's compatibility policy.

**Acceptance:** round trips preserve every declared field; team/report writers and readers share the same envelope; checksum metadata is excluded consistently; wrong-type fields and unsupported versions produce typed failures; corrupt data is rejected instead of silently re-signed. A sample migration preserves an older supported save.

## AH-07 — Make staged world restoration a supported operation

**Helps original issues:** #43, #44. **Suggested priority:** high.

**Existing support and gap:** independent collections can already hold candidate state, but [replace_all_entities](vendor/afterhours/src/core/entity_collection.h#L492) clears singleton and permanence registrations and rebuilds runtime handle tables. It is a low-level replacement operation, not an all-or-nothing restore of a playable session. The game's [restore function](src/systems/GameStateLoadSystem.h#L187) changes wallet/health and creates entities before the whole payload has been accepted.

**Proposed library change:** document and expose a staged restore builder that creates entities, validates references, registers singletons, and prepares service state in a candidate world before commit. Define what happens to old handles, pending queries, and system caches at the commit boundary. A failed candidate must be discardable without touching the active world. Build on AH-01 and AH-06 instead of attempting to copy arbitrary polymorphic components.

**Game adoption:** decode and validate the complete save, prepare gameplay state, then commit it. Stage cloud data before replacing the local recovery file through AH-10; define recovery behavior if file replacement fails. Do not claim an in-memory swap and a disk write are one atomic operation.

**Acceptance:** inject a malformed final inventory entry or a construction failure after earlier entries succeeded; the active wallet, entities, RNG, registrations, and local recovery file remain unchanged. A successful commit exposes a complete state and invalidates old references according to the documented contract.

## AH-08 — Add a world-owned, serializable deterministic RNG service

**Helps original issues:** #1, #38, #67, #89. **Suggested priority:** medium.

**Existing support and gap:** [EntityQuery::gen_random](vendor/afterhours/src/core/entity_query.h#L463) accepts an RNG callback, and bitset helpers accept an external generator. There is no shared RNG state/lifetime service. The game's [SeededRng](src/seeded_rng.h#L15) is a process singleton used by both shop and combat; saves/replays do not preserve or restore its consumed state consistently.

**Proposed library change:** provide an optional RNG component/service with explicit stream identity, seed, full-state serialization, and restoration. Document stable sampling algorithms if client/server agreement across standard-library implementations is required. Make it easy to pass the service into existing query helpers; avoid a second process-global generator.

**Game adoption:** assign separate shop and battle streams, save the consumed shop state, capture the initial battle state, and restore it on replay restart. Canonically order candidate entities before drawing an index; deterministic random numbers alone do not fix nondeterministic candidate ordering.

**Acceptance:** save after K draws and restore into a new world; the next M values match uninterrupted execution. Shop draws cannot change battle draws, concurrent worlds do not share generator state, and replay restart reproduces recorded random choices.

## AH-09 — Generalize asynchronous work and delivery to the owning ECS thread

**Helps original issues:** #24, #25. **Suggested priority:** high.

**Existing support and gap:** the [pathfinding plugin](vendor/afterhours/src/plugins/pathfinding.h#L43) has request/response queue structures, but they are specialized to paths; its [request system](vendor/afterhours/src/plugins/pathfinding.h#L297) does not actually start background work. There is no general lifecycle-managed task/completion service for systems. Game network calls currently run during frame updates.

**Proposed library change:** provide an optional task service with owned worker lifetime, atomic value-returning queue operations, typed success/error results, cancellation, and an ECS-thread completion phase. Tag jobs with world/session generation and resolve generation-checked entity references only on delivery. Provide queued/running/succeeded/failed/cancelled state and configurable bounded retry scheduling. Do not move HTTP or TLS implementation into the ECS core.

**Game adoption:** execute health checks, battle requests, and save/load I/O off the update thread. Apply accepted results on the owner thread, expose terminal failures to UI, and choose retryable HTTP errors/backoff policy explicitly.

**Acceptance:** a slow or unreachable service leaves input and update ticks responsive; an obsolete response cannot mutate a restarted battle; failures do not automatically submit a new request every frame; shutdown cancels/joins owned work without dereferencing destroyed entities.

## AH-10 — Add reliable file writes and confined path resolution

**Helps original issues:** #12, #19, #36, #37, #43. **Suggested priority:** high.

**Existing support and gap:** the [files plugin](vendor/afterhours/src/plugins/files.h#L29) provides paths, iteration, and directory creation. It has no atomic-write operation or confined resolver. Its [settings writer](vendor/afterhours/src/plugins/settings.h#L178) opens the destination directly and does not check stream state after close. The game's [FileStorage](src/server/file_storage.cpp#L89) now has its own temporary-file implementation, so this is also an opportunity to consolidate a local fix.

**Proposed library change:** add a result-returning `write_atomic` utility that serializes before replacing the destination, uses a unique temporary sibling, checks write/flush/close/replace failures, and cleans up temporary files. Specify platform replacement behavior and the difference between atomic visibility and crash durability; offer an explicit durable mode if promised. Add a canonical path-component containment helper for read paths, with documented symlink behavior and limits when directories can change concurrently. Use these utilities in the settings plugin.

**Game adoption:** use the common writer for accepted saves and reports; persist and restore cloud saves at server boundaries; use confined resolution for opponents. Authentication, revision conflicts, and the decision to acknowledge a cloud save remain server responsibilities.

**Acceptance:** serialization, disk-full, and replace failures preserve the previous file and return failure; overlapping writes do not share a `.tmp` name; a sibling such as `opponents_backup` is outside the permitted root. For durable mode, test the stated persistence guarantees. Rejected cloud payloads never replace a recovery file.

## AH-11 — Make file roots injectable before any test initialization

**Helps original issues:** #91, #101. **Suggested priority:** high.

**Existing support and gap:** [ProvidesResourcePaths construction](vendor/afterhours/src/plugins/files.cpp#L33) chooses save/config folders from platform defaults and creates directories immediately. Its fields are public, so overriding them later is possible, but initialization has already touched the default location. The game also has a separate [Files singleton](src/resources.h#L12) and server save paths. A unique test user mitigates deletion but does not isolate all settings, reports, or writes.

**Proposed library change:** accept explicit resource/save/config roots before filesystem side effects, and supply a scoped temporary-directory test fixture with deterministic cleanup. Allow the fixture to bind paths to a test world and restore prior bindings safely. Document read-only shared resources separately from per-test writable data.

**Game adoption:** inject the same test roots into both the game and server storage adapters before loading settings or creating `UserId`. Keep test identity injection, and ensure test reset only operates inside the fixture's directories.

**Acceptance:** place sentinel player saves/settings outside the test directory, then run, fail, time out, and reset tests. Sentinels remain byte-identical. Two concurrent fixtures cannot read, overwrite, or delete each other's data.

## AH-12 — Unify headless operation across the platform and plugins

**Helps original issues:** #88, #101. **Suggested priority:** high; start here for the requested headless suite.

**Existing support and gap:** [RaylibHeadless](vendor/afterhours/src/backends/raylib/headless.h#L17) already creates an offscreen GL context. Pure ECS code and some no-backend drawing/input stubs also exist. However, [RaylibPlatformAPI::run](vendor/afterhours/src/backends/raylib/backend.h#L252) always initializes a window, [window_manager](vendor/afterhours/src/plugins/window_manager.h#L51) directly queries window/monitor APIs, and synthetic-input wrappers still depend on the selected platform API. Offscreen rendering requires a graphics context and is a different capability from running logic without graphics. The game currently maintains its own [headless loop](src/main.cpp#L263), preload guards, and [text measurement approximation](src/preload.cpp#L173).

**Proposed library change:** define a supported headless runtime configuration shared by the run loop, input, resolution, font measurement, texture/resource loading, and sound plugins. Provide a logic-only backend with synthetic input, supplied resolution, a CPU text-measurement provider, and no window/audio/GL initialization. Integrate the existing offscreen backend for tests that actually render. Expose capabilities and typed initialization failures; do not silently switch modes or omit requested tests.

**Game adoption:** select the runtime before settings/preload side effects, replace scattered platform guards with its adapters, and keep gameplay/UI update systems active in headless mode. Use AH-03 for stepping and AH-11 for isolation. Keep screenshot tests explicitly identified, with their graphics dependencies documented.

**Acceptance:** one documented command discovers and runs the full intended client/server unit suite with no display, audio device, or graphics context. UI interaction tests can inject input and perform layout using deterministic metrics. Required offscreen tests have a separate explicit capability requirement; missing support fails clearly. Headless mode never opens a window or reruns a failure visibly.

## AH-13 — Publish a machine-readable test result and discovery contract

**Helps original issues:** #82, #83, #89, #101. **Suggested priority:** medium.

**Existing support and gap:** [E2ERunner](vendor/afterhours/src/plugins/e2e_testing/runner.h#L232) already supports timeouts, resets, and `has_failed`; [ScriptResult](vendor/afterhours/src/plugins/e2e_testing/runner.h#L18) records per-script outcomes. Results remain internal and are printed as logs. There is no shared manifest/result protocol for that runner and the game's C++ test registration. The game runner has started correcting its log/exit-code handling, but still owns the protocol independently.

**Proposed library change:** expose immutable discovery/result records and optional JSON/JUnit output, with test IDs, requested mode, counts, skips with reasons, timeout/failure status, and one final completion record. Provide an entry-point adapter for registered native tests as well as E2E scripts, with explicit empty-discovery policy and exit-code examples. Keep normal logs separate from structured output.

**Game adoption:** adapt existing test registries rather than rewriting unit tests into the E2E DSL. Require successful process exit and a complete matching result set; treat crashes/timeouts as failures even if earlier tests passed. Mark checksum agreement as required once its prerequisites are repaired.

**Acceptance:** empty/failed discovery, an unknown test name, a truncated result, a signal, and a success record followed by abnormal exit all fail the outer runner. Headless and visible runs report their actual modes and the same intended test IDs, with explicit reasons for any approved exclusions.

## AH-14 — Document supported consumer builds and headless integration

**Helps original issues:** #87, #96. **Suggested priority:** medium; documentation/example improvement.

**Existing support and gap:** the [Afterhours README](vendor/afterhours/README.md#L9) lists macros and plugins, [PLUGIN_API.md](vendor/afterhours/PLUGIN_API.md) describes plugin boundaries, and example makefiles provide working patterns. A consumer still has to assemble a supported platform/backend combination, the headless entry point, and dependencies. The [headless validation example](vendor/afterhours/examples/headless_validation.cpp#L1147) renders both offscreen and windowed reference passes, so it is not itself a display-free unit-test command.

**Proposed library change:** publish tested minimal consumer examples for pure ECS, graphical UI, logic-only tests, and offscreen rendering, with supported compiler/backend flags and dependencies on macOS/Linux. Document windowless versus offscreen requirements, initialization order, reset/input/path adapters, and platform executable naming. Keep integration documentation synchronized with those build targets.

**Game adoption:** link the appropriate upstream example and document this repository's recursive submodule checkout, Zig/pkg-config/raylib prerequisites, executable paths, working directory, and server startup. The `.exe` hardcoding in the game runner remains a local fix.

**Acceptance:** a fresh checkout builds and runs each documented supported example with its stated dependencies; the logic-only command needs no display; the game's README points to an actual tested build/run sequence.

## Suggested implementation order

For headless tests, begin with AH-12, AH-11, and the update-only portion of AH-03, then add AH-13 and AH-14. For simulation correctness, prioritize AH-01, AH-02, AH-04, AH-05, and AH-08. For persistence, implement AH-10 and the narrowly scoped codec/restore support in AH-06/AH-07. AH-09 makes network integration responsive while preserving ECS ownership.

These are reusable improvements, not prerequisites for every local fix. Small application corrections can proceed immediately using existing collections, components, queries, standard-library utilities, and the current test runner.

## Decision for every original issue

“Local” means the fix belongs in this application's code, scripts, schemas, or docs; no specific additional Afterhours capability was justified. A gap reference means the proposed library work would materially help, even where a local implementation or mitigation already exists.

| Issue | Original finding | Afterhours relevance / local responsibility |
| --- | --- | --- |
| 1 | Concurrent HTTP handlers mutate the same ECS world and RNG. | AH-01, AH-08 |
| 2 | A fresh server battle is complete before loading its teams. | AH-03 |
| 3 | The server writes the player team in a format its loader rejects. | AH-06 |
| 4 | Server results can be reused from an earlier request. | AH-01, AH-02 |
| 5 | Missing results are replaced with invented winners based on base flavor totals. | Local — Preserve the authoritative result and remove fabricated-winner fallbacks. |
| 6 | Normal “Next Round” gameplay never reaches the HTTP battle/save flow. | Local — Connect the production Next Round action to the intended HTTP/save flow. |
| 7 | Battle export removes purchased level upgrades. | AH-06 |
| 8 | Purchased drinks disappear when teams enter battle. | AH-06 |
| 9 | Accepted powerups never affect instantiated combatants. | Local — Implement powerup semantics or reject unsupported loadout fields. |
| 10 | The HTTP client cannot reconstruct the selected opponent. | AH-06 |
| 11 | Any caller can read or overwrite another user's saved state. | Local — Authenticate callers and authorize access to the server save store. |
| 12 | “Save game state” is only an in-memory save. | AH-10 |
| 13 | Team validation accepts nonexistent and non-string dish types. | Local — Validate dish type against the application enum and playable set. |
| 14 | Duplicate team slots pass validation. | Local — Reject duplicate team slots in the team validator. |
| 15 | Fractional slot values are silently accepted as integers. | Local — Require integral JSON values before conversion. |
| 16 | Unbounded dish levels reach overflowing, potentially enormous loops. | Local — Enforce gameplay level bounds and checked stat arithmetic. |
| 17 | The single-opponent shortcut skips team validation. | Local — Apply content validation in the single-opponent branch. |
| 18 | A malformed opponent can abort fallback selection. | Local — Return validation errors or catch malformed candidates individually. |
| 19 | Path containment is checked with a string prefix. | AH-10 |
| 20 | The configured HTTP body limit is not enforced at ingestion. | Local — Configure the existing HTTP transport body limit at ingestion. |
| 21 | Server numeric configuration accepts unusable ranges. | Local — Define valid ranges for this server configuration. |
| 22 | A server that fails to bind exits successfully. | Local — Propagate HTTP listen failure to the process exit code. |
| 23 | Health checks treat an unhealthy service as connected. | Local — Define and honor this service's readiness response. |
| 24 | Network I/O blocks the game update thread. | AH-09 |
| 25 | Failed battle requests retry every frame without a terminal error state. | AH-09 |
| 26 | Malformed successful responses can throw out of the game loop. | Local — Guard parsing and validate the HTTP response before applying it. |
| 27 | Connection checks ignore the configured server URL. | Local — Use one application server configuration source. |
| 28 | HTTPS URLs are downgraded to plain HTTP. | Local — Retain URL scheme and configure a TLS-capable HTTP client. |
| 29 | URL parsing reports malformed endpoints as valid. | Local — Use a complete URI parser/authority validation in the HTTP adapter. |
| 30 | Battle fingerprints depend on process-local entity IDs. | AH-05 |
| 31 | Client and server hash different save payloads. | AH-06 |
| 32 | Updating a save compares its checksum to the previous save. | Local — Separate payload integrity from comparison with the previous revision. |
| 33 | A failed save-integrity check still overwrites server state. | Local — Reject bad checksums before mutating stored server state. |
| 34 | Older writes can replace newer progress. | Local — Implement server revisions, conditional writes, and reconciliation. |
| 35 | Save endpoints accept state the client cannot restore. | AH-06 |
| 36 | Saving truncates the last good file before replacement data is safe. | AH-10 |
| 37 | Storage helpers can return success after a failed write. | AH-10 |
| 38 | Save/resume resets the RNG sequence rather than continuing it. | AH-08 |
| 39 | Partial merge progress is lost on save/resume. | AH-06 |
| 40 | Save/resume does not preserve the drink shop. | AH-06 |
| 41 | A wrong-type local checksum escapes the load error handler. | AH-06 |
| 42 | A corrupt local save is accepted after its checksum fails. | AH-06 |
| 43 | Cloud state overwrites the local recovery file before validation. | AH-07, AH-10 |
| 44 | Failed restore leaves partially mutated game state. | AH-01, AH-07 |
| 45 | Shop and inventory slot IDs are confused across operations. | Local — Use area-plus-index slot identity in existing component/query APIs. |
| 46 | Continue loading is permanently disabled after its first attempt. | AH-02 |
| 47 | “New Team” does not create a fresh in-memory game. | AH-01, AH-02 |
| 48 | The menu prevents the offline save fallback from being used. | Local — Enable the intended offline Continue flow in the menu. |
| 49 | Save version metadata is written but never enforced. | AH-06 |
| 50 | Battle rewards and round advancement are not saved when applied. | Local — Trigger saving after the application reward/round transaction. |
| 51 | A stat modifier heals all accumulated combat damage. | Local — Preserve damage when recalculating base combat stats. |
| 52 | Deferred flavor buffs lose level scaling when combat begins. | Local — Choose and consistently implement flavor-buff scaling order. |
| 53 | Combat bookkeeping accumulates entries for deleted entities forever. | AH-02 |
| 54 | Trigger sorting does not define a strict weak ordering. | AH-04 |
| 55 | Multiple hooks from one source can lose their required order. | AH-04 |
| 56 | Course-complete abilities are emitted for entity zero and discarded. | Local — Emit the course hook for the intended participating dishes. |
| 57 | The final defeat can end battle before death effects resolve. | AH-04 |
| 58 | Target selection includes defeated dishes. | Local — Use existing component/tag/predicate filters for target eligibility. |
| 59 | Copying an effect can invalidate the vector currently being iterated. | AH-04 |
| 60 | Copied effects can be copied repeatedly without a recursion policy. | Local — Define copied-effect eligibility, depth, and duplicate semantics. |
| 61 | Bite-taken events carry damage dealt rather than damage received. | Local — Attach damage received to the correct recipient event. |
| 62 | Two different combat simulators run for the same client battle. | AH-03 |
| 63 | Natural completion skips publishing the processor's result. | Local — Separate completion from publication, or remove the duplicate processor. |
| 64 | The processor never advances past a missing slot. | Local — Handle missing slots according to combat rules, or remove that processor. |
| 65 | Battle speed is applied twice. | AH-03 |
| 66 | Restart replay does not reset the systems that load the battle. | AH-01, AH-02 |
| 67 | Replay restart does not restore the RNG state. | AH-03, AH-08 |
| 68 | Battle history converts filesystem timestamps with the wrong clock epoch. | Local — Use the standard filesystem-to-system-clock conversion. |
| 69 | Battle history never refreshes after its first load. | AH-02 |
| 70 | The result loader cannot read the report schema the project writes. | AH-06 |
| 71 | Replay pause does not pause the parallel battle processor. | AH-03 |
| 72 | Rerolling can offer drinks above the shop's tier. | Local — Reuse the existing tier-filtered drink generator. |
| 73 | Rerolled drink sprites do not match the purchased drink. | Local — Reuse the existing drink factory and type-specific sprite lookup. |
| 74 | A partially empty drink shop stays empty across rounds. | Local — Refill eligible empty drink slots at round advancement. |
| 75 | Replay progress never receives a nonzero total duration. | AH-03 |
| 76 | Atlas packing silently clips images wider than the requested sheet. | Local — Validate sprite width in the project's Python atlas packer. |
| 77 | Wrapped atlas groups advertise only their last row. | Local — Record all rows covered by an atlas group in that packer. |
| 78 | Normal source changes do not regenerate the compatibility hash. | Local — Declare the actual source dependencies of the generated build hash. |
| 79 | Hash discovery omits shared gameplay implementation files. | Local — Include the full application gameplay source/dependency set in the hash. |
| 80 | Coverage and normal builds reuse incompatible object files. | Local — Separate build configurations and object directories in this Makefile. |
| 81 | Coverage runs overwrite each other's default profile. | Local — Assign unique LLVM profile output paths and merge profiles. |
| 82 | The test runner marks crashes and timeouts as passes. | AH-13 |
| 83 | Failed test discovery can produce an all-green zero-test run. | AH-13 |
| 84 | `--skip-lint` breaks server test execution. | Local — Remove the Python conditional-import scoping error. |
| 85 | The test server can block when its output pipes fill. | Local — Drain child-process output or redirect it to run-owned log files. |
| 86 | Starting tests kills unrelated battle servers. | Local — Track owned child PIDs and allocate isolated server ports. |
| 87 | The unified runner cannot locate Linux builds. | AH-14 |
| 88 | Headless integration tests are silently forced into visible mode. | AH-12 |
| 89 | The checksum agreement test is excluded from the default suite. | AH-05, AH-08, AH-13 |
| 90 | The invalid-team endpoint test fails at the wrong validation layer. | Local — Send a valid code hash and assert the specific team-validation error. |
| 91 | Running client tests deletes the developer's real save. | AH-11 |
| 92 | The dead-code script passes clang-tidy options to the compiler. | Local — Fix clang-tidy argument placement and propagate analyzer status. |
| 93 | The compilation database points at a different checkout and obsolete build. | Local — Regenerate the compilation database from the actual local build. |
| 94 | Dependency-graph Make targets reference missing tools. | Local — Restore or replace the missing project dependency-graph tool. |
| 95 | Coverage generation hides failures and reports nonexistent reports. | Local — Propagate coverage-tool failures and verify report artifacts. |
| 96 | The README's setup instructions are insufficient for a fresh checkout. | AH-14 |
| 97 | The endpoint verification guide tells users to run a missing script. | Local — Update project endpoint-verification examples to existing commands. |
| 98 | The endpoint guide describes a request-level debug switch the API ignores. | Local — Document or implement the actual endpoint debug contract. |
| 99 | The feature status table marks implemented subsystems as nonexistent. | Local — Refresh application feature status against its current implementation. |
| 100 | The combat status document describes obsolete combat rules. | Local — Update the application combat rules/status documentation. |
| 101 | Make the full unit-test suite run headlessly. | AH-01, AH-03, AH-11, AH-12, AH-13 |
