# Camp callback audit — 2026-11-05

## R1 battle weapon attachment correction - 2026-11-05

Original Disc 1 BTL_A3 0609FE12 is the weapon draw helper called at
0609FB26. Instructions 0609FE2E..0609FE32 load the rider model's +44
hotpoint pointer array (rider +5C), then pointer offset24 for dragon level8.
0609FE4C..0609FE50 use pointer offset14 for other levels. These are slots9
and5 in a four-byte pointer array. The upstream C++ translation mistakenly
uses slots3 and1. The owned battle source adapter now selects slots9/5,
retaining the original empty-slot fallback, inverse-view conversion, quadrant
rotation and model-table submission. Flight already selects slot5 and is not
changed; its missing gun remains under investigation. No extern/Azel edits.

Validation: generated battle source inspected for both slot selections; accepted
world_garil profile configured; Vita compile/link/SELF/VPK passed. Hardware
weapon visibility and attachment are UNTESTED. This does not complete R1.

`tests/rider_weapon_attachment_test.py` passes against the original Disc 1
battle and flight opcode sequences, COMMON Edge hotpoint counts and generated
battle helper. It pins the pointer-array layout independently of the adapter.
Flight0607449E reads slot5 already; EDGE.MCB table24 resolves a nonempty
four-quad weapon model. Flight runtime pose/transform/submission still needs
evidence; these checks do not prove visibility.

## Hardware acceptance and remaining visuals - 17:45 capture

User confirms Camp transitions, dragon healing/animation, saving, waiting until
night/day, Garil loading, returning to previous areas through WORLD, entering
WORLD from gameplay and travelling back to Camp. Capture/log:
`2026-11-05 17-45-27 - Lagi - Lagi`. These are hardware functional passes for
the tested route, not proof of every story branch or the33ms worst-case budget.
Dragon feeding/petting remains explicitly UNTESTED. Gash remains inactive in
this capture, which predates the actor cutover below.

Two visual issues remain: every VDP2 ground plane drifts with the camera
(especially obvious in the desert, described as a screen-projected texture),
and the Camp campfire-floor sprite has an incorrect hard/blended edge. The
capture overview confirms the latter. Investigate shared RBG world-to-plane
coordinates/presentation and native sprite color-calculation state separately;
do not add compensating scroll offsets or guessed alpha. Preserve point sampling
and SGX projection/cache performance. These issues are not fixed by the NPC commit.

## C2 native Camp NPC cutover - 2026-11-05

Definition06080148 now creates a concrete native17C NPC. Resource wait0606F530
loads the original hierarchy/pose before installing active update/draw callbacks,
and bit40 creates the recovered segmented child. Initialization registers both
typed task/NPC receivers, sets collision links and uses Camp's original control
map. Mode0 decays look-at, mode1 plays authored pose records, mode2 runs the
original bounded patrol, mode3 uses the known authored06070EB8 movement callback,
and mode4 schedules weighted animations. No shared Unimplemented mode2 remains.

`camp_npc_patrol.h` ports060709E8/06070ABC, retaining collision application,
native boundary comparisons, random-word reuse, integer turn duration versus
FP_Div walking duration, signed -113 speed, turn/look rates, early returns and
animation selections. Native+14C/+14D bytes are explicitly packed logically,
independent of host endianness. The initial MOV.B1 at+14E retains the native
big-endian word0100 rather than assuming it writes a host short1.

Verified shared NPC active animation, scripted turning and drawing bodies are
adapted from the pinned source. Camp's idle selector uses original06080180 and
post-decrement timing, not another town's hard-coded distribution. Drawing uses
the live view-relative bridge scope, including normal/interpolated pose paths,
shadow flags and the child. Delete protects replacement registry owners. The
task disposer releases C++ model vectors after native child/Delete/heap cleanup.
No renderer, camera or RBG implementation changes are included.

Native instruction/resource anchors, all patrol choices, boundary timing/equality,
normal/interpolated child attachment service contracts, and actual generated NPC
Init/resource-wait/Draw/Delete methods pass host fixtures. The latter covers all
five control modes and shadow/child flag combinations. Existing Camp script and
actor fixtures pass; full Vita compile/link/SELF/VPK packaging passes. Matrix
service mocks and source checks do not establish rendered parity or performance.
Hardware must confirm Gash visibility/activity, approach/interaction/dialogue,
scripted turn/animation, Camp exit/re-entry and pending dragon interactions.
C2, R1 and B1 remain open. This supersedes the unbound-actor status below.

## C2 segmented-child service checkpoint - 2026-11-05

