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
