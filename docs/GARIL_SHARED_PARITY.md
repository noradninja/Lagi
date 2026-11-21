# Garil shared runtime parity

**Hardware update, 21 November 2026:** The user confirms Scatter works as expected on `diagnostics/garil-cliff-current`. Other combat still crashes. Death lifecycle remains accepted. The constructor/damage repair below is a new source checkpoint awaiting device confirmation.

**Current position (20 November 2026):** The working combat tree is `60d7b61`, restored by rollback `977809e`. Experimental WEAK-marker depth and tracing changes after that checkpoint regressed Valley combat and were reverted. The native Scatter command, A5 escape/actor signaling and quadrant-eligibility changes described below remain in the restored tree; WEAK marker depth is still unresolved. This is a dated investigation record, with the latest acceptance boundaries in [STATUS.md](STATUS.md).

Active goal, 2026-11-17: reuse the working Valley battle, result, story,
cutscene and field-transition services in Garil and later areas. Restore
area-specific authored actors and callbacks through Lagi adapters. Do not
force story flags, replace authored sequences, or modify extern/Azel.

## User evidence and reproduction

The user confirms targeting and attacks work with 1f0148a. Queen gun death
fails to Scatter/end combat; dead enemies lack death motion and retain life
bars; rewards are absent. A subsequent encounter crashes. After the pillars,
the worm story trigger runs without the red-worm cutscene/opening sequence.
These are separate acceptance gates; the earlier cliff/clipping fixes remain
accepted.

Latest Camp save: PANDRA_3_02, 1,252 bytes, SHA256
C87DC5621D297263B15BF81BF73748FE5EB3F658A3246C715696656F060BF395.
Leave Camp and select Garil. Preserve the original file. Saturn save conversion
must map the port's native C++ fields explicitly; a raw memcpy of its stats is
not a valid native Saturn save. Earlier diagnostic emulator checkpoints used
an incomplete conversion, so they are not progression-acceptance evidence.

## First checkpoint: death notification and motion

Original BTL_A5 0605E706..E778 cleans up targetables, increments the native kill
stat, notifies the existing life meter with bits 1 and 8, sets gravity -0x160,
and selects death angular speeds from two RNG calls. The port omitted those
calls and left gravity/angular speeds zero. The adapter restores this tail.

Valley Baldor and Garil Nanyd now call the same typed defeat notification.
Azel's existing battleEnemyLifeMeter remains the sole owner of COMMON-derived
XP/Dyne accumulation and shrinking/removal. Result screens, growth, inventory,
saving and field return retain their existing shared ownership. No independent
Garil XP table or result pipeline is introduced.

Validation: production meter-update host fixture checks a nonlethal hit,
one reward lookup/award, position detachment, meter retirement and a negative
record ID. Fixed-point arithmetic and engine services are mocked; this is
lifecycle coverage, not numerical/rendering parity. Vita VPK compilation passes.
Hardware death/reward acceptance remains pending. Scatter, enemy attacks,
subbattle 4 constructor/lifecycle, worm cutscene and end-to-end return/re-entry
remain open. This checkpoint does not claim those are fixed.

## Native trace findings still to implement

- Queen loss at formation 0605CDFA..CE4E waits for intro completion, checks
  m1D4 and mF0, enters battle mode 7, clears formation attack state and sets
  formation state 9. States 9/10 issue Scatter and signal surviving actors.
  The upstream simplified formation omits this path.
- Subbattle 4 calls BTL_A5_createEnemySingle. The port reads
  mE0_entityGroup.m8_dataTable despite never initializing it. Original
  0605BC3C..BC46 instead reads the enemy type at entity-group +0xC directly.
  Original position table is entity-group +8, selected from COMMON by
  0605AF6C. Model construction/target/damage/formation accesses also require
  typed conversion before treating this encounter as implemented.
- Shared formationTriggerAttack/Followup are stubs because native enemy
  work-area offsets cannot describe expanded C++ tasks. Implement a typed
  actor interface rather than adding byte-offset casts for each overlay.

