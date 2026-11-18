# Garil shared runtime parity

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