`src/integration/lagi_camp_npc_body.inc` recovers the complete native child
factory0606ABE8/definition0607EB94, Init0606AA04, Update0606AB88, Draw0606ABD2,
attachment0606A888 and movement impulse0606AAB8. It uses the existing native
segmented-body services, seven linked parts, model8/pose70 seed translations,
the original0607EAFC templates, normal/interpolated parent bone poses and
attachment vector0607EB88. The original separate Z/X/Y inverse rotation and
wrapping SHLL14 impulses are preserved. Each update writes body rotation target
+18; it does NOT overwrite base rotation+0C, which retains its initial pose.
The host-owned body/vector has an explicit task Delete callback.

The adapter is compiled into generated Camp but is not yet invoked. Definition
06080148 remains unbound until its full control pipeline is recovered. Do not
substitute the existing Zoah class without checking its initialization/control
callbacks, original idle table and child factory. No new hardware run is needed
for this unused prerequisite; keep the ec3199a recovery/feeding package for tests.

Original-disc instruction/resource anchors and the production child fixture pass
for initialization service calls, seed translations, both pose banks, attachment
service order, motion impulses, base/target separation, drawing scope and cleanup.
The fixture substitutes matrix/physics services, so it does NOT prove their math
or rendered parity. Full Vita compile/link/SELF/VPK packaging passes; the child
still requires integrated actor and hardware validation.

Original Camp control map0606FF30: mode0=0606F8BA look-angle decay;
mode1=0606F974 pose-record playback; mode2=06070ABC bounded wandering/patrol;
mode3 uses the entity's callback pointer; mode4=0606F8D8 weighted animation
schedule. Authored06080148 records include mode2 (for example0607BAE0 and
0607C9C4), plus bit40 segmented-child variants0607B8AC/0607B900/0607B954.
Shared initEdgeNPC still leaves mode2 Unimplemented. Camp's walking/idle selector
060718BE/060717A0 uses original table06080180, not the Ruins table or an invented
idle distribution. Recover those native services before enabling this class.

## C2 feeding and interaction-state correction - 2026-11-05

Native feeding continuation060559A4 now runs through the existing Camp mode
dispatch. It waits for ready-state10, installs the shared special dragon Draw
with highlight00001F00, plays sound6F, starts command sequence0607AF5E on the
next mode update, and restores normal Draw/substate0 only after the native mE
animation dispatch returns. Unlike recovery it has no particle factory or camera
interpolation. Feeding setup resets native phase+30 without overwriting head pitch.

The native+2C timer/+30 phase correction also covers the exact mode1..6 source
block. Those translated petting/ambient branches previously reused animation
sound count+28 and head pitch+3C. The bounded generated-source adaptation leaves
sound-table lookup/initialization and head tracking outside that block intact.
This fixes state ownership without changing the authored mode choices or delays.

Native feeding instructions, green highlight, callback/command targets and
interaction timer/phase stores are pinned in the recovery fixture. Production
phase tests cover readiness blocking, distinct sound/animation calls and final
callback restoration; signed color/recovery regressions still pass. Vita full
compile/link/SELF/VPK packaging passes with the retained comparison profile.
Hardware feeding/petting and the33ms deadline remain unverified. C2 still needs
the06080148 actor and segmented child pipeline, plus save/exits/re-entry checks.

## C1 recovery runtime cutover - 2026-11-05

The complete060557BC recovery continuation now replaces the translated stub
through `src/integration/lagi_camp_recovery.inc`. The native mode dispatcher
still gates recovery on mE==0; animation commands own their intervening frames.
Camera approach uses41 updates (40/40 through0/40); readiness10 precedes the
lighting callback, sound6F, native35-particle factory and animation0607AF5E.
The return uses40 mode updates and restores the original dragon Draw callback.
The unchanged script poll releases control; no timer bypass or forced flag.

Important translation correction: original timer+2C and phase+30 were omitted
from the host sTownDragon. The translated setup incorrectly reused+28 sound
count and+3C head pitch. Recovery now has separate typed state, preserving
animation sound tables and head motion. Native instruction anchors pin all four.

`lagi_camp_recovery_effect.inc` ports factory0605669C, Update060569EC,
Draw060570F0 and particle step06057226. It shares the verified fixed-point motion
arithmetic with the battle effect, but NOT that effect's resources or lifecycle:
Camp uses descriptor06073CA8, colors0607B052, target0607B264, native file entry2,
and skips expired particles except the final one on the terminating draw.
The original repeated motion-ring initialization/RNG progression, frame70 state
change, source order, transforms and scaled color-table emission are retained.
Task heap arrays and the parsed quad vector are released on expiry/parent teardown.
The native+34 factory argument remains stored but unused by Update/Draw.