## Enemy record identity correction

Both A5 enemy constructors incorrectly read a word at position-table +0xC.
Swarm positions there start with 00009000, so this selected enemy record 0
instead of the group's native 0xF/0x10 records. Subbattle 4 did the same read
through a null table. Original 0605E2B4 and 0605BC42 load entity-group +0xC
directly and add the leader variant. Both constructors now share that typed
lookup. This repairs a definite invalid read and restores authored HP/reward
identity, but other subbattle 4 constructor/target/damage accesses remain open;
no whole-encounter crash acceptance is claimed.

## Hardware acceptance update and formation control trace — 2026-11-18

The user confirms that checkpoint 1 (the shared enemy-death lifecycle,
including death motion, meter retirement and rewards) works as expected in the
current Vita build. Treat this as hardware acceptance for that tested path,
not proof that all encounters or variants terminate correctly. No need to
retest checkpoint 1 as the primary next gate unless a later change regresses it.

Next gate: restore the authored queen-loss -> Scatter -> encounter-completion
control flow using existing Azel battle and formation facilities. Do not
invent an A5-specific battle exit or directly set result/story state.

Pinned repository baseline: `816c5aa9bca3424e261cabb85e3a2b7a0dc642ef`;
pinned Azel submodule: `4cebb24b91ed1499b8754cf992c2fc90dbfb044d`.

### Trace of the known implementation gap

- `AzelLib/battle/BTL_A5/BTL_A5_formation.cpp`:
  `BTL_A5_formationTask_Update` (0605CD10) increments the engine's active
  formation count; removes finished entity pointers; enters cleanup state
  0xB when every entity is dead. Its current switch only handles 0, 1, 2,
  3, 4, and 0xB; **9 and 10 are missing** and fall through the default,
  which forces state 0xB. The 0xB timer subsequently releases the two NPC
  references and marks the formation finished.
- `BTL_A5_enemy_Update` increments `mF0_flag` when a lead enemy is killed
  and increments `mE0_entityGroup.mF_deadCount` when any enemy is killed.
  The Vita death adapter now also performs cleanup and shared meter
  notification. These counters must remain distinct: leader loss is not
  identical to all-enemies-dead.
- Prior Disc 1 native trace locates the leader-loss branch around
  `0605CDFA..0605CE4E`: it checks `m1D4_flag` and `mF0_flag`,
  advances through battle mode 7 and formation states 9/10, signals
  surviving actors, and initiates Scatter. The exact arguments, branch
  order, counter timing and action flags **must be pinned from BTL_A5.PRG**
  before implementing these states; these details are not established by
  the upstream C++ translation alone.
- `AzelLib/battle/battleFormationBase.cpp`:
  `formationTriggerAttack` (0609B60A) and
  `formationTriggerAttackFollowup` (0609B65E) already iterate entity
  arrays and remove finished references but leave the live-actor flag
  write/clear as `Unimplemented()`. They currently expect Saturn's
  `entity+0x14` attack-flags layout, which cannot be assumed for a native
  C++ task.
- The shared `formationSubStateMachine` has the established
  attack/follow-up protocol (states 2/3/4) and calls those functions.
  Restore a typed live-actor command/flag interface at the shared layer
  and make the A5 actor satisfy that interface; do not introduce unsafe
  Saturn offset casts or an A5-exclusive replacement engine.

### Implementation acceptance boundaries

1. Verify the native queen-loss branch using the exact Disc 1
   `BTL_A5.PRG` bytes; distinguish the conditional leader-loss transition
   from the all-enemies-dead cleanup path.
2. Implement the smallest typed actor interface required by the shared
   formation signaling services, with a host fixture for set/clear and
   finished/null entities. No modifications to `extern/Azel`.
3. Restore A5's missing authored states via a guarded Lagi source adapter.
   Test the state transition, signal lifetime, one-shot Scatter, and ordinary
   all-dead retirement independently before a Vita build.
