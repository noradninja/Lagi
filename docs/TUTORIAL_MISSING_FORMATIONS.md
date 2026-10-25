# Missing native Tutorial 1 formations

Goal 7 remains active. Movement lesson progression was corrected separately in
8600ece; hardware acceptance is pending. Lessons 2, 3 and 4 still have missing
formation dispatch/implementations and must not be reported playable.

## Original Disc 1 evidence

### Shared hit-effect binding checkpoint

`lagiDispatchTutorialModelHit` explicitly maps the T0 callback identities:
0609F440 is a no-op; 0609F444 invokes explosion mode 1; 0609F468 and
0609F48C invoke mode 0. Each uses target.mC (not the hotpoint at +4),
engine.m1A0_battleAutoScrollDelta as velocity, and scale 0x20000. Unknown
addresses return false; no host function pointer is manufactured from an EA.

Native 06074328 matches `createExplosionAtPosition` in gunShotRootTask.cpp:
the overlay-dependent selection is 3/2, 5/4 or 8/7 for modes 0, 1 or 2.
Its callee 0607386C is the existing animated-quad `createParticleEffect`,
not a missing 3D-model factory. Native task size 0x44 and fields (quad +8,
position +10, velocity +1C, acceleration +28, scale +34, colors +38,
lifetime +40) match the shared service exactly. No new render pass is added.

`tests/tutorial_model_hit_test.ps1` checks the production dispatch include,
all 65536 low-word callback identities, live engine switching, the distinct
impact/current position pointers, scale and mode. Particle submission is
mocked; Vita build/link checks are not visual acceptance. Formation dispatch
is still disabled pending remaining bindings/state ownership and hardware
testing. This checkpoint does not claim the later lessons are playable.

Read BTL_T0.PRG directly from the MODE1/2352 disc: LBA 262947, 392928 bytes,
base 06054000. Encounter entries start at 060A989C, stride 0x20; the first
pointer references `{callback, argument, flags}` records, terminated by null.

| Sub-battle | Formation callback | Initialization | Update | Native allocation |
| --- | --- | --- | --- | --- |
| 2 | 060586E0 | 0605863C | 06058740 | 0xF4 |
| 3 | 0605B00C | 0605AEBC | 0605B036 | inspect descriptor 060AB05C |
| 4 | 06055A54 | 0605594C, argument 1 | descriptor 060A9DEC | 0x100 |

These are different authored model-enemy controllers, not aliases for the
generic Urchin formation. Never suppress normal battle completion or insert a
dummy persistent task to hide their missing formations.

Sub-battle 2's initializer allocates NPC/file slot 0x14, sets the enemy turn
timer's m2 to 120, invokes 06067B28 with zero, and displays formation name 6,
mode 0, argument 9. Its entity group has two entries, zero dead count,
position block at native formation +0x2C and data table 060AACC8. Each entity
is created through 060587F0; null creation increments the group's dead count.
The constructor sets state zero and callback +0xD8 to the empty 060586DC.

The 06058740 update increments the engine's running-formation count. On all
entities dead it sets formation state 7 and resets the enemy-turn fields m8,
m0, m2 and m4. It then executes position service 0609DE44 with spring 0xFF,
damping 0xFFF, followed by shared state-machine service 0609E68E. State 7
uses the original delayed task completion, not an immortal formation.

Entity 060587F0 uses native model descriptor 060AADA8, file slot 0x14,
model/targeting services 0609ED96 and 0609EE7C, animation and damage handling
0609F0CA. Its update 06058996 includes target-dependent jitter, hit/death state,
reward counters, and delayed group dead-count increment. These must be restored
before enabling the formation. Raw native work-area offsets cannot be applied
to C++ task pointers: task headers, pointers and vectors have different layout.

## Shared model/target lifecycle recovered

`native_battle_models.h` and `lagi_battle_models.inc` now provide typed model
creation, target initialization and retirement, compiled in the battle build.
They are opt-in services for the missing actors, not replacements for already
accepted Baldor/Urchin paths. No missing formation is dispatched yet.

- 0609ED96 reads eight-byte model descriptors, creates each model with the
  authored static pose/hotpoint bundle and steps animation once. Creation
  failure finishes the owner and stops before later models.
