# Garil Lathum field placement and battle handoff

## Original data and missing integration

Disc 1 `FLD_A5.PRG` SHA256:
`3743201c4b369da9bd88377c315711be5fd18246ba6ac3d1d656048c026efeb4`.
The native instructions were read with the existing Saturn disassembler, using
base `06054000`. This is a source/data trace, not a new emulator playthrough or
Vita acceptance.

Original `060543B6..060543C0` passes all three setup operands:

| Operand | Original value |
| --- | --- |
| Environment table | `0608E24C` |
| Cell-object table | `0608E34C` |
| Start-tasks callback | `0605A6C8` |
| Setup service | `06078D26` |

`06078D26` first sets up the environment and start tasks, then traverses the
cell-object table. Lagi previously called only `setupField2`, dropping the
second operand and every placed actor. The existing Valley
`setupFieldWithCellObjects` already implements that traversal and lifetime.

The original object table has area 2, a 2x2 grid and five records in cell 1.
Every record calls factory `06055E1E`. Their static hierarchy keys are
`1EC, 1EC, 1E8, 1E8, 1F0`. The last variant alone enables interaction; the
other four mounds are decorative. Placement positions, signed 16-bit rotations,
resource pairs and cell parenting are read from these original records.
Subfield 7's native setup explicitly passes no object table; it must not be
populated by copying the subfield-0 list.

## Actor, target and sand burst

Task descriptor `0608D72C` selects Init `06055AE0`, Update `06055D34` and Draw
`06055D9C`. The translated host actor uses named members rather than Saturn byte
offsets. It registers the existing shared field model/LCS context at placement
Y + `10000`; only hierarchy `1F0` clears the disabled-target flag.

The random initial delay is modulo 300. On expiry, the original screen test
uses maximum depth `100000`; a rejected attempt retries after 240 + modulo 600.
An accepted attempt runs three 39-frame animation cycles. Each cycle emits its
sand task when the old frame counter is 28. Animation tables `0608D474` select
39 original hierarchy/pose pairs per variant. Collision keys/poses are
`1F4/728`, `1F8/72C` and `1FC/730`; they use the working shared hierarchy collision
producer, not an extra visible model submission.

The sand child descriptor `0608D73C` selects Init `06055F6C`, Update `06056080`
and Draw `060560DA`. It owns 32 particles and seven replacement attempts. The
original reset/step order, random masks, lifetime, gravity `-2C`, scale factor
`111EB`, frame tables `0608D480`/`0608D5D0` and signed sorting bias `-6000` are
retained. It reuses the existing quad animation, scaled/sorted particle draw
service and A5 wind integrator. The origin is copied at spawn, as in the original.

## Authored battle request

Target callback `06055D50` requests field script 0 with flag `5B5`. If that
script cannot start, it calls `06054DCC` immediately. Script 0 also reaches
`06054DCC` after its authored dialogue. That action passes encounter `10` and
completion bit `90` to `060703E4`, the same service contract implemented by the
working Valley `startBaldorQueenBattle` service. There is no forced victory bit,
replacement encounter or shortened dialogue. Original `06055BF6` retires the
mounds when `bitField[12] & 80` is set, the authored completion bit.

The existing battle-manager remap sends A5 encounter `10` to COMMON overlay
index `12`, subbattle 0. Original `COMMON.DAT` names that index `BTL_A5_3.PRG`.
The battle loader already recognizes this overlay; its Lathum formation
Initialization and drawing are restored below; the Update controller is unfinished.
**This field checkpoint does not make the Lathum battle playable.** Do not use
interacting with the mound as a successful combat acceptance test yet.

## Shared service boundary

Lagi's native cell factory registry keys a binding by both the source overlay
object and native function address. This avoids collisions between overlays
that reuse load addresses. The shared cell traversal tries a registered factory
first, then retains its existing overlay dispatcher. Field re-entry replaces
the same registration. Garil supplies only its unique mound factory and its
original table operand; it does not duplicate Valley's grid or battle loader.
Generated-source adapters are guarded at their expected boundaries. No file in
`extern/Azel` is edited.

## Validation and remaining gates