4. Hardware acceptance: queen gun death signals the survivors, displays
   the authored Scatter action, permits the encounter to end normally,
   and reaches the existing battle results/field return path; no regression
   to the already accepted death/reward lifecycle.

This trace checkpoint is documentation only: it does not claim Scatter is
implemented or hardware-tested.

## Disc 1 original PRG queen-loss disassembly checkpoint — 2026-11-18

Received three distinct PRGs. **Only `BTL_A5.PRG` (418,316 bytes,
SHA-256 `639328c247713e686e6b638caa94396092e7cc89e46cfab66f3d09067dfc0186`)**
was selected for this trace because its code at 0605CD10 matches the original
routine entry. The other uploads (`BTL_A5_2.PRG`, 419,380 bytes; and
`BTL_A5_3.PRG`, 365,872 bytes) differ and must not be mixed into the
Disc 1 interpretation without identifying their overlay mappings.

For the selected file, Saturn VA `0x06054000` corresponds to file offset
zero; native function `0605CD10` starts at offset `0x8D10`. The byte
sequences are from the uploaded file, not inferred from the incomplete C++
translation.

### Confirmed native control-flow details

- At `0605CDF4..CE06`, the branch uses the result of a previous
  original helper, checks formation field `+0x1D4` and field `+0xF0`
  and skips the special path if the required conditions do not hold.
- At `0605CE0C..CE4E`, that path sets battle mode **7**, clears the
  shared engine control fields, writes formation state **9**, and
  branches into the common update tail.
- The jump table based at `0605CEA8` indexes states 0 through 11.
  **Correction to the previous trace note:** state **9** starts at
  `0605D0A0`; state **10** branches to the generic state-11 cleanup
  transition at `0605D172`. State 8 starts at `0605D06A`.
  The earlier loose description of states 9/10 both initiating Scatter
  was imprecise.
- In state 9, `0605D0A0..D0B2` waits for battle-engine flag
  `0x2000`. When set, `0605D0B4..D0F4` loops the formation entity
  slots. Each live entry's work-area pointer is converted to its task
  header by subtracting `0x18`; the task flags at header `+0x14`
  are tested for bit 1 (finished). Surviving entries receive **bit 8
  in the task-header flags**, using the same `+0x14` field, rather than
  a write to the enemy's `m14_flags` member.
- State 9 then calls native `06070A7E` with `r4=3, r5=40, r6=0`,
  sets battle-engine flags at `+0x188` using masks `0x00010000`
  and `0x00000100`, and advances to formation state **11**.
- Upstream `s_task::m14_flags` currently defines finished=1,
  paused=2, deleting=4, and does not assign a meaning to bit 8.
  Upstream A5 enemy update also has no explicit response to task flag
  8. Merely writing the native bit into a C++ `s_task` would **not**
  restore its original consumer semantics. Those semantics require
  an independent task/actor trace before writing a functional adapter.

### Decision

Do not equate native task-header bit 8 with `sBTL_A5_Enemy::m14_flags`,
or shortcut it to `markFinished()`. Determine its original scheduler or
actor response, plus the 06070A7E display command, before coding Scatter.
The absence of state 9 in the upstream C++ switch is confirmed; the
missing **consumer** of bit 8 is a separate concrete integration gap.
The next useful change should be an atomic implementation of the
correct typed actor/task signal, with host coverage for live and
finished entities, then the A5 formation-state restoration.

This is a static PRG/source trace, not a Vita gameplay fix.

## Correction: native Scatter producer AND consumer — 2026-11-18

**Supersedes the mistaken task-header destination claim in the preceding
disassembly checkpoint.** The native instruction flow uses two different
pointers, not one:

- `0605D0C8..D0D0`: take the entity work-area pointer, subtract `0x18`
  to reach its task header, then inspect header `+0x14` bit 1 to skip a
  finished task.
- `0605D0D2..D0E0`: reload the entity work-area pointer (without
  subtracting `0x18`), load its **own** `+0x14` flags, OR with bit
  `0x8`, and write that work-area flag back.