- 0609EE42 initializes only named target state: health floor zero, four
  caller-supplied **overlay-specific** callback EAs, four hitbox values of 10,
  auxiliary count zero and auxiliary data null. The callback EAs are data,
  never native addresses invoked as host function pointers. T0 uses F440,
  F444, F468, F48C under base 06090000, not A5's hard-coded addresses.
- 0609EE7C/EFB0 count targets from `m40` authored bone definitions. Each
  target uses the entry's `m0` flags and `m10` radius. Transformed
  `m44_hotpointData` consists of three-component positions; its element 3 is
  not a radius. The old shared reconstruction indexed that nonexistent
  component and used a position component as flags.
- Native target initialization passes the actor's entity index as argument 6
  (target `m5F`), plus owner and task-owned target-position storage. The old
  shared reconstruction discarded that index. A model without hotpoint data
  has zero targets, bone count preserved and null target arrays.
- 0609F4B0 sets actor flag 80000000, clears 40000000, invokes an optional
  callback **before** testing flag 10000000, and retires model targets only
  on the first ungated call. The callback still runs on repeat calls and may
  affect the gate. Arrays remain in the owner task heap for native deferred
  shot/target cleanup; retirement does not free referenced storage.

Malformed host-vector metadata is rejected before any target is registered,
including when a later model is malformed. The actor is marked finished
rather than exposing partial target registration. Authored valid metadata
follows the original ordering. This adds no renderer, GXM pass or per-frame
model allocation; all allocations belong to setup and task cleanup.

`tests/native_battle_models_test.ps1` compiles the exact production include.
It checks first/middle/last model failure, authored descriptor EAs, one initial
animation step, 8192 target metadata/identity/retirement combinations, malformed
metadata and a deliberately padded task header. Engine/allocator/model services
are mocked. The Vita VELF/SELF/VPK build and retained-root ELF audit pass with
all three services forced into the link; this proves compile/link closure,
not model rendering, frame time or on-device lesson execution. Next restore
the actor's damage/animation/draw path and its original formation controller.

## Shared model damage pipeline recovered

`lagi_battle_model_damage.inc` restores 0609F0CA..0609F43E using a named
actor view and explicit native service hooks. `B4` is a signed **health floor**
(zero disables it), not a damage accumulator; `B6` is the separate pending
damage accumulator. The target-state member was corrected accordingly.

The native operation order is preserved:

1. Actor flag 10000000 returns before position/scale/hit processing.
2. For every model with targets, copy transformed per-bone hotpoints into
   stable target-position storage. Set each target's byte `m60` from the
   actor's quadrant-indexed word table. Apply matching auxiliary model/target
   overrides in row order; the last matching row wins. Model and target row
   indices are signed bytes, table entries are words truncated to bytes.
3. Actor flag 80000000 skips hit consumption for that model, but not the
   preceding position/table updates. This gate is once per model: a callback
   raising it does not abort remaining hits in the current model.
4. Consume target flags 80000 and 20000, set actor hit-flash flag 800000,
   set actor light value from unsigned target `m60`, and invoke the selected
   non-null damage callback for signed impact kinds 0..3 **before** health
   subtraction. Callback EAs remain data handled by the actor's typed adapter.
5. Subtract signed damage with native 16-bit wrap. Display the per-hit result
   only if that intermediate health is nonnegative. Then apply a nonzero
   health floor, notify life-meter byte bit 2 and accumulate damage with
   16-bit wrap. Callback/display mutations remain visible in subsequent steps.
6. After all models, nonpositive health flushes the pending damage even if
   it is zero or negative. Alive actors flush only while engine flag 1000 is
   set and pending damage is positive. The native world-position number
   presentation uses mode 1, then clears pending damage.

Quadrant/status hooks query live state instead of capturing it before hit
callbacks. There is no new hit timing, damage formula, renderer work, per-frame
allocation, callback address cast or enemy-specific substitute behavior.
The existing accepted enemy paths are not redirected through this service.

`tests/native_battle_model_damage_test.ps1` executes the production include
against 29040 signed health/damage/floor/kind/quadrant/status combinations,
plus ordered overrides, invalid target-index protection, live callback changes,
per-model death gates and retirement. It also reruns the 8192 model/target
lifecycle cases. Task/engine/presentation hooks are mocked: these are host
ordering/arithmetic tests, not original SH2 execution or a hardware fight.
The Vita build retains this damage function as an explicit link-audit root.