The special dragon Draw applies the signed packed-color transitions through
setupLightColor, submits the normal animated dragon, then restores all three
saved light gradients. Camera-target writes use the native no-op-update service.
No renderer, WORLD transition, shader, input or comparison-profile changes.

Validation: native CAMP instruction/resource anchors,2031616 signed color
cases, camera approach/readiness/return tests, shared effect-motion tests and
original BTL_A3 anchors pass. Full Release compile/link/SELF/VPK packaging passes
with `cmake/world_garil_profile.cmake`; FULLRES remainsON and GPU_PASS_TIMINGOFF.
This supersedes the unused-helper status of the arithmetic checkpoint below.

C1 remains OPEN pending hardware: one capture should enter Camp after damage,
show recovery text, dragon animation, white light/blue particles, restored
lighting/camera and working Edge movement; then re-enter with full HP/BP and
confirm the authored no-recovery path. Include the matching log. Preserve the
exact package/unstripped executable/velf together as described in
`WORLD_MAP_WORK_BUILD.md`. This fixture does not prove Saturn visual parity or
the33ms hardware frame deadline. C2/R1/B1 remain open.

## C1 recovery arithmetic checkpoint - 2026-11-05

Added `include/lagi/camp_recovery.h` with exact logical host state for native
060555E2/06055634 color transitions and060557E8/06055906 camera-target
interpolation. Native setup stores packed RGB as signed channel bytes; it is
not RGB555. Highlight begins after30 draw calls and restoration after120,
with30-step transitions. Delay processing precedes interpolation and restores
last if both expire together. A zero-duration equal-color transition retains
the native divider call; never perform C++ integer division by zero instead.
Native0601DFC8 is setupLightColor, not setupLight;0601DF8A restores the falloff
colors after normal dragon drawing. These identities are pinned in the fixture.

Original CAMP instruction/literal checks and2031616 signed source/target/tick
cases pass, plus delayed highlight/restoration, equal-delay ordering, native
zero-duration divider binding and wrapping camera endpoints. These helpers are
NOT yet wired into the live dragon: the particle factory0605669C and its
Update060569EC/Draw060570F0 (definition0607B270) still require complete recovery
before an atomic runtime cutover. They must not be replaced with the superficially
similar battle heal task without instruction-level semantic comparison.
No new VPK/hardware run is requested for this unused-helper checkpoint; C1
remains open. The next implementation step is the original particle lifecycle,
then typed callback integration and the full recovery-to-control regression.

## Next goal markers - 2026-11-05

All markers are open. Implement in this priority order, preserving accepted
WORLD transitions, the comparison build profile and the33ms frame deadline.
Each subsystem uses original native producers/services, lands in atomic,
buildable commits, and needs hardware acceptance at a coherent checkpoint.

| Marker | Outcome | Completion evidence |
|---|---|---|
| C1 - Camp recovery | Complete dragon healing animation, original lighting/particles, camera return and script-driven Edge control | Damaged HP/BP and already-full entry behave correctly against Saturn; no forced wait bypass |
| C2 - Camp functionality | Remaining actor/segmented-child chain, petting/feeding, save interaction, exits and repeated entry | Native lifecycle/service tests plus playable interactions, save/load and re-entry on hardware |
| R1 - Rider gun | Edge holds the correct weapon in flight and combat through native resource/hotpoint/model submission | Correct animated hand attachment, texture and visibility in both modes on hardware |
| B1 - Bird/nest progression | Returning to the first area enables the authored bird target and route to D Unit01 | Real story state and target lifecycle permit selection/nest travel and item acquisition; no forced flag |

Host fixtures and packaging are prerequisite checks, not substitutes for these
visual/behavioral gates. Keep unrelated runtime work out of each atomic fix.

## Current priority: native dragon recovery and control release

User confirms WORLD transition and Camp loading now work at the newest branch
checkpoint52f9ebf. Camp functionality is the priority; do not continue treating
the old camp startup crash or WORLD transition as the active blocker.

Saturn reference:20261105-2059-34.1759502.mp4,18.17seconds. The sequence shows
the recovering text, dragon rising/animation, white lighting with blue particles,
settling/restored color, camera return and eventual Edge control. It is a
reference capture, not evidence that the current Vita implementation passes.