- `0605E814..E85E`: the native A5 enemy state-zero path checks work-area
  `+0x14` bit `0x8` at `0605E844..E848`. If set, it calls
  `0605EFEC`, invokes `06061CF6`, and switches the enemy state to
  **6**. Thus the original flag has an actual area actor consumer and
  is not a scheduler signal.
- `0605E936..E9DA`: state 6 updates escape movement relative to the
  battle position and advances its 16-bit frame counter. At 90 frames
  it resets the counter and marks the enemy task finished when the
  appropriate formation guard permits it.
- The original state-9 call `06070A7E(3,40,0)` invokes the battle
  command/display pathway (eventually `0601C018`); its exact text
  presentation mapping to existing Azel functions is still to be pinned.

The previously committed generic **task-header** bit-8 helper is not a
correct Scatter implementation; do not call it from A5. Replace it with a
typed actor work-area flag interface, keeping finished-task filtering
separate. Restore the native state-0 -> state-6 escape sequence; do not
shortcut survivor retirement or change the shared result owner. Source
for these findings: uploaded Disc 1 `BTL_A5.PRG` at the addresses
above and pinned upstream A5 actor/formation implementations.

## First end-to-end Scatter Vita candidate — 2026-11-18

Implementation branch checkpoint: `33c8badc9cd088845454388f0da5472079512374`.

**Important correction:** the native state-9 loop tests TASK HEADER +0x14 bit 1
for finished, but writes bit 8 to ENEMY WORK AREA +0x14. The historical
mistaken task-flag helper was removed; `lagi_formation_actor_signal.h`
iterates typed Azel entities and dispatches actor-specific writes. A5
registers `lagiA5SignalActorFlags` to set its `m14_flags` bit 8.

Atomic functional changes:
- `b709b54`: replace incorrect task-header signaling with typed work-area
  actor signaling; retain finished/null filtering.
- `fbda4d5`: register the A5 typed flag receiver in generated Azel source.
- `0d39bb3`: restore A5 state-0 -> state-6 escape and count/finish
  boundary, without death rewards.
- `6592d1a`: restore lead-death guard and formation state 9, feeding the
  existing battle mode 7 and shared completion flags, never a private
  result/exit pipeline.
- `6d82818`: add executable production-include host fixture for queen
  loss, intro gating, once-only signal, ten-leader variant, and normal
  all-dead guard.
- `33c8bad`: retire the separately dragon-owned enemy life meter when
  an enemy begins escaping, avoiding dangling position/health references
  and without granting XP/Dyne.

Static source boundary audit: original upstream A5 actor and formation
anchors each occur exactly once; current CMake patch references all
added integration files. An equivalent formation sequence was compiled
and run with GCC on the analysis host; the committed MSVC fixture can
be run with `tests/battle_a5_scatter_sequence_test.ps1`.
**This is not a verified Vita build** and is not whole-game acceptance.

### Targeted hardware test

1. Pull `diagnostics/garil-cliff-current` at the candidate commit and
   build with the usual native battle flags. Start from the established
   pre-join Camp save, progress into the first Nanyd swarm.
2. Shoot the lead/queen Nanyd while leaving survivors alive. Confirm
   queen death/rewards still use the accepted shared lifecycle.
3. Observe whether remaining Nanyds receive the escape behavior, their
   life meters disappear, and the encounter transitions through the
   existing results then field return. Neither a hang nor a crash is
   acceptable.
4. Record video plus log, particularly any battle-mode transitions
   following the queen's death. Check whether the sequence works on
   repeated encounters after returning to the field.

Limitations still open: native `06070A7E(3,40,0)` Scatter display call is
not yet mapped to its existing battle text/presentation service, so the
authored text may be absent. The A5 state-6 escape currently restores the
state and retirement timer; its detailed native movement requires a later
parity pass. Subbattle-4 enemy constructor/lifecycle and worm story
cutscene remain separate acceptance gates. A Vita test is intended to
validate the handoff and termination first, not claim visual parity.