Still required: bind the original per-enemy hit callbacks and model movement,
animation/draw updates, create the original formations and enable their exact
dispatch callbacks. The last three lessons still immediately complete until
that integration is finished. No new playable-lesson acceptance is claimed.

## Shared motion variants and model draw recovered

The model task descriptor at 060AAD98 is `{0, 06058996, 0609F5B4, 0}`:
the missing enemy uses shared draw service 0609F5B4. The new
`lagiDrawBattleEnemyModels` calls the existing native services in exact order:
conditional lighting if actor flag 800000, then for each model matrix push,
translation by actor position, Y-X-Z rotation, the model's actual draw callback,
and matrix pop. It rereads the live flag after model callbacks and, if set,
clears it and calls the existing `clearLightColor`. It does not unconditionally
restore lighting from a saved pre-draw flag or skip models based on death flags.
It adds no geometry, textures, allocation or GXM pass.

`native_formation_motion.h` additionally recovers:

- 0609DDB4 positional interpolation: wrapped 32-bit difference times fixed-point
  rate, stored as velocity, then integrated into current/world position. This
  variant does **not** sign-extend a 28-bit difference and does **not** clear
  accumulated acceleration.
- 0609E01E angular spring: the signed-28-bit spring recurrence, current angle
  masked to 0FFFFFFF after integration, then parent rotation added to produce
  world rotation. Velocity is not masked; acceleration is cleared.

The formation initializer's literal at 06058728 is 060AACC8, not 060ACCC8;
the earlier note was a transcription error and has been corrected above.
Read the actual table/descriptor before binding a new actor.

Host checks exercise 10752 production draw-service call-order cases, including
zero models, signed light values and callbacks mutating the live lighting flag.
Matrix/renderer calls are mocked, so this is not a pixel/render test. Motion
checks cover 897775 positional spring, 859375 angular spring and 50625 ordinary
interpolation cases against scalar instruction recurrences, plus Vita syntax.
Existing model/target/damage tests still pass. The draw service is a forced
link-audit root in the full Vita build, resolving the existing matrix and
lighting implementations. Hardware performance is not established.

Remaining first missing actor work includes actual construction/state update,
animation selection/step, original hit callback bindings and death effect,
formation lifecycle and dispatch. Shared services existing does not make that
actor or its lesson playable yet.

## First missing enemy update assembled

`lagi_tutorial_laser_enemy.inc` now translates the complete gameplay update
06058996..06058C2E. It uses the actual shared damage/target retirement and model
animation services rather than an isolated alternative damage simulation.
`lagi_tutorial_model_motion.inc` binds the native fields explicitly:

- position: world +30, velocity +48, acceleration +54, target +60,
  current +90, interpolation rate +A8;
- rotation: world +3C, velocity +6C, acceleration +78, target +84,
  current +9C. Parent position/rotation come from the formation block.

Movement runs before the first model's animation step and damage update.
Native position interpolation preserves accumulated acceleration. Angular
movement uses spring FF and damping FFF and clears angular acceleration.
The actor view is passed by reference without allocating or copying model
arrays each frame. It is not yet constructed by an enabled native task.

After a live hit, native 0609F568 latches 40000000 (callback zero), sound 66
plays, and state 1 executes immediately: clear 20000000/40000000, add three
RNG impulses `(rng & 3FFFFF) - 200000` to **angular acceleration**, then state
zero. These are not animation indices or angular-velocity writes.

After a lethal hit, retire the actual targets, play sound 65, notify statistic
selector 8 (0605D6E4 -> 0606B160), and add 16 EXP/10 dyne with 32-bit wrap.
The native routine does not itself force state 2 here. Keep that separate
ownership intact; its binding still needs audit when restoring the formation.

State zero sets all three position targets to authored offset plus
`(rng & FFFF) - 8000` using three original RNG calls. State two calls the
existing `createBoneDebris` with native owner/resources/model, position,
rotation, null velocity/acceleration, gravity -44, mask 1FFFFF, zero extra
parameters, scale 10000 and lifetime zero; then disables drawing and enters
state three. State three increments its signed word timer and, when the old
value exceeds 30, zeros it, increments the group's signed byte dead count
with wrap and marks the actual task finished. No health-driven substitute
death state or immortal formation was introduced.

