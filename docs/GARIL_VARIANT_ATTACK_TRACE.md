# Garil variant attack trace - 22 November 2026

Current source checkpoint: ea2d32b. This is original-data evidence, not a
runtime fix or device acceptance. Lathum testing remains independent.

## Confirmed missing launch

In `BTL_A5_variant.cpp`, enemy Update state 4 advances animation and, at frame
0x2E (46), only plays sound 0x6D and enters state 5. Original BTL_A5.PRG
Update 06055086 instead performs the following at 06055286-060552D8:

- Reads targetable 5 through model+0x54 (0x1F4 = 5 * 0x64),
  taking its live world-position pointer at targetable+8; copies XYZ to actor offsets 0x1D8/0x1DC/0x1E0.
- Calls factory 06054384 with actor, copied source vector and dragon+0xF0.
- Plays sound 0x6D and enters state 5.

No implementation of factory 06054384 exists in the current adapter or
upstream overlay. Restoring sound alone cannot restore this attack.

## Factory and child timing

Factory 06054384 allocates a 0x30-byte task using callback table 060AD104.
For non-null source and destination, it computes destination minus source,
then calls 0603570E and 060359AE. Each velocity component uses 06035664 with
literal 0x1000 followed by 06035644 with the corresponding delta component.
The precise math helper identities must be verified before implementation.

It initializes a quad from descriptor 060AD2D8 and resource slot 0xE, stores
signed counters 27 and 128 at offsets 0x10/0x12, allocates 0x45C bytes, and
initializes 31 records of stride 0x24 using 060545EC. Initial task flags at
0x28/0x2C are 1/0. A single RNG call sets byte 0x24 to (rng & 0x0E) + 13;
this yields odd values 13 through 27, not an unrestricted random range.

Update 060544DC decrements counter 0x10 and compares its old signed value
against zero to change flags to 0/1. Damage is checked against the NEW counter
matching byte 0x24. At that moment 06054532-06054550 calls shared dragon
service 060642F8 with targetable dragon+0x8C, damage 44, kind 2, impact
position dragon+8, zero knockback vector and stack flag 0x0800. There is no
sphere-contact test in this launch/timing path.

After the active flag switches, counter 0x12 uses another old-value negative
check. Expiry writes 0x1000 to dragon+0xD8 and finishes the child. Every update
also visits all 31 visual records through 06054680. The draw callback
060545A4 visits all 31 through 060546AA, then advances the shared quad via
0602CF38. There is no native delete callback in this task definition. Do not replace this task with a homing
projectile or contact-triggered damage merely because those services exist.

## Reproduction and remaining work

Disassemble the extracted original BTL_A5.PRG with GARIL_DISASM_BASE=06054000
using garil_saturn.exe --disasm. Inspected ranges: 06055086-06055488,
06054384-06054540 and 06054532-060545EC. Literal values above come from the
big-endian PRG words, not guessed structure offsets in the host port.

Next: trace 060545EC/06054680/060546AA and callback table 060AD104; identify
math helpers from common code; restore task through a generated-source
adapter and shared dragon-damage/quad services. Audit the formation selection
and release gates separately: current formation state 3 also lacks its native
turn selection. Prove both gates before presenting this enemy as functional.

## Callback and particle trace correction

The callback table at 060AD104 is exactly {0, 060544DC, 060545A4, 0}.
060546AA is a per-particle DRAW helper, not deletion. It copies the live XYZ
at record+4/+8/+C and calls quad draw 0602CF7C with owner-task+8. The wrapper
060545A4 draws 31 records then advances the owner's single shared quad once.
Do not create 31 independently advancing animations or release the quad from
this draw callback.

Each record initialized by 060545EC stores owner at +0, source XYZ at +4,
velocity XYZ at +0x10, random word at +0x1C and active=1 at +0x20. Velocity
starts at MTH_Mul(0x28000, launchVelocity) per axis (literal 060546E8).
Four RNG calls per record, in order:

