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

## Hardware acceptance update and formation control trace â€” 2026-11-18

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

## Disc 1 original PRG queen-loss disassembly checkpoint â€” 2026-11-18

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

## Correction: native Scatter producer AND consumer â€” 2026-11-18

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

## First end-to-end Scatter Vita candidate â€” 2026-11-18

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

## Queen-loss Scatter presentation and escape motion, plus weak markers â€” 2026-11-18

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

## Subsequent-encounter constructor and shared damage repair — 21 November 2026

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

## Red-worm event trace and shared camera support — 21 November 2026

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

## Story-worm actor and event wiring checkpoint — 21 November 2026

Implemented the actual 0609D7BC actor via Lagi-owned typed fields and the shared
monsterBody, animation, resource, collision-draw and particle services. It loads
memory area 1 (FLD_A5.MCB, not FLD_A5_0.MCB), preserves the authored head/body/tail
templates and model/pose/animation tables, and draws only above-ground segments.
Its native pitch/roll damping, triple-product velocity, body spring updates,
ground-level camera anchor and head breach sound/particles are restored.

The command tracker consumes one original signed word per frame, adjusts the
six angular directions and speed, and retires the actor on the negative sentinel.
Crucial correction to earlier upstream comments/notes: actor +0x10 is POSITION Z,
not Y. Native 06067E1E compares Z with -0x1DE000 for the first passage flag.
The original bit is set once at that threshold and again idempotently on stream
completion; field-control bit 0x8000 is released at completion. There is no
forced early discovery, replacement path or fixed delay to unlock the passage.

Proximity day/night calls now use that factory and tracker and the existing
cutsceneTaskInitSub2 camera service with original scripts/callback EAs and
flags zero. Actor initialization failure leaves the trigger eligible to retry
instead of publishing a false discovery bit. Camera script vectors have stable
per-script storage. Native callback EAs are retained by the shared camera
service; their dispatch purpose and typed invocation still need a separate
parity check, so normal playback is the current validation path.

Validation: the production tracker passes all 4,096 low-12-bit command masks,
Z-not-Y threshold timing, once-only threshold publication, negative sentinel
cleanup, unrelated field-flag preservation, speed floor and original sound
frames 0/30/45. Original PRG contracts and all three types' model/pose/animation
bindings pass against FLD_A5.MCB. Vita compilation/link/package passes.

At this checkpoint the native per-segment sand/debris producers 0605F86C/
0605F804 were still unimplemented; the later segment-effects checkpoint below
restores them through the shared pool. Full scene visibility, camera timing,
passage opening/entry and return need
matching Vita video/log confirmation. This checkpoint is not full visual or
story acceptance.

## Latest hardware steering and expanded checks

The user confirms the second battle works using a70d109. Its authored Gash
warning ('Watch out for the ones with horns.') is absent: native 0605B2B2 calls
shared battle dialogue with subtitle index 9 and B_A5_03.PCM at 060AF9F4. This
must use the existing subtitle/PCM lifecycle, just as the first battle does.
The main area exit cannot be selected and needs an LCS/transition trace.

The supplied 1,640,057-byte lagi.log (21 November, 16:35 file timestamp) contains
57 presentation windows. Of 53 windows without a >100 ms outlier, average rates
range 20.609..29.970 fps. The slowest has 109/120 frames at three VBlanks,
mean 48,520 us, preRender 3,619 us, publish 2,491 us, present 13,916 us. This
establishes repeated missed deadlines, not a rendering/CPU root cause. The user
will supply video after the next run for graphical glitches; do not change
culling or offsets speculatively. Exit, warning, performance and visual checks
remain within the active parity work, alongside full field return/re-entry.

## Second-battle Gash warning — 21 November 2026

Original 0605B2B2..B2C4 passes subtitle index 9, PCM name B_A5_03.PCM
(060AF9F4), and the shared battle-dialogue function 06070E0E. The actual
subtitle is "Watch out for the ones with horns." State 2 compares its old
counter with 0x4B (75), not the first encounter's 0x54.

The existing battle opening subtitle/PCM task now accepts an authored subtitle
index and PCM name. The first Nanyd opening keeps its existing 4/B_A5_00.PCM
binding. The second formation invokes the same lifecycle at its original
0x2000 gate, restores the existing camera and retains the native 75-frame
handoff. Other formations can bind their own original index/file through that
same service; no second playback or subtitle implementation was added.

