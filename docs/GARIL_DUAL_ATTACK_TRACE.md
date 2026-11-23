# Garil dual-formation trace - 22 November 2026

Baseline inspected: 73788c6. This encounter is not restored or device-accepted.
Keep the packaged variant/Lathum checkpoint independent of this investigation.

## Constructor mismatches confirmed in original BTL_A5.PRG

Native constructor 0605F10C allocates resource slots 2 and C. It writes COMMON
record IDs 15 and 17 to group offsets EC and 1DC respectively (0605F148-156).
The port omits both assignments, so actor health/reward identity cannot be
assumed correct even after existing common services are restored.

The first group contains ten swarm actors. Its factory call at 0605F258 uses
0605E220 with parent formation, pointer to the group at E0, entity index and
lead=0. The second group contains three lead single actors. Its call at
0605F2E8 uses 0605BBD0 with parent formation, pointer to the separate group at
1D0, entity index and lead=1. The port incorrectly calls BTL_A5_createEnemy
(the swarm factory) for both groups. The already restored single-enemy factory
is static in a different translation unit and currently receives an implicit
parent-owned group; supporting this encounter requires a real shared group
binding, not a cast that pretends the dual formation is the single formation.

Both native groups bind a parent position block: E4 points to formation+2C,
while 1D4 points to a second block at 1FC. Native initialization gives the
second block a separate positional/rotation state (0605F280-0605F2B2). The
port leaves both m4_positionBlock fields unset and does not model that second
block. Its secondary actors cannot simply share the first group's identity,
position table, counters, targetable callbacks or finished-state bookkeeping.

Native first-group position table is B11C8 + uint8(variant*0x78), with mapping
B13A8 and an 18-entry copied position trail. Second table is B13B4 plus signed
byte(variant*0x24). Native initial offset FF FED000 is used in the second
position block. The port selects B11C8 unconditionally and does not initialize
the trail or second block. Full actor and controller use must be traced before
assigning host field meanings or changing those offsets.

## Preserve the unusual RNG decision

At 0605F120-0605F138 the original consumes one RNG result, masks it with 1,
compares the result with 7 and 8, and normally selects variant 1. The port's
apparently impossible `(r & 1) != 7` follows that native sequence. Do not change
it to a guessed unbiased random choice because it looks suspicious. Original
instruction bytes C901 / 8807 / 8808 establish this independently.

## Next implementation boundary

Refactor actor creation around explicit typed group/formation-motion context so
both existing actor families can be reused without duplicating their already
restored attacks, damage, target retirement or rewards. Keep the original
parent task/resource ownership. Then restore the dual-specific layout/turn
controller from 0605F3A4. Current state 0 only checks player-turn status and
never selects an enemy attack; native phases remain to be traced. This source
audit does not justify a new gameplay acceptance request or a dual build claim.

## Explicit single-actor group binding checkpoint

Single actors now carry LagiGarilSingleGroupBinding: a typed group pointer,
separate active/reaction flag addresses, dead-word storage and a live target
position callback/context. Position interpolation, return-distance checks,
reaction bookkeeping and death count/entry writes use that binding instead of
dereferencing the parent as a specific single-formation work area. The current
single formation supplies its existing fields and writable layout callback,
so its behavior and ownership remain unchanged. This is preparation for the
native separate parent/group factory contract, not a completed dual factory.

The binding references live layout state; it does not snapshot a table that
would stop following authored formation switches. The production motion check
runs two independent groups/centers/layouts for 24,000 actor ticks and verifies
separate flags, dead storage and counters. Existing single constructor,
selection/controller and charge/fire regressions pass against original PRG
contracts. Vita compile/link/VPK packaging passes. The dual encounter still
uses its old incorrect factories until explicit-context factory export and
native dual layout/turn initialization are connected in subsequent work.

## Shared single-actor factory export checkpoint

The native single factory is now exported as lagiCreateGarilSingleActor(parent,
binding,index,isLead). Its task parent is independent of the bound group: it
loads COMMON identity from binding.group and initializes its live position via
the binding callback. No formation-specific parent dereference or cast remains
inside the shared factory. Existing BTL_A5_createEnemySingle is a wrapper that
supplies the original single formation's group, flags, dead entries and live
layout callback. Model initialization, attacks, damage and lifecycle remain
the same production implementations rather than a second dual-only copy.

`tests/garil_single_factory_test.py` compiles the actual generated public factory
and wrapper. It verifies unrelated parent ownership, independent group IDs and
flag/dead bindings, ordinary/lead variants, positions, baseline wrapper and
live layout switches, plus task allocation failure. The two-group motion test
and existing single initialization, controller and charge/fire regressions pass.
Full Vita compile/link/VPK packaging passes. The dual caller still requires
its native second motion block, flags/dead storage, correct constructor tables
and swarm binding before this export can be used there. No dual gameplay or
hardware acceptance is claimed at this prerequisite checkpoint.

