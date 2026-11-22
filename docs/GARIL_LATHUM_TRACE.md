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


## Emergence vector-task service - 22 November 2026

The original 060540E2 factory and 06054078 child Update are restored as the
shared lagiCreateNativeVectorTransition service. It samples an endpoint at
creation, computes FP_Div(10000, signed-duration << 16) separately per axis,
then MTH_Mul with that axis's wrapped endpoint-minus-live delta. Each child
update adds its fixed velocity to the parent-owned live vector, decrements
the signed 16-bit duration and retires when it is nonpositive. It never snaps
to the endpoint. This is deliberately distinct from the existing spring and
ordinary positional interpolation services.

Emergence substate 1 requests 180 frames; substate 2 requests 270. A 180-frame
move from 1000 to 100000 produces reciprocal 364, velocity 549 and final 99820,
not 100000. Negative displacement uses the native signed-product low bits.
The shared service preserves these rounding/remainder and word-wrap effects,
with typed child-task storage and a parent-owned live vector. Allocation
failure returns null before touching either vector.

The original emergence state 0 waits for the engine's intro task, retires it,
resets camera state and supplies live eye/target pointers. Its initial eye is
battleCenter+(1E000,11000,FFF55ED2), target is
battleCenter+(FFF94000,FFFAE000,7C000); these are traced operands, not proposed
replacement camera offsets. It starts a 90-frame wait before the first vector
transition. The camera helpers, particles and remaining state transitions must
still be connected before Update can replace the stub.

native_vector_transition_test.py checks original creation/update/duration
instructions and compiles the production task. Golden cases cover authored
180/270-frame motions, reciprocal remainder, sampled endpoint/live ownership,
independent concurrent children, completion timing, signed word duration and
32-bit coordinate wrap, and allocation failure. Vita compilation, linking and
packaging pass. This service is not yet called by Lathum Update; emergence and
the battle remain unfinished, with no new device acceptance claimed.


## Seven-substate emergence controller - 22 November 2026

Native mode 4 06054D34..060554CC is translated as lagiUpdateLathumEmergence.
The rig adds typed live eye/target vectors, a signed controller timer and
camera/dialogue child pointers. Every call first adds battle auto-scroll to
both camera vectors and retires completed child references. The native debug
shortcut sets normal model phase/substate 6 but still waits for dialogue.

State0 waits for engine intro task 3D0, clears its delete callback and retires
it. It uses shared camera restore/preset, snapshots desired eye/target into
reference vectors, zeros their deltas, requests battle mode 7, enables attack
camera, snapshots/zeros again and binds the live camera pointers. The traced
initial offsets and 90-frame wait are retained. State1 waits until its signed
counter reaches zero, then starts the 180-frame target transition. State2 waits
for that child, sets the second eye position, resets interpolation and starts
a 270-frame eye transition. It plays sound 6D and emits 100 particles using
original descriptor 060A3C60 and the authored resource slot 13, with 700 RNG calls
in native axis/position/lifetime order. It then selects normal model phase 1
and starts a 30-frame timer. State3 waits until the timer becomes negative
(the zero frame still waits), then starts dialogue table 060A3F04, entries 4/5.

State4 waits for BOTH camera and dialogue, starts independent 270-frame eye
and target transitions and the shared dialogue table 060A3F28, entries 6/7/8/9.
State5 waits for the tracked eye child before setting eye to dragon position
and target to battle center. State6 waits for dialogue, sets engine flag 100
(attack-animation finished), restores the default camera and selects mode 0.
The untracked target child and asymmetric state 5/6 waits match native code.
No timing, offsets, story flags or substitute lines were invented.

The shared content-keyed particle descriptor cache now has a public accessor;
particle children keep owned descriptor storage rather than pointers into the
boss rig. Dialogue uses the existing sFormationIntroEffectTask typed factory
with its original zero flag and the supplied native table, avoiding a second
speech implementation. Common camera/vector/particle/audio services are reused.