Validation: both production openings pass the shared lookup/submit-retry/drain/
cleanup host fixture. Original captured BTL_A5 instruction and string checks
pass garil_battle_dialogue_evidence_test.py. Vita package builds. The user's
hardware confirmation covers combat before this warning was added; voice/text
and scene timing for the warning still require the next matching run.

## Main-exit observations for the next combined capture

The exit's mode 5 waits for isScriptActive()==0 before changing its target flags
to 3 and entering selectable mode 6. Selection advances it to 7 and starts the
original exit cutscene. An unfinished worm dialogue/script could keep that wait
active, but the current log does not prove this cause. Do not bypass that wait,
force the discovery bit or enlarge selection bounds.

The next build logs [GarilMainExit] on mode/script-state changes and periodically
while the exit is visible, capped at 64 reports per actor. It records script EA,
discovery state, visibility/disable flags, registration in the current LCS list,
selection, LCS state/count, previous projected view/screen coordinates and native
selection range. These are observations only; authored exit behavior is unchanged.
The records distinguish script waiting from missing registration or rejected
selection. One capture of the warning, worm sequence and attempted main-exit
selection can cover the outstanding questions, together with the promised video
for visual glitches and frame pacing.

## Shared field-particle payload retirement - 21 November 2026

The original A5 pool update at 0608045E..6A checks the completed slot's
payload, calls freeHeapForTask(pool,payload) through literal 060804CC
(060151FE), then clears the payload and draw pointers. Upstream's shared
fieldParticlePool.cpp only cleared the payload pointer. Each payload stayed
on the pool task's heap list until scene teardown; Lagi's allocator walks
that list when allocating the next payload. This is a proven lifetime and
allocation-cost defect, not proof of the reported Garil frame-rate cause.

The generated adapter now restores the native free before clearing the pointer
for every field using this shared pool. It retains live payloads, active-count
accounting, full-pool behavior and allocation-failure behavior. No area-specific
particle engine or guessed culling/performance changes were introduced.

Validation: field_particle_lifetime_test.py executes the production shared
spawn/update functions with the actual CMake replacement, checks 30,000 payload
retirements/reuses, live and payload-free slots, full-pool rejection and
allocation failure. It also verifies the original cleanup instructions and
free-function literal against FLD_A5.PRG. Vita compilation/link/package passes.
Performance and visual acceptance still need the next matching hardware run.

## Native worm segment effects through the shared pool - 21 November 2026

Original 0605F86C emits four stationary projected particles, using the quad list
at 0608C6E4 and independent centeredRandom(0xFFFF) offsets on X and Z. Original
0605F804 uses quad list 0608C5E8, zero offsets and a two-cycle payload. Its
060805C8 update reads the live segment X/Z, keeps particle Y at the supplied
vertical offset, and decrements the cycle count only when the animated quad
reports bit 2. Both producers now call the existing shared field-particle pool.

The story actor checks every segment after its body update (060664E4..655A): a
ground crossing emits sand and resets its buried-effect latch; an odd buried
segment emits its attached effect once. The actor's native m110 threshold input
is zero-initialized and not written by its initializer/controller, so its
half-negative threshold is zero. The environmental worm uses its authored
radius threshold and exits the segment loop after the first emission
(06059EA0/06059ECA); that different policy is preserved.

The X/Z-following update is a shared service with an owning alias to the
segment's body storage. Actor retirement releases its ownership; any still-live
particles retain the position until their two cycles finish. Shared pool
retirement destroys that payload before freeing it. Pool teardown also releases
remaining aliases, independently of actor/pool deletion order. Both concrete
worm owners use the existing typed work-area disposal service; upstream Azel
files remain unchanged.

Validation: production producer/loop fixtures check quad bindings, random-call
order, four zero-velocity sand sprites, two-cycle binding, all-segment story
crossings and once-only buried latches. Original PRG checks verify both quad
pointers, the live-X/Z updater, cycle decrement, and the different environmental
versus story loop branches. The shared pool fixture additionally exercises live
movement, event-bit gating, two-cycle retirement, failed allocation, producer
retirement and pool teardown. Tracker fixtures still pass all 4,096 control
masks; all original worm/camera/model contracts pass. Vita package builds.