`tests/tutorial_laser_enemy_test.ps1` executes the exact production update,
motion, damage and retirement includes together. It checks previous-target
movement order, one animation step, live-hit impulses, one-time lethal rewards,
native debris arguments, all 65536 signed timer values and 1536 wrapped motion
trajectory axes. Engine presentation, RNG, model animation and debris services
are mocked; the fixture is not original SH2 execution or a hardware tutorial.
Existing 8192 lifecycle, 29040 damage and 10752 draw cases still pass.
The update and motion wrappers are explicit Vita retained-link roots, so their
actual animation/debris dependencies must resolve in the full VELF/SELF/VPK.

Still outstanding before dispatch: original constructor and initial animation
setup, concrete RNG/sound/statistic/hit-effect/draw-disable bindings, formation
ownership/state transitions and lifecycle. The last three lessons remain
unaccepted and disabled at those missing callbacks; Goal 7 is not complete.

## First missing enemy initialization recovered

`lagi_tutorial_laser_enemy_init.inc` restores the initialization after native
task allocation in 060587F0..06058994, using named references rather than
casting a host task to native byte offsets. The task creator still must bind
the original parent/group, slot-14 resources and typed callbacks before use.

The initializer sets the owner's file bundle, entity index and health 300;
creates the life meter from the actor's world-position/health addresses with
maximum 300 and type -1; then creates one authored model. T0 descriptor
060AADA8 is `{model 0004, pose 0088, hotpoints 060AAD40}`. It initializes
animation from the bundle's pointer-table entry 009C, selects a starting frame
with native **unsigned remainder `(frames - 1)`**, and steps animation once
after selection (in addition to the shared model constructor's first step).

MAIN 06035550..060355F0 confirms this is remainder, not quotient or signed `%`.
Absent animation means frame count zero and unsigned divisor FFFFFFFF; one
frame means divisor zero. The existing generated `performModulo2` adapter
returns zero and bounds arithmetic-error-044E diagnostics in that latter case,
matching the original nonfatal helper. The unchanged upstream assertion is
not the production adapter; both the generated adapter and constructor fixture
were checked.

Target setup receives the entity index and overlay-specific callback EAs,
then overrides quadrant values to `{30,1,10,1}`. The authored position offset
is copied into target/current position, rate becomes 0F5C, and world/current/
target rotation becomes `{0,08000000,0}`. Other native fields (world position,
velocity, acceleration, task header) are deliberately left untouched. The
actual task's zero-initialization remains responsible for their initial state.

On failed model/target setup, finish the already-created life meter as well
as its failed actor. Life meters are dragon-owned tasks; leaving one active
would retain a pointer to failed actor health. Missing bundle or life meter
also fails before exposing a half-created actor. No replacement resources or
guessed fallback animations are introduced.

`tests/tutorial_laser_enemy_init_test.ps1` executes the production initializer
for all 65536 frame-count words and seven unsigned RNG boundary values
(458752 combinations), null animation and resource/life-meter/model failures.
It checks native setup order, two animation steps, target defaults, unchanged
adjacent named fields and padded task headers. Engine/resource services are
mocked, not a real disc model or device execution. Existing model/damage/draw,
integrated actor update and generated-remainder fixtures pass. The constructor
is a forced Vita link root, resolving real animation and life-meter services.

Remaining: actual typed task creation, original formation/group binding and
ownership, concrete hit-effect/statistic service bindings, formation controller
and dispatch, then full on-device lesson/title-return/performance acceptance.

## Concrete actor task assembled (not yet dispatched)

`lagi_tutorial_laser_enemy_task.inc` now supplies an actual
`s_workAreaTemplateWithCopy` work area and task factory. Its typed definition
matches 060AAD98: no Init callback (explicit native post-allocation setup),
the recovered Update, shared Draw and no Delete callback. It is compiled
against the real Vita task classes, not a native-address task blob.

Each actor owns its model vector, target state, position/rotation motion,
health, life-meter pointer, state/timer and copied authored offset. Explicit
references retain the formation's position/rotation, dead count and engine
reward counters. Constructor resources/descriptors/callback EAs are supplied
from the native formation boundary, not guessed from enemy identity.

The factory copies concrete service bindings and bridges their context
separately from actor identity. Native sound/RNG/statistic/target callbacks
reach the supplied service context; drawing-disable always clears that
actor's real typed Draw method. The caller's generic hideDrawing hook is not
used by the task wrapper. Two actors can share services and formation state
without sharing the drawing-disable target, health or model storage.

The actual `sEnemyLifeMeterTask::m31` is **u8**; concrete binding exposed the
earlier signed-reference mismatch in the damage view. The public view and
fixtures now use that authoritative unsigned type without a pointer cast.
Existing byte flags and arithmetic are unchanged.

Incomplete service/resource bindings reject creation before allocating a task.
Native initialization failure leaves the actual failed task marked finished
and finishes its newly created life meter. No live placeholder is returned.
The native Delete callback remains null; normal graph disposal and the
debris-state ownership still need full formation/hardware lifecycle validation
before dispatch, not an invented task-specific cleanup sequence.

`tests/tutorial_laser_enemy_task_test.ps1` compiles the production task factory,
constructor, update, motion, damage and retirement includes together. It creates
two actors, checks distinct storage/life-meter health pointers, live parent
motion references, actual Update/Draw callback entry, debris drawing-disable
isolation, group count/task completion and failed/incomplete setup. Task-kernel,
animation, presentation and resource services are mocks; this is not a device
task graph. The factory is forced into the Vita link audit, retaining its
constructor/update/draw call chain and actual native task templates.

Still required before this lesson can run: original formation initialization
and update, concrete shared statistic/hit-effect bindings, authored state/death
ownership, dispatch, title return and hardware performance/visual acceptance.
The remaining tutorial callbacks are still disabled; Goal 7 remains active.

## Shared formation spring recovered

### Typed laser-lesson formation checkpoint

The production formation now recovers 060586E0/0605863C, update 06058740
and deletion 060587CA. It owns the two authored enemies at 060AACC8,
slot-14 resource reference, shared formation motion, reward references and
dead count. Initialization preserves the native 120 timer and formation-name
call. The all-dead path clears all four attack timer fields on every update
and finishes after 62 updates from a zero counter (old counter > 60).
Failed child construction contributes to the dead count without inventing
an enemy. Deletion releases the original NPC resource reference.

`tests/tutorial_laser_formation_test.ps1` exercises the actual formation,
typed task, constructor and update code together, including 65536 signed
completion-counter boundaries, shared ownership and child failure. Kernel,
engine and rendering services are mocked. The Vita link audit retains the
formation entry and its call chain; this is not hardware acceptance.

Dispatch remains disabled pending concrete original statistic/hit-effect
bindings and remaining state/death ownership audit. The last three lessons
are not claimed playable; Goal 7 remains active.

`include/lagi/native_formation_motion.h` recovers one axis of native
0609DE44..0609DF72 using named state, without a host/Saturn layout cast.
The original three axes are independent with the same recurrence:

1. Subtract target/current with 32-bit wrap.
2. Sign-extend bit 27 by OR F0000000 when set, otherwise AND 0FFFFFFF.
3. Add signed fixed-point spring force to accumulated acceleration.
4. Subtract fixed-point velocity damping.
5. Add acceleration to velocity, velocity to current, current to world center.
6. Clear accumulated acceleration.

All adds/subtractions wrap; multiplication returns product bits 16..47.
Negative products round down, not toward zero. No floating point, allocation,
clamp, guessed interpolation, rendering state or new GXM work is introduced.

The literal constants at 0609E0BC/C0/C4 prove the difference is signed **28**
bits, not 24. Position source fields are +0x18 velocity, +0x24 acceleration,
+0x30 target, +0x60 current, and +0x00 world. The reconstructed upstream
two-argument `formationUpdatePosition` is not a valid substitute for this
four-argument spring service; its differently documented entry also must not
be silently treated as a matching function solely because the names resemble.

`tests/native_formation_motion_test.ps1` compiles the production header and
compares 897775 boundary/random/trajectory cases against a separate scalar
rendering of the SH2 recurrence. It also checks Vita C++ syntax. This does not
execute SH2 or a Vita, and the primitive is not yet called by an enabled native
formation. It does not repair the last three lessons or establish hardware
performance. Next restore their typed entity/model/target lifecycle and bind
the native spring with the exact authored parameters.