`tests/garil_mound_test.py <FLD_A5.PRG>` verifies native setup/descriptor/action
operands, original five placements, COMMON's overlay identity, the generated
integration boundaries, static/transient model selection, one interactive
variant, script-start fallback, three burst cycles, particle retirement,
overlay-owner isolation and field re-entry. It compiles and runs the production
mound/registry source with host service mocks and also runs the existing Garil
native-action regression fixture. It does not execute a full Saturn battle.

Host/native-data checks and full Vita compilation/link/VELF/SELF/VPK packaging
pass. The older raw-disc audit was not rerun because its previous local CUE/BIN
path is unavailable; the extracted original PRG and COMMON data were used for
this checkpoint. Device visual/interaction acceptance and the entire Lathum
formation/fight/victory/exit sequence remain pending. Earlier hardware-confirmed
passage, camera, rewards and combat entry/return acceptance remains in force.
Remaining slowdown and unaccepted dual/variant enemy attacks remain separate.

## Typed battle-rig initialization checkpoint

Original BTL_A5_3.PRG SHA256:
`cd4b77b2fea5d10cc1cd1eb81351f3e14b53ae213f1edc1b044598b1347035dc`.
Factory `060565DA` selects task descriptor `060A4008` and original work-area
size `18C`. Init `060542B0` and helper `060541AC` now use typed host storage,
with a concrete disposer for their owned vectors. The Saturn size is not used
as a C++ allocation size. The existing model, target and life-meter services
handle their usual work; no separate damage engine is introduced.

| Native binding | Restored value |
| --- | --- |
| NPC slot / bundle | 8, ZUSU.MCB/CGB |
| Actor descriptor / local position | 060A3EB4 / (0,0,-64000) at 060A3EC8 |
| Combat hierarchy / pose / animation | 4 / 144 / 150 |
| Emergence hierarchy / pose / animation | C / 14C / 158 |
| Bones / animation frames | Both hierarchies 21; both animations 149 |
| Core targets | 11 hotpoints, flags F0000001, radius 1000 |
| Group sizes at 060A3EFB | 1,2,1,2,1,2,1,1,2 (13 additional targets) |
| Group target flags / radius | F0004000 first, F0004002 additional / A000 |
| Life meter | Stable world-position/health addresses; COMMON enemy type 22 |
| Initial selection / counter | One RNG value modulo 9 / signed -1 |
| Initial mode / model phase | 4 / 0 |
| Formation-name operands | (0,1,13) |

Descriptor byte 16 is enemy type 22, not a bone count. Group positions remain
independent and stable; the original null-owner target contract is retained.
The future Lathum controller consumes their damage explicitly. Groups are not
flattened into regular-enemy targets.

`tests/lathum_rig_test.py <BTL_A5_3.PRG>` compiles the production initializer,
shared model creator and Valley core-target producer against the real PRG,
ZUSU.MCB and COMMON.DAT data. It checks 18 selector inputs, every resource and
target binding above, the initial state, failed resource/model creation and
the generated integration. Native instruction/table assertions and host checks
pass, as do Vita compilation and full link/packaging. This fixture does not
simulate a battle or prove rendering/device behavior.

Update `06055EF8`, mode-specific controllers and damage helper `06055C3C` remain
unfinished. The Update stub remains explicit; drawing is restored below. No
idle fallback, replacement boss or forced completion bit is installed. No new
Vita boss test is requested yet.

## Native draw and live-hotpoint submissions

Draw `06056530` and grouped-part helper `06056202` are now connected. Phase 0
draws the emergence model at the live world position with Y rotation 8000000;
it does not consult the normal-body hide byte. Other phases honor native byte
18A, draw the combat model, and use the existing Valley `urchinUpdateSub3`
service to flatten its live view-space hotpoints. This is the original
bone/point order, not a generated list of approximate attachment locations.

The nine groups use core anchors `8,3,9,10,6,0,4,5,7` (060A3ED4), pose bones
`15,4,16,19,9,2,5,8,12` (060A3FFE), and the three-state mesh rows at 060A3FC8.
The two fixed attachments use core anchors 1/2, pose bone 3 and meshes 94/BC.
Each view-space anchor is transformed through the current camera's inverse
matrix, followed by the native Y rotation and the live bone's ZYX rotation.
The native shared rotation helper 06035D4C reads Z/Y/X in that order. Group
world target positions are owned by the controller and are not overwritten by
Draw. All mesh submissions use the existing bundle/model rendering service.