These are source/host/build checks. Worm appearance, timing, effects, passage
opening/entry, the Gash horn warning, main exit and performance remain pending
the next hardware video/log. The previous exit observations are retained; no
exit story gate, lock-on bounds or culling behavior was bypassed.

## Shared type-1 camera roll repair - 21 November 2026

The original camera step initializer 06071BF6..C2C loads roll endpoints from
the current record's +0x34/+0x38, subtracts start from end, normalizes the signed
28-bit angle and divides by duration. Upstream fieldCutsceneTask2.cpp instead
subtracted the task's mode/index fields. Its frame update also wrote roll into
rotation[1] (yaw), whereas native 06071D10 writes camera-status +0x14, which is
rotation[2] (roll). Generated adapters now repair both shared calculations.
All authored camera records, durations, live target additions and the existing
script-completion barrier remain in control. This applies to every field using
the shared type-1 camera, not only the worm event.

Validation: field_camera_interpolation_test.py executes the production
initializer/update with the actual fixedPoint type. It checks 245 angle/duration
cases including positive/negative wrap, preserves yaw and pitch, tests live
target movement and the m5C script-completion barrier, and verifies the original
roll-load/subtract/step-store/camera-axis instructions against FLD_A5.PRG. Vita
compilation/link/package passes. These corrections have not been hardware
validated and are not established as the cause of the reported exit or glitches.

Additional callback trace: 06067F12/7F7C/7FAC recreate their respective worm and
tracker and re-enter the shared camera wrapper with a null camera-data argument.
They must not be described as proven ordinary script-completion or skip
callbacks on the evidence currently available. The shared camera stores their
EAs in m7C/m80; its normal end path does not invoke them. Their original dispatch
context remains unverified. No raw Saturn callback address is executed by Lagi.

## Hardware video review and Lathum gate correction - 21 November 2026

Reviewed the supplied 357.624-second 1280x720 recording in the 12-05-42 capture
folder and its 2,284,256-byte log. Extracted review sheets are retained in the
work-session outputs/2026-11-21-1205-review folder. The early Wires view
(approximately 37..49 seconds) shows through geometry. This diagnostic explicitly
uses CULL_NONE and submits source-quad perimeter lines; it has no filled surface
prepass to hide other edges. The normal textured path uses CW/CCW culling, with
existing ordered two-sided and known-cliff exceptions. Wires alone does not
establish that normal world/dynamic draws lack face culling. No global culling
change was made.

Around 340..354 seconds the video shows large, changing colored rectangles
covering sky/ground/cliffs while the dragon and radar remain recognizable.
That is distinct from geometry disappearing at a screen edge.

The user corrected the exit expectation: it becomes available only after
fighting Lathum, which is not implemented yet. The pre-Lathum exit is therefore
not accepted as a selection defect. Preserve its authored progression gate. The
bounded exit observations remain available for the future post-Lathum test;
there is no reason to force the exit available now.

## Wind sprite table/slot restoration - 21 November 2026

Native 06058800..06 masks the random index, scales it by four, and loads a
pointer from table 06099804. Its first four values are 060992B4, 06099394,
06099394 and 06099474. Upstream mode 0 instead passed 06099804/20/3C/58 directly
to initVdp1Quad as sprite records. The first bad record has isLast=6, PMOD=0609,
SIZE=0609 and width=06099474 (about 1,545.58 world units), whereas the intended
first record has width 0000C000 (0.75). This is a proven malformed wind sprite
producer consistent with the full-screen corruption. Hardware confirmation of
the corrected output is still required.

All wind modes now resolve the original pointer tables through a common helper.
Modes 0/3 use 06099804 and mode-1 secondary particles use 06099814. Three native
slot producers only replace their quad-list pointer (06058C76/58D1A/58DF6);
upstream particleInitSub(...,0,...) also reset the pool's character-memory base
and animation phase. The adapter restores pointer-only rebinding. Native
06058D06..0C writes 0x51 to worm-particle gravity at +0x24; the upstream source
wrote it to lifetime before immediately replacing lifetime. This is corrected
too, while retaining each mode's original producer/physics and pool.