The production emergence fixture executes all seven states and verifies live
camera pointers, reference snapshots/delta clearing, exact offsets, 90/180/270
frame contracts, 30-to-negative timing, both completion gates, phase/engine
flags, auto-scroll ordering, all 100 particle payloads and 700 RNG calls, and
the debug dialogue guard. Native PRG contracts independently anchor helper
calls, effect descriptor/scale/count and dialogue table identities. Rig/init,
19,683 drawing states, damage and shared projectile regressions pass. Full
Vita compilation/link/packaging passes.

Overall Update is still explicit Unimplemented: emergence is compiled and
tested but not dispatched until the remaining mode controllers and shared
animation/effect update order are ready. Combat/death/victory/return and device
acceptance remain open. No new boss hardware run is requested yet.


## Selected-group hit reaction and shared single speech - 22 November 2026

Mode 5 (060554CE..06055604) is translated as lagiUpdateLathumReaction.
It adds auto-scroll to the target vector on every tick. Substate 0 waits for
battleEngine_isBattleIntroFinished, requests battle mode 2, binds the live
dragon position as camera eye, binds the actor target vector, enables attack
camera and transforms the selected group's core anchor through the previous
inverse camera. Substate 1 waits for engine flag 2000, then requests string
10, duration 0, filename 060A3F64 through the original single-line speech
factory. Substate 2 waits for the dialogue child; substate 3 sets engine flag
100 and returns to mode/substate 0. It does not install a guessed camera
restore or skip the separate dialogue-finished tick.

This exposed an omitted shared native facility. Factory 06068B66 builds one
12-byte entry with continuation -1 and calls 06068AF8 with flag -1. Negative
flag selects Update 06068B84, which shares voice/text states 0/1 with chained
speech but finishes directly at state 2. The existing shared C++ factory
ignored its flag; using its normal chain Update on a temporary entry could
read nonexistent next-table data. The Lagi-owned generated adapter now offers
lagiCreateNativeBattleSpeech with copied named parameters and a single-line
Update wrapper. It reuses the existing voice/text body, setup, skip handling
and delete callback; no borrowed temporary table or duplicated voice system
is introduced. The ordinary chained Update retains its original behavior.

battle_single_speech_test.py compiles the actual generated shared Update body
and production factory/wrappers. It verifies voice completion, signed timer
post-decrement, skip completion, no next-table access, parameter ownership,
allocation failure and retained chaining. Native PRG contracts anchor the
negative-flag branch, selected Update, single-entry continuation and Lathum
factory operands. The Lathum suite also executes all nine selected-group
reaction anchors, camera pointer/matrix selection, auto-scroll, readiness and
speech-completion gates, and the final engine completion flag. Existing
init/draw/damage/emergence checks and full Vita packaging pass.

These helpers are not yet dispatched by overall Update. Turn selection,
attacks, death and authored victory/field return remain unfinished. Initial
turn-selection inspection also found a native word at 06050224+6 selecting
attack-name 3 versus 11 when the group counter expires; its host mapping is
not established, so no constant or invented progression flag is substituted.
No new hardware acceptance or boss test is claimed at this checkpoint.


## Native turn selection and resolved port 2 selector - 22 November 2026

Mode 0 060543F4..060544E4 is translated as lagiSelectLathumTurn. It waits for
engine 3CC.m8 and for battleEngine_isPlayerTurnActive to be false, clears the
native ready word/counter, then requests mode 7. A positive signed groupCounter
is decremented; if it reaches zero, attack-name 3/11 is displayed for 30 frames
and mode 3 is selected. Otherwise the first reacting group in the current
dragon quadrant selects attack-name 2/mode 2; if none is reacting there,
attack-name 1/mode 1 is selected. Each selected mode resets substate 0. Waiting
ticks leave the ready state, counter and substate unchanged.

The previously unresolved selector at 06050224+6 is the held-button word of
controller port 2, mask 4. The original A3 item-menu routine 060742A8 loads
060501B8 at literal 06074318 and reads +8 for the source-mapped primary-port
new-button word. One native input-device record is 6C bytes; the second port
is 060501B8+6C=06050224. The generated controller therefore reads the named
port 2 current held-button field, preserving attack-name 11 when bit 4 is held
and 3 otherwise. Primary-port input is not substituted and no story flags are
read or written for this branch.