Concrete source gap:06056484 recovers HP/BP and starts dragon substate10 when
needed. The native mode dispatch enters06055734, advancing to11, then
060557BC. That continuation remains Unimplemented in campDragon.cpp under the
misleading translated name sCampDragon_handlePetting. Thus bound script calls
cannot finish recovery:060564B4 waits for substate<10. Do not bypass this wait,
force substate0, replace it with a timer, or substitute the battle heal effect.

First coherent implementation milestone must recover the whole chain:

-06055734/060557BC: authored camera interpolation, animation-state10 wait,
  sound6F, command sequence0607AF5E, effect parameters/table0607B052 and native
  return to substate0 with the original Draw callback restored.
-060555E2/06055634: native lighting-transition setup, delay/counts, signed-byte
  color interpolation and normal dragon draw. The current tail fields are
  misleadingly named; adapt their typed semantics rather than raw task offsets.
-0605669C and its child effect lifecycle: original particles/resources,
  transforms, timing, release and rendering boundaries.
-Let the original script poll, camera restoration and player-control services
  finish naturally. Verify both damaged HP/BP and already-full entry paths.

After recovery, finish the remaining Camp actor/segmented-child callback gap
below and audit petting/feeding continuations (060559A4 is also a stub), saving,
normal exits and re-entry. Visual correctness and performance remain acceptance
requirements; preserve the existing WORLD comparison build profile.

Two separate user reports remain tracked, without forced fixes:

-Rider gun missing in flight AND combat. Native fieldDragon.cpp and
  battleDragon.cpp already call addObjectToDrawList for the rider bundle's
  weapon slot. Trace loadRider weapon index, hand hotpoint data, captured model
  transform/material and actual submission; do not append a guessed gun mesh.
-First-area bird not targetable on return. Both generated A3 bird guards still
  use the original byte0A mask02/bit6 correction, not upstream's incorrect bit1.
  Trace the live captain/progression flag and target disable/registration state.
  A source guard being corrected does not establish that the user can target
  the bird. Do not force the story bit or conflate the nest exit-zone gate.

Camp callback coverage remains incomplete. The current registration and save
fixes are checkpoints within this audit.

Sixteen additional script callbacks are restored in this checkpoint, covering
dragon feeding/rewards, player animation banks, NPC turning/animation/flags,
random choice and the direct camera restore entry. Their source-level binding
checks and production fixtures pass. All 55 inventoried script targets now have bindings. One NPC task
definition below remains pending. NPC receivers must be implemented before the
connected script services can establish a playable scene.

## Native script inventory

Read-only inspection uses US Disc 1 TWN_CAMP.PRG at LBA239428, base06054000.
The provisional static walk starts at the distinct script roots referenced by
all nine setup records and adds object interaction scripts reached by native
object registration. It follows conditional branches, script calls, switch
entries and native-call operands across story alternatives. Current inventory
is55 distinct native function/arity pairs. Bound means a generated dispatch
entry exists. Semantic parity and hardware acceptance remain separate gates.
Opcode38 is a no-operand Yes/No dialog, recovered from MAIN600D660 and task
factory601C6CA. Its runtime handler and original dialog task are now restored. The production fixture checks all 512 choice/input combinations, original COMMON text, cursor animation, result inversion, display-task handoff and owner cleanup. Vita compile/link/SELF/VPK passes; hardware acceptance remains open. Table-default handling and all later-game paths still need
independent native instruction validation before this becomes a completion gate.