Validation: a5_wind_particle_test.py runs the production quad decoder, pointer
resolver and wind emitters against original FLD_A5.PRG. It checks all four
mode-0/3 selections, both secondary lists, valid eight-frame data/dimensions,
preserved texture base and animation phase, native gravity, non-identity free
indices and full-pool rejection. Original instruction/literal checks cover the
indexed pointer load and all three pointer-only rebinds. Vita package passes.
The user's run being prepared at d21bb02 predates this wind correction and
remains useful for the horn warning, worm crossing sand and camera checks.

## Worm passage unlock/selection/entry restoration - 21 November 2026

Native Init (06056BCC/56BEC through literal 06056C78) assigns LCS callback
060569E8. It sets the actor's approach state to 2 and records the visit bit from
record +0x2A using the adjusted story-bit index. Upstream passed a null callback
in both normal-vector branches, so selecting a passage could not start entry.
The adapter restores this typed callback through the existing LCS context.

The original position at record +0 and normal at 06098AE4 are now decoded into
stable host vectors. Upstream cast the big-endian disc bytes to sVec3_FP, giving
LCS byte-swapped coordinates/normals on Vita. Init and activation checks also
remapped the byte index for bits >=1000 but kept the unadjusted bit mask. They
now use mainGameState.getBit566, matching the original adjusted byte/mask. The
draw-time check already remapped both parts correctly; it uses the same shared
accessor now. No unlock or progression bit is forced.

The debris producer now passes decoded world position and rotation to the shared
scatter effect, as 06056A5C..64 does (position record+0, rotation record+0xC).
Upstream used the rotation bytes as position and omitted rotation entirely.
The original 90-frame approach, dragon motion, visit bits, destination records
and shared triggerSubfieldChange/WORLD handoff remain in control. This is the
worm passage, not the separate Lathum-gated main exit.

Validation: a5_worm_passage_test.py executes production Init, unlock draw,
selection callback and Update with all seven original passage records. It
checks stable decoded coordinates/normals, locked gates and neighboring-bit
rejection, one-time unlock/debris, correct open model on reload, LCS registration,
only the authored visit-bit change, the 90-frame barrier and one-time destination
handoff. It uses the actual shared bit-accessor implementations and validates
the native callback/bit-write/debris-transform instructions. Vita package passes.
Live passage opening, selection, flight-in and arrival remain unverified.

## Acceptance from d21bb02 capture - 21 November 2026

The user confirms the red-worm cinematic now triggers. The supplied 12-28-58
video (85.611 seconds) still shows colored full-screen rectangles, and the user
confirms this run used d21bb02. It therefore does not test wind correction
8471dd2; keep that correction pending hardware acceptance. The log also records
shared first-encounter dialogue and second-encounter text 9/B_A5_03.PCM starting.
A start record alone does not prove complete voice/scene timing.

Ordinary deadline-outlier samples (not serialized GPU-pass samples) have median
render=30,863 us, CPU preparation=16,182 us, scene build=7,066 us, final GXM
finish wait=10,898 us, and presentation interval=50,050 us across 2,595 reported
outliers. These are selected slow frames, not a whole-run average. CPU
preparation includes scene build (render-start to pre-wait minus submission);
do not add the two times together. The wind-corruption samples reach about
3.24 million sprite bounding-box pixels, but frame pacing also remains slow
without that overdraw. Performance remains an open, separately measured task.
The user is testing the preserved 8471dd2 package next.

## Hardware acceptance at 5e252ba - 21 November 2026

User-confirmed on Vita:

- Red-worm cinematic triggers and plays.
- The worm passage opens.
- Secret Worm Passage entry and return to the field work.
- Combat entry/exit, player attacks, enemy death, experience and dyne work.

These supersede earlier pending checks for those paths. They do not establish
correct passage-entry camera animation/angle: the user reports that is wrong.
Enemies never attack. The field Lathum interaction is absent; source inspection
also finds BTL_A5_3's formation Init/Update/Draw unimplemented. The main exit
remains authored-gated by Lathum and must not be forced available. Renderer
performance problems persist. The user has not explicitly accepted every earlier
visual glitch, ground-crossing sand emission or full horn-warning scene timing.