1. X velocity += (rng & 0x1FFF) - 0xFFF.
2. Y velocity += (rng & 0x1FFF) - 0xFFF.
3. Z velocity += (rng & 0x07FF) - 0x3FF, then add engine auto-scroll Z.
4. Store rng & 0x7F as the word at +0x1C.

The fourth value is not used by the traced update/draw helpers; retaining its
RNG consumption is necessary. Across 31 records, initialization consumes 124
RNG calls, followed by the factory's one timing RNG call (125 total).

06054680 skips inactive records. Otherwise velocity Y += -0xB0, then position
XYZ += the resulting velocity. There is no per-record expiry, fresh RNG,
contact test or quad-step call. The native 32-bit additions must wrap.

## Formation gate omissions

Original formation Update 0605493C has states 0 through 8. The port has only
0 through 4 plus a relabeled death state 0xB. Original death state is 8.
The missing states are consequential, rather than harmless unused cases:

- State 3 checks player turn, formation byte +0xF2 and engine turn-ready
  m3CC->m8 == 1 before calling selector 06054C2E.
- State 4 uses the signed old counter >15, sets attack-finished, enters 5.
- State 5 waits for engine flag 0x400000 to clear, returns to 3 and clears
  m3CC->m8 and m3CC->m0.
- State 6 waits for engine 0x2000, displays attack name 15 for 30 frames,
  invokes group helper 060A2AB0 and enters 7.
- State 7 uses signed old counter >105, sets attack-finished, enters 5.
- State 8 uses signed old counter >75 then releases resource 0xE and finishes.

Original intro state 2 enters 5 after its signed old counter >84; it does not
enter the port's state 4. Selector 06054C2E marks each live actor selected by
formation byte +0xF3 with flag 0x04000000. If any selected actor exists it calls
060A28A4; success sets battle mode 7, plays quadrant camera and enters state 6.
Failure returns to state 3 and releases turn ownership. If no selected actor
exists, byte +0xF8 postincrement alternates live actor movement flags 1/2,
sets mode 7 and enters state 4. Full helper identities and actor/formation
position updates must be connected before this encounter is advertised as
restored. Avoid merely adding the frame-46 launch to an unreachable attack.

## Implemented timed burst checkpoint

The generated variant adapter now launches `lagi_garil_variant_burst.inc` at
frame 46 from model targetable 5's live world-position pointer. This follows
the same typed targetable mapping used by the existing single-enemy adapter;
it does not reinterpret host model or task memory using Saturn offsets.
Factory normalization uses the shared dot-product, square-root, division and
multiply services. Common RAM disassembly confirms 0603570E is the three-axis
MAC product shifted by 16; upstream sqrt_F is explicitly identified 060359AE.

The child retains all 31 visual records, 125 factory RNG calls, per-frame
ballistics and one shared Draw animation step. Damage uses the existing
applyDamageToDragon service; expiry writes the named dragon targetable m4C.
No contact gate, new battle engine, replacement resources or story flag is
introduced. The native hit-timer value 27 is preserved even though the first
update decrements 27 to 26 and therefore never matches that choice. Do not
silently turn this native outcome into guaranteed damage.

`tests/garil_variant_burst_test.py BTL_A5.PRG` compiles and executes the actual
production task/factory/launch code. It verifies every timing choice, exact
125-call RNG order and per-particle initial velocities, all 159 update ticks
to expiry, damage arguments/count, integer wrap, shared Draw stepping,
allocation failure and targetable 5 selection. It also checks native callback,
RNG, motion, damage and timer literals from the original PRG. Vita compilation,
linking and VPK packaging pass. This does not establish device acceptance.

Formation selection/release gates and actor position updates remain pending;
this checkpoint restores the effect implementation, not the whole encounter.
The earlier Lathum checkpoint remains the appropriate independent Vita test.

## Implemented formation selection and release checkpoint

The generated variant adapter now restores the native turn-ready/reaction
latches, selector 06054C2E and states 3-8. It uses existing typed
lagiVisitFormationActor / lagiAnyFormationActorFlags /
lagiSignalFormationActors / lagiTransitionFormationActors services. Finished
actor pointers retire through the ordinary task API, never Saturn task offsets.