## Queen-loss Scatter presentation and escape motion, plus weak markers — 2026-11-18

**Hardware report at prior `4645997` checkpoint:** queen loss ends the fight,
but no "Scatter" UI is displayed and surviving Nanyds do not visibly fly
away. Weak-point sprites remain visible from the wrong battle quadrants.

Disc 1 `PDS_Disc_1_contents.zip` is in the user Library; the archive includes
`BTL_A5.PRG`, `BTL_A5_2.PRG`, `BTL_A5_3.PRG` and all accompanying
original Disc 1 data. The Disc 1 A5 executable provides the following
direct cross-checks:

- The text table at Saturn `060ABDE4` references `060ABC24` ("Nanyd
  Swarm"); its fourth pointer at `060ABDF0` references `060ABC42`,
  the literal **"Scatter"**. State 9's native call
  `06070A7E(3,40,0)` therefore maps to the existing shared
  `battleEngine_displayAttackName(3,40,0)`. The adapter now calls
  that service, not a one-off VDP1/VDP2 label.
- `0605E84E..E85E` consumes the enemy work-area bit 8 and enters state 6.
  The native helper `0605EFEC..F090` initializes three fixed-point
  escape offsets at actor fields `+0x170,+0x174,+0x178`. It uses a
  `-0x14000` Y bias and normalizes/scales with the `0x60000`
  value before cleanup of targetability.
- `0605EAA4..EB20` reads each entity's current formation position,
  adds its offset vector and orbit velocity, and passes the resulting
  target to the normal position interpolation. The upstream simplified
  C++ actor still models fields `+0x170..178` as unused padding and
  omits their addition to the movement target. The Lagi guarded adapter
  now restores typed `m170_scatterOffset` and its state-6 integration.
  **Limitation:** the Vita typed battle center substitutes for the
  native reference-position transform. This is a directional parity
  step, not proof of exact Saturn path geometry.

Weak marker trace:

- Authored target eligibility is already computed in
  `battleOverlay_C.cpp`, with each target's high-nibble quadrant mask
  and current one-based selectable index `m5A`; do not rework the
  selection engine or invent per-enemy facing flags.
- `battleTargetable.cpp` has **two separate UI families**.
  `sBaldorSubTask0_draw` emits the ordinary normal/WEAK reticule, while
  `sBaldorSubTask1_draw` emits the selected-target marker. The first
  guarded change covered only subtask1. Follow-up commit now gates
  both subtask0 draw methods on native current-quadrant eligibility,
  without requiring selection bit `0x200000` for ordinary markers.
- Both marker families also check the current selectable index and
  authored quadrant mask at draw time, preventing stale one-frame
  visibility when the dragon changes quadrants.

Relevant atomic commits:

- `7b77a91`: restore the shared Scatter display command.
- `6ef4002`: add once-only command assertion to host sequence fixture.
- `87d89f0` + `c1f3646`: selected-marker gate and CMake escape fix.
- `94a3160`: native A5 escape offset integration and host vector fixture.
- `a0c2a30`: ordinary WEAK reticule draw gates.

**Next Vita acceptance:** repeat the queen-loss sequence with survivors:
verify Scatter text, survivor flight motion and existing result/field return.
Also orbit an enemy with a rear-only weak point through all four quadrants;
the WEAK sprite must appear only in authored eligible quadrants and remain
targetable when visible. Gather video/log for comparison. No change to
the previously accepted queen defeat rewards is intended.

## Subsequent-encounter constructor and shared damage repair � 21 November 2026

Baseline: remote 8284343, following the accepted 977809e combat rollback.
The old staged Scatter experiment files were preserved and are not part of
this change. No accepted Scatter or WEAK-marker behavior was changed.

Original BTL_A5 subbattle 4 uses constructor 0605BBD0 and formation setup
0605AE48/0605AF6C. The port selected the enemy record correctly after 816c5aa,
but still left the formation position table null, dereferenced it as a
second descriptor, used model index 4/8 as an animation index, and accessed
formation +4/+8 as host pointers in the first enemy update. Those are definite
invalid paths even without attributing the latest crash to an unmatched dump.

The guarded generated adapter now:

- Selects the original three formation position tables from the native
  random selector: 060AF954, 060AF99C or 060AF90C; preserves its postincrement.
- Uses shared lagiCreateBattleEnemyModels for model descriptors 060B0028/30,
  animation bundle slot 70, and collision model C with pose 6C and its own
  bone count. Model, pose and animation indices are distinct.
- Registers model targets through lagiInitBattleEnemyModelTargets, with the
  actor owner, entity index, original hotpoint metadata and A5 hit callbacks.
- Uses the typed group position and the shared native positional interpolation
  on the first and subsequent updates, instead of dereferencing host offsets.
- Runs the existing shared model damage service (hit callback order, damage
  accumulation/display, HP and meter notifications) and shared once-only
  target retirement. Teardown retires the same registered model targets.
- Maps hit/death counters to named group fields. The existing named byte buffer
  stores the native per-entity death words via memcpy, without alignment or
  polymorphic-task offset assumptions.

Existing shared result/progression and field-return services remain the owners
of victory, XP/Dyne, growth and return. No new area-specific reward system.

Validation: garil_single_init_test.py executes the production model initializer
for both variants, all initialization failure stages and each original formation
choice; original captured Saturn instruction anchors pass. Shared model/target
fixture passes 8,192 cases; shared damage fixture passes 29,040 cases plus live
callback, override and death/retirement ordering cases. Vita VPK compilation and
linking pass. These host services are mocked; this is not a Saturn playthrough
or hardware acceptance of the repaired encounter.

Open: native enemy attack selection/effect producers and shadow behavior in this
archetype still contain stubs. Whole-battle entry, player attacks, death/result,
field return and another encounter require matching device verification. The
red-worm cinematic and passage transition remain a separate investigation.

## Red-worm event trace and shared camera support � 21 November 2026

The day proximity trigger at 06067FDC already starts field script 2/5B7, but
its actor factory, tracker and camera calls are stubs. This explains why the
story dialogue can run without a visible red-worm scene. No story bit or trigger
condition was forced or changed during this trace.

Original 060666E8 selects task descriptor 0609D7BC: init 06066028, update
060662BC, draw 06066584. The adjacent descriptor 0609D7CC starts at 060666F8
and is a DIFFERENT actor. Do not implement the first factory using that adjacent
initializer merely because it follows the factory in address order.

The first event data is 0609E790 (type 0, eight parts, original position and
rotation). Tracker 06067EE2 uses name table 0609DBC6, Y threshold -0x1DE000
and bit 0x74B. Callback 06067F12 recreates that same authored actor/tracker
for the camera continuation. Camera 0606F9E4 passes flags zero to shared
060720C6, equivalent to cutsceneTaskInitSub2 after deserializing its script.
Actor creation/update/draw, tracker completion and typed callback dispatch must
be implemented together before the event is wired. The camera's flags-zero
path is distinct from its optional moving-subject tracking path.

One missing COMMON camera capability was also identified: s_cutsceneTask2's
moving-subject arm asserts instead of adding the live subject to its current
relative target. Original A5 06071D1A..D4C performs that three-axis addition
every update. The guarded shared adapter restores it, preserving the fixed-target
arm, camera trajectory, segment timing and existing completion behavior. This
supports authored tracking scenes across areas; it does not by itself restore
the flags-zero worm event or claim that its actor now appears.

Validation: field_camera_tracking_test.py compiles the actual generated Update
and checks a moving subject across ten frames, fixed-target behavior, segment
advance, held completion and initial camera tracking. Original FLD_A5.PRG
factory/descriptor/tracker/camera anchors pass garil_worm_contract_test.py.
Vita package compilation/linking passes. A full worm scene and the passage
opening remain unimplemented/unverified; no hardware test is requested for this
support-only checkpoint.