Next source traces remain separate: native passage camera setup versus shared
follow-mode behavior; generic enemy-turn/attack dispatch versus formation-owned
attack producers; and the original field Lathum actor/encounter handoff. Keep
using original Saturn instruction/data traces and shared Valley runtime services.

## Shared fixed-position camera tracking - 21 November 2026

Passage entry's original 06056D64..7C stores the camera position from record
+0x2C and selects follow mode 0. Its update table at 0609E8E8 resolves to
0606A4F4. When the dragon has no camera-script task, native 0606A658..69A
computes dragon-position minus the fixed camera position, calls the shared
look-at calculation, writes pitch/yaw and updates distance. It keeps camera
position and roll unchanged while the dragon approaches the passage.

Upstream cameraFollowMode_scriptTarget retained the scripted/clamped branch
but left this no-script arm as assert(0). The generated fieldCamera adapter
restores that arm using the existing computeLookAt and vecDistance services.
It introduces no camera offsets, durations, alternate path or Garil-only camera
simulation. Other authored users of the same shared mode receive the same fix.

Validation: field_fixed_camera_test.py executes the production mode with its
actual adapter across 360 moving-dragon frames and init/blend/active states. It
checks fixed position, unchanged roll, exact camera-to-dragon delta, pitch/yaw
and distance service arguments, and retained near/clamped scripted behavior.
Original PRG checks verify passage mode 0, table target, no-script branch and
the native position/angle/distance operands. The user's report at 5e252ba
establishes that this camera was wrong before this repair; the corrected camera
still needs Vita confirmation. This camera change is committed independently
from enemy attacks, Lathum and renderer optimization.

## Exact shade-cache storage reuse (separate performance item)

The existing shade cache compares all 12 source corner-color float bit patterns
before producing nine midpoint shade triples. Static visibility rebuilds and
prepare_vdp1_model previously cleared those keys even when the same mapped
subdivision storage survived. The performance change retains keys for reused
storage; changed bits still rewrite, and freed/replaced storage still clears.
Position invalidation is unchanged. Polygon identity, geometry, UVs, camera and
material identity do not enter shade interpolation, so they need not invalidate
a physical slot whose complete corner-bit key matches. All subdivision-stream
writers were inspected: reused preparation writes UVs without overwriting shade;
new storage zeroes shade and invalidates its keys.

Validation: subdiv_shade_storage_test.py executes the production cache key,
midpoint and shade-write blocks and compares every shade output byte with
unconditional interpolation across 512 geometry/material rebinds, changed
colors, visibility/probe changes, signed zero, buffer replacement and slot
shrink/growth. It observes over 58,000 exact reuses. Vita compilation/link/package
passes. This establishes output equivalence for those cases, not a measured
Vita speedup or stable 30 Hz. Existing subdivShadeReused/subdivShadeWritten
counters remain available for the next matching performance capture.

## Enemy attack trace checkpoint

The original single/horned formation selector, wind-up, homing projectile and
collision/damage contracts are recorded in [GARIL_ENEMY_ATTACK_TRACE.md](GARIL_ENEMY_ATTACK_TRACE.md).
Captured Saturn attack ranges match the original BTL_A5.PRG byte for byte.
The live single update omits attack selection and states 5-8, and its wind-up
and projectile call sites remain stubs. This checkpoint establishes the native
implementation contract; it does not restore or hardware-verify enemy attacks.

## Hardware camera acceptance and remaining slowdown - 21 November 2026

The user confirms the corrected passage-entry camera and better overall
performance, with some slowdown remaining. This accepts the camera repair;
do not reopen it without new contrary evidence. Enemy attacks and Lathum remain
incomplete. The main exit remains authored-gated by Lathum.

New log: D:/Users/Noradninja/Downloads/lagi.log, 1,504,674 bytes,
SHA256 0244FE65C228778EC60FC5EA91D6BA54FE9DA11645D6528B518C4C4C17432A35.
The log does not identify an exact Git commit, so retain the user's confirmation
without inventing build provenance. No serialized GPU samples were found:
gpuSerialized=1 occurs zero times. Shade reuse is active: the final ScenePerf
sample records subdivShadeReused=741 and subdivShadeWritten=0.