The selector ORs ready flag 0x04000000 only into live members of byte +F3.
Its native helper 060A28A4 is an any-ready-actor predicate, not a random choice.
The successful attack branch sets battle mode 7 and marks the dragon quadrant
using battleEngine_FlagQuadrantBitForAttack; it does not launch a guessed
camera. State 6 releases ready actors to 0x02000000 through the same shared
transition used by single and Nanyd formations. Movement-only turns alternate
flags 1/2 using native byte +F8. Intro state 2 now joins state 5, and all-dead
retirement uses native state 8. Counter increments preserve 16-bit wrap and
compare the old signed value at the native >15 / >105 / >75 boundaries.

`tests/garil_variant_control_test.py BTL_A5.PRG` executes the actual generated
phase block and production selector. It covers 2,592 combinations of member
masks, all quadrants, null/live/finished actors and movement counter choices;
player/reaction/turn-ready gates; shared release preserving unrelated flags;
turn ownership return; and every signed 16-bit counter value for states 4, 7
and 8. Original PRG words check the field offsets, ready/release bits, attack
name/duration and timers. The burst regression and full Vita packaging pass.

This remains a partial encounter checkpoint. Original 06054872 formation
membership/position updates, actor movement and consumption of release flags,
and intro transition/speech must still be audited before device acceptance.
No new hardware result is inferred from these source and host checks.

## Implemented formation occupancy checkpoint

Original 06054872 counts the three bytes at formation+F4..F6 across four
quadrants; retired byte 4 and other out-of-range values match none. It clears
both radar classifications, then classifies count 0 as danger, count 1 as
neutral and counts 2/3 as safety, using shift table 060AD194 = {0,1,2,3}.
The generated adapter now restores this exact sequence via shared safety and
danger services. Attack and other radar bits remain intact. The native
formation Update calls this helper before shared formationUpdatePosition;
that previously omitted interpolation call is restored in the same order.

The existing production-controller fixture now exercises 6,400 occupancy
combinations, including all 256 first-slot values, retired entries and mixed
occupancy, plus radar clear/set order and preservation of unrelated bits.
Original PRG table values and generated call ordering are checked. Selection,
phase and counter regressions pass; full Vita packaging passes.

Actor update 0605553C is a separate missing producer: it updates formation
membership byte +F3, actor orientation, altitude and targetable weights. Its
restoration remains pending, along with release consumption and intro work.
Occupancy classification alone is not evidence of a playable variant battle.

## Implemented authored quadrant and membership checkpoint

Original factory 06054E56-06054E7A reads the direction table 060AD5B8 using
formation descriptor byte +0x11 and entity index, then stores that byte into
formation+F4+index. The port read the direction for rotation but omitted the
formation store. The generated adapter now retains the same authored byte.

Original actor update 0605557E-060555CE compares its current formation quadrant
with the dragon quadrant, setting or clearing only its own bit in formation
byte +F3. This producer now runs before damage and state dispatch, matching
the native Update order. It preserves every other actor's membership bit and
handles retired/out-of-range slot values without creating membership.

The production controller fixture now checks 786,432 combinations of actor
index, dragon quadrant, every slot byte and every prior membership mask.
Generated initialization/update boundaries and the native store instruction
are checked, alongside existing selection, occupancy, timing and burst tests.
All pass, as does Vita compile/link/VPK packaging. This is source/host evidence.

Remaining actor work includes native positional interpolation, rotation,
altitude, per-quadrant target weights and release consumption. Intro speech
and transitions also remain pending. This partial checkpoint is not device
acceptance of the complete variant encounter.

## Implemented reaction ownership checkpoint

Original 06055146-0605515C sets bit entityIndex in formation byte +F2 when
starting a damage reaction; 06055380-06055398 clears it on reaction completion,
and 0605540C-06055422 clears it during actor retirement. The generated actor
now performs these typed writes at the corresponding entry/completion/death
boundaries. Other actors' bits and the independent +F3 membership mask remain
unchanged. The restored formation gate can therefore observe the same latch.

