# Missing native Tutorial 1 formations

Goal 7 remains active. Movement lesson progression was corrected separately in
8600ece; hardware acceptance is pending. Lessons 2, 3 and 4 still have missing
formation dispatch/implementations and must not be reported playable.

## Original Disc 1 evidence

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
position block at native formation +0x2C and data table 060ACCC8. Each entity
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

## Shared formation spring recovered

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