There are 84 mode-7 renderer timing windows covering 10,080 frames. Their mean
renderer times range from 11,612 to 30,754 us (median window mean 21,210 us);
50 sampled renderer frames exceed 33,333 us. These are renderer-only counters,
not complete gameplay-frame or display-interval measurements. The 542 unfaded
mode-7 presentation outliers have median interval 50,049 us, renderer 30,504 us,
CPU preparation 16,885.5 us and GXM finish 9,381.5 us. They are selected slow
samples, so their median must not be presented as the entire run's frame rate.
Build time is nested inside CPU preparation; do not add it again. Remaining
slowdown is supported by both the user's observation and the display outliers.
No fresh performance code change or another capture is requested at this point.

## Single/horned enemy attack implementation checkpoint

The previously missing single-formation selector and attack phases now run
through the existing battle-engine camera/turn services, shared typed actor
flags and new reusable native charge/homing projectile services. Native
writable position layouts, spacing and layout cycling are restored alongside
the actor's original world-space firing records. See
[GARIL_ENEMY_ATTACK_TRACE.md](GARIL_ENEMY_ATTACK_TRACE.md) for contracts, scope
and validation. Host/generated-stage/native-PRG tests and full Vita packaging
pass; device attack/continuation acceptance is pending. First-Nanyd, dual,
variant and Lathum attack paths remain open. The user's accepted camera and
observed overall performance improvement remain in force; this attack commit
adds no renderer or Lathum change.

## Nanyd enemy turns and coordinated attack checkpoint

The Nanyd selector, native 18-point formation history and shared spring motion,
actor rushes and coordinated attack are connected through Lagi adapters and
existing battle-engine/damage services. Coordinated dragon evasion reuses the
working Valley helper. Shared typed actor flags, contact geometry and safe
meter retirement serve both area-specific and common paths. See
[GARIL_ENEMY_ATTACK_TRACE.md](GARIL_ENEMY_ATTACK_TRACE.md) for exact original
contracts and validation. Nanyd, single/horned, Valley projectile, reward-meter
and Scatter regressions pass, as does full Vita packaging. Hardware acceptance
of restored enemy attacks is still pending. Dual/variant attacks and Lathum
remain separate unfinished items; no renderer or story-flag change is included.

## Lathum field cell-object restoration - 22 November 2026

Original A5 subfield-0 setup passes both environment 0608E24C and object
table 0608E34C to 06078D26. The port dropped the object table. Its five
mound actors now use the existing Valley cell traversal, an overlay-keyed
factory binding and original model/LCS/animation/sand contracts. Native
action 06054DCC requests encounter 10 with completion bit 90 through the
existing shared battle service. The authored script and victory gate remain
intact. See [GARIL_LATHUM_TRACE.md](GARIL_LATHUM_TRACE.md).

Host/native-data checks and Vita packaging pass. This is a field actor and
handoff checkpoint, not a playable Lathum fight: BTL_A5_3 formation callbacks
remain unfinished. No new device confirmation is claimed or capture requested.
The previously accepted camera correction and performance improvement remain
accepted; remaining slowdown stays open.

## Lathum battle initialization - 22 November 2026

Lathum's native Init 060542B0/helper 060541AC now acquire slot-8 resources
and build both 21-bone rigs, 11 core targets and 13 targets across nine groups
through existing model/target/life-meter services. Health comes from COMMON
type 22; initial mode/group selection and formation-name operands are retained.
Typed host storage replaces the raw Saturn-size placeholder. Native-data/host
checks and Vita link/packaging pass. See [GARIL_LATHUM_TRACE.md](GARIL_LATHUM_TRACE.md).

Update/Draw and Lathum-specific damage/turn/part behavior remain unfinished.
This initialization checkpoint does not make the fight playable and adds no
hardware acceptance or bypass of authored completion/exit conditions.

## Lathum native drawing - 22 November 2026

The emergence/combat model draw switch, nine state-selected parts, two fixed
attachments and original live hotpoint/pose/camera bindings are restored.
They reuse Valley's hotpoint flattening and the existing renderer. Original
PRG contracts, initialization regressions and all 19,683 grouped-part state
combinations pass host checks; full Vita packaging passes. See
[GARIL_LATHUM_TRACE.md](GARIL_LATHUM_TRACE.md). No final-pixel or gameplay
acceptance is claimed. Lathum's Update controller and complete fight/return
remain unfinished; no new hardware capture is requested at this checkpoint.