The production turn fixture checks all 19,683 combinations of three states for
nine groups, all four quadrants, six signed counter classes and both selector
values: 944,784 cases. It also checks readiness/player-turn guards, exact
ready-state clearing, mode-before-name ordering and opposite primary-port
button values to prove they do not control the variant. Native A5 operands
and the independent A3 input literal anchor the mapping. Existing Lathum
init/draw/damage/emergence/reaction checks and Vita packaging pass.

Overall Update is still not dispatched; attacks/death and authored victory/
field return remain unfinished. The next original attack mode 1 calls native
producer 06056840 for intact groups facing the dragon. That producer has no
upstream translation and must be recovered before the controller can run.
No boss hardware run or new device acceptance is claimed at this checkpoint.


## First attack and timed five-sprite producer - 22 November 2026

The native mode 1 controller 06054504..060547B4 and its missing producer
06056840 are translated. Every controller tick applies auto-scroll to both
camera vectors. Substate 0 waits for engine flag 2000, binds the live dragon
position as camera eye and actor target as camera target, enables attack
camera and sets that target to battle center. Substate 1 plays sound 6D, then
fires once from each intact (state 0) group facing the dragon's quadrant, in
original group order. Substate 2 waits for a signed 90-frame counter to become
negative before setting engine flag 100 and returning to mode/substate 0.

Producer 06056840 creates the original typed child, with Update 06056610 and
Draw 060567F0. One RNG call selects duration 75..106. It transforms the group's
core anchor through the previous inverse camera, adds A000 to Y and copies
the result into the parent's live camera target. Core velocity uses the same
reciprocal-then-multiply calculation as the vector-transition service, then
adds battle auto-scroll. It initializes five quads from 060A3AD8 in resource
slot 13's VDP1 allocation, advances each by its index 0..4 and adds three RNG
velocity components (mask 1FFF, offset -FFF). Creation consumes 16 RNG calls.

Update integrates the core first. Each visual then computes three spring
accelerations from the pre-integration visual position using fresh per-axis
gain (RNG&FFF)+28F, adds them to its velocity, integrates that visual and
advances its quad. This consumes 15 RNG calls per tick. Only after all visuals
advance does the signed timer decrement. When it becomes negative, native
code directly invokes shared applyDamageToDragon for 28 damage, kind 3, impact
at the core, knockback from current dragon position minus core and final flag 0,
then retires the task. There is NO sphere/contact check in this producer;
routing it through the homing projectile's contact policy would change the
authored attack. Draw uses shared scaled-quad submissions at scale 30000.

The production fixture covers nine anchors, all 32 duration seeds, low/high
initial visual jitter and spring gains, exact reciprocal/core movement,
per-visual initial animation offsets, shared resource/descriptor identity,
RNG counts, no damage through timer 0, expiry damage/knockback/retirement,
all five draw submissions and allocation failure. The controller fixture
checks all 78,732 part-state/quadrant combinations, exact intact-group order,
readiness and 90-to-negative completion. Native operands independently anchor
the factory/Update/Draw, timer/masks, previous camera, animation descriptor,
damage 28/kind 3 and scale 30000. Existing rig/draw/damage/emergence/reaction/turn
regressions and final-tree Vita packaging pass.

Field-entry clarification from the user: the required route is to target the
Lathum field mesh with lock-on, play the short authored cutscene, then enter
battle. The current Mound target callback starts script 5B5; native action 54DCC
hands off to encounter 10/completion 90. The interactive 1F0 mesh and this route
are restored in source but visibility/targetability/cutscene behavior still
need device acceptance. The user reports the trigger mesh was not visible;
no automatic encounter or substitute trigger is added.

Overall Update remains undispatched while attack modes 2/3, death and main
animation/effect ordering are unfinished. This checkpoint does not make the
Lathum encounter playable and does not request another boss hardware run.