The host drawing fixture checks all 19,683 combinations of the nine three-state
parts, live pose/anchor selection, camera-transform identity, phase/hide guards,
body translation and balanced matrix-stack submissions. It compiles the actual
draw producer and Valley hotpoint-flattening function. The shared rig fixture
also continues to pass against original PRG/ZUSU/COMMON data. Full Vita
compilation/link/packaging passes. These checks do not establish final pixels,
performance or a working Lathum battle: its Update controller, emergence camera,
attack/damage/death behavior and authored victory/field return are still open.


## Damage-service trace and shared zero-value correction - 22 November 2026

Native damage helper 06055C3C uses shared services rather than a separate boss
combat implementation: 0605A61E is createHitSparkEffect, 0605A5B4 is
createDamageDisplayTask, 0605B896 returns the targetable hit position, and
0607068C is CreateDamageSpriteForCurrentBattleOverlay. The reaction helper
0606960A maps the authored group quadrant to a bit and calls 06059984,
which sets the low safety nibble (battleEngine_FlagQuadrantBitForSafety).
These identities are traced in BTL_A5_3.PRG itself; addresses in a RAM capture
from another overlay are not interchangeable.

A shared translation error was found before connecting Lathum damage: the
upstream createHitSparkEffect zero branch sets both color and damage to 3.
Original A5_3 0605A658..666 and Valley A3 0605BB10..1E retain damage zero and
write only color 3. The generated shared adapter now removes the damage-value
assignment, preserving upstream files and the existing formatter/rendering.
This corrects zero-hit display input across all users of the shared service;
it does not change health subtraction or manufacture a damaging hit.

battle_hit_display_test.py checks both original instruction sequences and
compiles the generated producer. It verifies damage values -32767..32767,
all four impact types and hitbox strengths 0/10/11/255, live position, formatter
mode, and allocation failure. Signed-minimum negation is outside this fixture's
coverage. Full Vita packaging passes. Device appearance is not yet verified.

Lathum's controller and damage-consumption loop remain unfinished. That loop
must preserve previous-inverse-camera target refresh, 16-bit accumulation,
per-group reactions and original display/death flush ordering. The generic
model-damage loop has different ordering and cannot replace it wholesale.


## Native grouped damage helper - 22 November 2026

06055C3C..06055EDE is now translated as lagiUpdateLathumDamage in the
Lagi-owned generated adapter. Typed rig storage adds the native pending-damage
word, pending byte, selected-group hit counter and controller substate.
The helper reuses the existing hit display, overlay damage sprite, audio,
quadrant safety and aggregate display services identified above.

It refreshes all nine target positions from prior view-space core hotpoints,
using the previous inverse camera before the current view transform. Previously
reacting groups lower the world anchor by A000. Hit processing preserves the
native order: display, signed-word damage/health accumulation, life-meter flag,
sprite, sound, target-flag clearing, group reaction and safety quadrant. The
first selected-group hit requests mode 5; repeated hits preserve the counter's
byte wrap. State 0/1/2 sets target strength 1/3/20. Processing continues across
all groups even if an earlier hit exhausts health. The final death branch
requests mode B/substate 0 and retains accumulation; the live engine-1000
flush clears both pending fields. Healing and zero damage retain native gates.

The helper is compiled and fixture-tested but is deliberately not called by
Update yet: the native phase dispatch and emergence/combat/death controllers
are still unfinished. No partial replacement Update or idle fallback is
installed. This checkpoint does not make the Lathum battle playable.

The real-PRG rig suite now compiles the production damage helper and checks all
nine selected groups and anchors/quadrants, previous/current camera identity,
repeat-hit transitions and byte wrapping, preserved unrelated target flags,
16-bit health/accumulator wrapping, healing/zero hits, death after multiple
hits, dead-entry early return, debug mode-4 guard and live display flush.
Original PRG operands independently anchor service calls, flags, effect scale,
previous-camera selection and death mode. Initialization/drawing regressions,
Vita compilation, full linking and packaging pass. Hardware remains untested.