Original helper 06055030 runs after damage handling and before actor state
dispatch. Nonzero damage byte +1D5 is consumed/reset; value exactly 1 also
clears actor flags 0x20000000 and 0x40000000 (literal masks DFFFFFFF/BFFFFFFF).
These are distinct from ready/release flags 0x04000000/0x02000000. The new
helper preserves that distinction and does not cancel attacks speculatively.

Production fixtures cover every old damage-byte value, representative actor
flags and every old reaction mask for each actor on both entry and exit.
Native mask literals and generated call counts/order are checked. Existing
variant selection, membership, occupancy, timing and burst regressions pass.
Full Vita compile/link/VPK packaging passes; hardware acceptance remains open.
Actor positional/rotation updates, target weighting, full release consumption
and authored intro behavior remain pending.

## Implemented positional interpolation checkpoint

Original 06055540-06055558 invokes shared 060A23C0 on actor+0x30 relative
to its formation position block, then overrides world Y from actor+0x1C8.
The host names are misleading: m60_targetPosition is the local target,
m90_initialPosition is the live local interpolated position, m48 is positional
velocity, and mA8_soundEffectId is actually the interpolation rate 0x1999.
The adapter now maps those fields through existing
stepNativeFormationInterpolation, preserving subtract/multiply/add rounding
and 32-bit wrap. A named adapter altitude field represents native +0x1C8
without reinterpreting padding or Saturn task layout. Its native factory value
is zero (06055020-06055022). Rotation remains a separate unimplemented producer.

The former port-only per-actor scroll additions are removed: native actor
position is recomposed from formation world position each update. Interpolation
runs before membership and damage, in the original order. The actor's local Y
interpolator still advances even though its world Y is then overridden.

`tests/garil_variant_motion_test.py BTL_A5.PRG` compiles the production helper
and checks 12,000 frames against an independent scalar fixed-point recurrence,
including changing target/center, signed extremes, rate 0/1/0x1999, overflow
and independent altitude. Original operands and generated ordering are checked.
All motion, controller and burst tests pass; full Vita packaging passes.
Hardware acceptance, rotation, target weighting, release consumption and intro
behavior remain open. No complete-encounter claim follows from this checkpoint.

## Implemented quadrant rotation checkpoint

Original 0605555A-0605557C writes desired rotation Y from 060AD5A4 indexed
by the actor's current formation quadrant, then invokes shared 060A2580 with
parent rotation at formation position block+0xC. The table contains exactly
{0,0x04000000,0x08000000,0x0C000000}. The generated adapter now performs
that lookup and maps desired m84_displayRotation, current m9C_targetRotation,
world m3C_rotation and a named angular velocity through the existing ordinary
interpolation service with rate mA8 (0x1999).

This helper does not normalize angle deltas or wrap the current angle: native
060A2580 uses ordinary subtraction/multiply/add. A shortest-route angular
spring would change authored behavior at the quadrant boundary. Parent rotation
is copied as authored block data using the existing Nanyd adapter convention;
no Saturn work-area offset is applied to a host actor pointer. Execution order
is position, rotation, membership, damage, damage-byte consume, actor dispatch.

The production motion fixture adds 12,000 rotation frames across all quadrants
with nonzero parent XYZ rotation, checking native long-route interpolation
against an independent scalar recurrence. It checks the original PRG angle
table and generated ordering. Position, membership, reaction, formation and
burst regressions all pass. Full Vita packaging passes. Target weighting,
release-state consumption and authored intro behavior still require work;
complete variant gameplay and visuals remain unverified on hardware.

## Verified actor release protocol (no additional gate required)

The pending release-consumption audit is now resolved for actor states 3-8.
Original 06055256-060553A4 is animation-driven: state 3 waits until previous
animation frame >>1 is zero, then starts the attack animation; state 4 launches
at returned frame 46; state 5 restores idle at the final animation frame.
There is no separate actor wait on release flag 0x02000000 in these states.
The formation's shared ready-to-release transition clears ready 0x04000000,
which prevents idle from starting the same attack again. Adding an extra wait
would change the native state machine. States 6-8 retain reaction frame 43,
quadrant update and completion latch handling.