## Dual group/factory wiring checkpoint

The generated dual constructor now initializes COMMON group records 15/17,
binds the first group to the original position block and the second to a named
secondary block, and creates the three second-group lead actors through the
exported single-enemy factory. Their binding uses native secondary active byte
1E0, reaction byte 1E2 and dead words at 1E4, plus a live callback to the
secondary group's current position table. It does not create swarm actors in
that group or reinterpret the dual work area as a single formation.

Original 0605F280-0605F2B2 first initializes the secondary block Y to -13000;
factory 0605F338-0605F35A then overrides all primary block Y values to +13000
and secondary block Y values to zero, setting both rates to A3D. This later
factory override was verified before implementing the host state. Both blocks
now update through shared formationUpdatePosition, matching calls at F474/F486.
The unusual native RNG decision is retained.

The production group fixture checks identities, distinct parent blocks,
constructor/factory overrides, exact single-factory bindings, all three indices,
live table changes and creation failure. It checks final native motion literals
and generated initialization/update order. Shared single factory/group isolation
regressions and full Vita packaging pass. This remains partial dual coverage:
swarm layout/trail initialization and bindings, dual-specific turns/attacks,
completion and full encounter/hardware acceptance remain open. Do not treat
this constructor checkpoint as a playable dual battle.

## Dual swarm layout/trail checkpoint

The dual constructor now selects swarm position table B11C8 + uint8(variant*78)
and mapping B13A8 before creating actors. Native initialization F18E-F1D8 seeds
trail slots 0 through 16 with the formation head; slot 17 is not written by
that loop. The generated adapter preserves this distinction. Native update
FBF4 writes head slot 0, then reverse-copies slots 16..0 into 17..1, leaving
slots 0 and 1 equal to the new head. It now reuses stepNativeFormationTrail
after the two shared formation position updates. No alternate motion engine
or guessed positional offset is introduced.

The production fixture checks both table choices, the exact ten-actor mapping
{0,0,3,3,6,6,9,9,12,12}, untouched initial slot 17, and 24,000 trail updates
against an independent expected history with a storage-boundary sentinel.
Original seed/shift loop operands and generated order are checked. Dual group
regressions and full Vita packaging pass. The special native RNG decision is
unchanged. Dual turn selection/attack phases, mixed-actor coordination and full
encounter/device acceptance remain open; this is a source prerequisite.

## Dual swarm release/completion phases checkpoint

Original controller states 2-5 are now generated through shared actor signaling.
State 2 waits for engine 2000, displays attack name 1 for 30 frames, transitions
only readied primary-group actors from 04000000 to 02000000 and enters 3.
State 3 compares old signed counter >15, sets attack-finished and joins 1.
State 4 similarly displays name 2 for 35 frames, releases primary actors and
enters 5. State 5 compares old counter >60, restores the variant's native swarm
layout B11C8 + uint8(variant*78), sets attack-finished and joins 1. All other
actor flags remain intact; null/finished actors use the shared task filter.

The production phase fixture checks readiness, names/durations, all combinations
of selected/unselected flags and null/live/finished actors, and every signed
16-bit timer value for both completion phases and both table choices. Native
release-helper EA, timing/name operands and table base are checked. Dual trail
and group regressions and Vita packaging pass. These phases are not yet reached
from native state-0 selection: selector FC2A, secondary attack phases and their
camera/mixed-group coordination remain to be restored. No dual gameplay or
hardware acceptance follows from this partial controller checkpoint.

## Dual secondary attack phases checkpoint

Native states 6-9 are now connected to the existing single-actor flag accessor,
shared release and camera services. State 6 waits for engine 2000 and displays
name 8/duration 35. State 7 tests old signed counter >45, copies dragon mF0
to the formation camera target, snapshots the primary head as eye plus
{A000,-1000,-14000}, sets phase 1, enables attack camera, binds desired/current
pointers, resets interpolation and releases readied SECONDARY-group actors.
The Y operand is native signed word F000 (-1000), not positive F000.

State 8 tests old counter >27, sets attack-finished and restores the default
camera without interpolation reset. State 9 waits until player turn is inactive,
clears turn-ready/ownership and phase, then calls the interpolating after-attack
camera restoration. Native helper A289E forwards to 818D0; it is not a no-op.
The typed single flag accessor is exported, so the dual controller does not
cast an opaque actor to a guessed work-area layout.

Production generated phase fixtures check camera call order/identity, exact
snapshot values including integer wrap and the signed Y offset, release masks,
every signed timer value and player-turn handoff. Original helper EAs, offsets,
name/duration and timer operands are checked. Swarm-phase and baseline single
controller regressions pass; full Vita packaging passes. Native state-0
selection and per-frame camera follow/layout coordination remain pending;
these phases alone do not establish a complete dual encounter on hardware.

## Native dual quadrant selector connected