| Native function | Arguments | Dispatch | Call sites | First call site |
|---|---|---|---|---|
| 0600CC78 | 2 | Bound | 1 | 0605807D |
| 0600CCB4 | 1 | Bound | 9 | 06065B80 |
| 0600CDD4 | 0 | Bound | 10 | 060580F9 |
| 06014DF2 | 1 | Bound | 90 | 06058140 |
| 06026DA4 | 0 | Bound | 1 | 06060487 |
| 06027110 | 1 | Bound | 2 | 0605A259 |
| 0602C2CA | 1 | Bound | 9 | 060591CD |
| 0602C32A | 1 | Bound | 8 | 06065C24 |
| 0603011E | 0 | Bound | 1 | 06060361 |
| 060541C4 | 1 | Bound | 9 | 06065B8C |
| 06056408 | 1 | Bound | 1 | 06060A3C |
| 0605641C | 1 | Bound | 1 | 06060A4C |
| 06056484 | 1 | Bound | 1 | 060580A4 |
| 060564B4 | 1 | Bound | 1 | 060580C4 |
| 060564D6 | 0 | Bound | 1 | 06060998 |
| 060564FE | 0 | Bound | 1 | 06060980 |
| 06056526 | 1 | Bound | 5 | 06060BE4 |
| 06056544 | 1 | Bound | 5 | 06060B70 |
| 06056562 | 1 | Bound | 1 | 06060A93 |
| 06056574 | 1 | Bound | 5 | 06060BA8 |
| 06056588 | 2 | Bound | 4 | 06060B2C |
| 0605664A | 1 | Bound | 2 | 06060A18 |
| 06056662 | 1 | Bound | 2 | 06060A28 |
| 06057B62 | 1 | Bound | 1 | 060604F0 |
| 06057B76 | 1 | Bound | 1 | 06060514 |
| 06057F46 | 0 | Bound | 1 | 06060AD8 |
| 0606AECC | 0 | Bound | 1 | 060580DA |
| 0606C70C | 1 | Bound | 9 | 06065BC8 |
| 0606C952 | 0 | Bound | 10 | 06058608 |
| 0606C95C | 0 | Bound | 10 | 0605861C |
| 0606CB54 | 0 | Bound | 1 | 0606EB74 |
| 0606CC0E | 0 | Bound | 3 | 060587BD |
| 0606D416 | 0 | Bound | 1 | 0606EB88 |
| 0606D542 | 1 | Bound | 38 | 060584B8 |
| 0606D580 | 0 | Bound | 1 | 0606EB9C |
| 0606D5BC | 0 | Bound | 3 | 060603B1 |
| 0606D5F2 | 0 | Bound | 13 | 06060431 |
| 0606D608 | 1 | Bound | 5 | 060609E0 |
| 0606D790 | 1 | Bound | 2 | 06065CEC |
| 06070198 | 3 | Bound | 13 | 06058498 |
| 06070268 | 2 | Bound | 5 | 060586F1 |
| 060704C4 | 4 | Bound | 9 | 06065B98 |
| 060704F2 | 4 | Bound | 9 | 06065BB0 |
| 06070560 | 3 | Bound | 80 | 060584C4 |
| 06070664 | 3 | Bound | 48 | 060584F8 |
| 0607091E | 1 | Bound | 1 | 0605C8EB |
| 06070932 | 1 | Bound | 1 | 0605CCC0 |
| 06070946 | 1 | Bound | 22 | 060584AC |
| 06071A78 | 0 | Bound | 4 | 06058FBD |
| 06071A90 | 0 | Bound | 5 | 06059320 |
| 06071ADE | 0 | Bound | 1 | 0606038C |
| 06071B40 | 2 | Bound | 9 | 06065C30 |
| 06071CA4 | 1 | Bound | 9 | 06065C10 |
| 06071DA8 | 1 | Bound | 1 | 06058094 |
| 06071E20 | 1 | Bound | 13 | 06057FC9 |

## Missing task dispatch

Five original object definitions now have factories in Camp. The interactive
actor definition0607B5BC is restored with Init06057714, resource wait06057A68,
Update06057818, Draw0605799E and Delete060579F0. Its two collision scripts,
seven model variants, PCM lifecycle and relative palette-sequence loops are
connected.06057B62 queries state3;06057B76 switches to state4 or1 and returns
the previous state. Production lifecycle and native-anchor checks pass, as
does Vita compile/link/SELF/VPK. Rendering and gameplay still need device checks.

One definition reached through object registration remains absent.

| Definition | Init | Update | Draw | Delete | Native size |
|---|---|---|---|---|---|
| 06080148 | 0606F3F0 | 0606F530 | 0 | 0606FF04 | 17C |

These require recovery or verified reuse of matching task implementations.
Common NPC movement/animation callbacks must be checked against their concrete
receiver types and registration lifecycle, including Edge and the Camp dragon.

## Remaining acceptance

- Validate native-call semantics together with concrete actor lifecycles.
- Recover remaining06080148 NPC task and its child callbacks. Native factory
  0606ABE8 creates a34-byte segmented-body task from0607EB94. Its init/update/draw
  entries are0606AA04/0606AB88/0606ABD2; parent bit40 requires this path.
  Existing shared NPC translation leaves it unimplemented, so binding the class
  alone would not complete the callback chain.
- Validate remaining static coverage against native dispatch. Runtime opcode38 is restored and covered by its production fixture.
- Exercise startup, dragon recovery/interactions, other characters, save station,
  save/load return state, normal exits and repeated Camp entry.
- Preserve the tested VPK, unstripped lagi and lagi.velf together.

WORLD sky mapping is confirmed fixed by the user. Camp remains unaccepted.
