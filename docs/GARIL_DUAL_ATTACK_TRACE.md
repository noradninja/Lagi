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