State 0 now requires no active player turn, engine turn-ready exactly 1 and
secondary byte 1E3 clear, then calls the translated FC2A selector. Quadrant 0
checks dragon status bit 1 and secondary completion; it scans secondary dead
words and live/nonbusy actors, readies live nondead actors with 0C000000, sets
native mode 9 (not 7), marks quadrant attack bit 1 and enters state 6. With no
eligible secondary actor it releases turn ownership without inventing a move.
Native status/completion fallback consumes one RNG bit for direction 1/2.

Side quadrants increment native signed-byte counter +278. Old values >=2 reset
it and move in quadrant-specific direction; other values select the first
three live swarm actors, clearing coordinated tag 08000000 on every live actor
and setting ready 04000000 on the selected three. Successful rush selection uses
mode 7 and attack mask 2/8, entering state 2. Quadrant 2 readies all live swarm
actors with 0C000000, selects B12B8 + uint8(variant*78), invokes the existing
Valley evade service with duration 60, marks attack bit 4 and enters state 4.
Missing groups/actors fall back to the original one-bit RNG move and release.

Production selector fixtures exhaust swarm membership masks for side cycles
and coordinated layouts, plus secondary live/dead/busy combinations, status
and completion fallbacks, exact RNG consumption and mode 9. Original field,
selection count, table, mode and helper operands are checked. Swarm/secondary
phase regressions and full Vita packaging pass. Per-frame camera/layout
coordination, mixed-actor integration and hardware acceptance remain open.

## Native per-frame dual camera update checkpoint

The incomplete eye-only scroll substitution is replaced by original F48E-F590.
After both formation position updates and the shared trail copy, every tick
scrolls camera eye A8 by the battle auto-scroll vector. With phase 280 equal to
zero, target C0 is refreshed from the live primary head and target Z subtracts
12000. With ANY nonzero phase, the saved target is instead scrolled by the
same delta; it does not snap back to the current dragon or formation head.
Native 32-bit arithmetic wraps independently per axis. State-7 snapshot setup
and state-9 phase reset therefore feed the same per-frame rule as the original.

Production camera tests compare 60,000 updates against independent unsigned
arithmetic, changing head/scroll values and phase values 0/1/-1/2/INT_MAX.
Original phase/vector operands and generated trail-camera-state order are
checked. Selector and secondary phase regressions and full Vita packaging pass.
Remaining native layout helper F922 and mixed actor/formation integration,
results/return and hardware acceptance still require work. No visual parity
or whole-encounter acceptance is inferred from this camera recurrence test.

## Shared single/dual spacing service connected

Native F922 is ordered-pair secondary layout spacing, not a height/turn helper.
It matches the existing single formation's B558 algorithm. Both now call
stepNativeBattleSpacing: filter dead/nonlive actors, mutate the OTHER position
immediately per ordered pair, self-jitter only actors 0/1, separate close/random/
far-distance branches, exact RNG order and X/Z clamps. Common RAM 06035A5C
confirms the distance helper includes all three axes even though spacing only
changes X/Z. Signed jitter and wrap additions are explicit.

The dual secondary layout is copied from its authored table into task-owned
three-position storage before actor creation. The shared actor binding reads
that live mutable storage, so spacing affects movement without modifying
original overlay asset bytes. Spacing runs before both formation interpolation
calls, matching native Update. Table metadata remains authored; the constructor
copies the selected table exactly once into its writable encounter state.

The service fixture compares 24,000 three/six-actor cases with an independent
ordered-pair implementation, exact RNG state/count and liveness/bounds. Existing
single-controller spacing regression cases, dual group/layout ownership checks
and selector checks pass. Native constants and call order are checked; full
Vita packaging passes. Mixed actor/formation integration, battle completion,
return and hardware acceptance remain open. This checkpoint shares common
behavior rather than adding a dual-only spacing implementation.

## Native defeat-count completion restored

BTL_A5 Update 0605F3C8..0605F460 calls shared cleanup 060A2C64
and shared defeat predicate 060A2C50 independently for both groups. The
predicate is signed-byte deadCount >= entityCount; it does not inspect actor
pointers. The translated dual controller incorrectly added an all-null actor
requirement. The generated adapter now uses formationCleanupEntityArray and
formationAreAllEntitiesDead directly, preserving quadrant call order and the
state-10/turn-field release when both defeat flags equal one. Death animation
retirement therefore cannot add a completion gate absent from the original.

The production adapter and actual shared helper bodies pass 32,768 combinations
of thirteen actor retirement bits and two independent defeat counts. Tests
check pointer cleanup, quadrant call ordering, completion and all four turn
fields; original predicate bytes and generated update order are checked.
Selector, both attack-phase regressions and full Vita packaging pass. Full
encounter integration, rewards/return and device acceptance remain open.

Resource audit: allocateNPC tracks each slot independently; model initialization
stores the explicitly supplied bundle in m4_pModelFile and model draw retrieves
its hierarchy from that bundle. No speculative parent allocation rewrite was
made. This source audit does not establish texture or visual acceptance.