`tests/garil_variant_actor_phases_test.py BTL_A5.PRG` compiles the actual
generated actor cases 3-8. It exercises previous-frame gates with ready,
released, both and neither flag; every attack/reaction frame; one frame-46
launch with sound; final-frame idle/reaction transitions; all dragon quadrants;
and the attack-to-idle chain. Native frame operands are independently checked.
These host checks pass; no runtime change or fresh device test is needed for
this test-only checkpoint.

The remaining four values at actor+144..14A are damage scaling inputs, not
merely targeting priority. Original helper 060A3488 initializes them to 10;
actor update 060555D0-06055632 writes three values 5 and one 45 according to
its current quadrant. Existing shared lagiUpdateBattleEnemyModelDamage already
consumes LagiBattleModelTargetState::hitboxValues by the live dragon quadrant.
The current variant bypasses that shared path with a simplified damage loop;
connecting the model/target initialization and shared damage view is the next
required work. Merely copying values into an unused array would not fix it.
Authored intro behavior and full variant hardware acceptance also remain open.

## Implemented shared model/target initialization checkpoint

The variant now uses lagiCreateBattleEnemyModels with original descriptor
060AD5C0 = {hierarchy 4, static pose 40, hotpoints 060AD278}. The constructor
selects animation 48, randomizes its frame using the original RNG/modulo order,
and builds collision hierarchy 8 with pose 44. The previous port interpreted
model indices as animation indices and derived the collision pose from the
wrong table word. Actor state resources are corrected too: idle 48, reaction
4C, attack 50, as the native bundle loads explicitly show.

Shared lagiInitBattleEnemyModelTargets initializes callbacks and stable
model-owned target arrays. The current actor damage and retirement loops now
visit these same live arrays through typed spans; they no longer inspect an
independent host vector. Launch targetable 5 therefore refers to the same
registered set. Enemy COMMON record ID comes from group+0xC directly, matching
06054E42, rather than reading a word from the positional table at group+8.
Initial native quadrant values {45,5,5,5} are retained in the target state.

The production constructor fixture checks descriptor/animation/collision slots,
shared target binding, callback table, initial values and all allocation/init
failure paths. Native descriptor bytes, constructor operands and all three
animation slot operands are checked. Generated damage/retirement loops must
both use model target arrays. All five variant host checks pass; full Vita
compile/link/VPK packaging passes. Quadrant value refresh and shared damage
processing remain pending; this checkpoint alone does not make scaling live.
Authored intro behavior and complete hardware acceptance remain open.

## Implemented shared damage processing checkpoint

The variant now calls the same lagiUpdateBattleEnemyModelDamage service used
by earlier model enemies. Original actor update refreshes four values before
damage: all 5, except 45 in quadrant (actorQuadrant+2)&3. The typed adapter
writes those live values; the shared pipeline copies the selected dragon
quadrant's value to targetable m60 while refreshing transformed target positions.
This restores the target damage-scaling input, not an extra multiplication of
m58 after a shot has already computed damage.

The simplified variant hit loop is removed. Shared callbacks handle authored
impact effects, light/hit feedback, signed health subtraction, pending damage,
engine-window/death damage displays and meter hit flags. The actor retains its
native outer death/reaction state transitions. Callback identities and effect
routing match the already established single-enemy adapter. HP/rewards are not
awarded by a replacement variant system; meter/result services remain shared.

`tests/garil_variant_damage_test.py BTL_A5.PRG` executes the actual variant
adapter with the actual shared pipeline, checking all actor/dragon quadrant
pairs, display windows, two live target hits, callback/impact routing, health,
accumulation, displays, meter flags, target position refresh and retired gates.
The shared model/damage suite passes 29,040 damage boundary/order cases plus
8,192 metadata/identity/retirement cases. All six variant host checks pass;
full Vita compile/link/VPK packaging passes. Original value literals and
generated position/membership/value/damage ordering are checked.

Authored intro camera/speech, remaining constructor/task-lifetime auditing and
complete variant hardware acceptance remain open. No hardware result is
inferred from these source, host and build checks.
