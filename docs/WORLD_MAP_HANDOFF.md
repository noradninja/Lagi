# Native WORLD map bring-up

## Active completion goal — 2026-11-03

Finish native WORLD entry, navigation, confirmation, travel and exit/re-entry so
normal progression can reach Garil Desert. Work stays on
`feature/light-state-index` in small atomic commits. Preserve native story gates,
destination records, task/resource ownership and GXM presentation; do not force
an unlocked destination or substitute a field viewer. Trace the actual destination
status/COMMON/overlay chain before declaring the Garil route connected.

Implementation and acceptance gates:

1. Recover WORLD's missing environment motion and its live native state, keeping
   normal-background origins separate from rotation-coefficient wave modulation.
   Replace the documented interim arrival phase and fixed-row name behavior
   with their original producers rather than treating the experimental path as final.
2. Check entry from both Map and E014, selection/locked destinations, confirmation,
   native arrival/fades, cancellation where authored, teardown and re-entry. Ask for
   hardware testing when this forms a coherent visual/behavioral milestone.
3. Follow the original unlocked progression into the next fields toward Garil:
   verify destination dispatch, required overlay services/assets, player control,
   and the return route to WORLD. Keep unsupported downstream services explicit.
4. Validate the complete route on Vita, including repeated WORLD visits, resources,
   save/load as applicable, and render timing. Target 22 ms with a 33 ms deadline;
   host fixtures/builds alone cannot close this acceptance gate.

Baseline audit: the existing production display/motion and model fixtures pass
against the original Disc 1 image. The boundary fixture now covers all four
destination types, disabled status, story rejection, arrival readiness, fade wait,
existing pending-status protection and startup/resource cleanup. Native camera,
arrival and rendering services remain mocked there: this is regression coverage,
not proof of complete arrival semantics or rendered output. No new hardware
test or VPK is claimed by this test-only checkpoint.

## Resume here

### Garil authored placement / direct camera reset — 2026-11-04

A5-owned `0605C020/54` actions now read their original position/rotation
literals from FLD_A5.PRG and port `0607C584`'s state changes: dragon position
and angles, rider rotation vector at +18 when present, native rotation-matrix
producer and the active camera's yaw. Then `06069AE8` resets follow-camera
slot zero, which is not necessarily the active slot. The existing native
altitude/config producer is exposed through a guarded generated-source wrapper;
the reset copies its distance, normalizes and bounds pitch, and computes the
fixed-point camera offset/position. It does not run the ordinary spring update,
clear impulses, blend a camera or substitute a guessed view.

MAIN `0600AE68` also invalidates a live light-wing task during placement.
That factory is still unported. A non-null light-wing task therefore declines
the binding **before any state mutation**, rather than silently skipping its
required effect; this path is not claimed supported. The normal early-game
null-light-wing path is implemented. No animation-table rebinding or upstream
edits are made.

Original instruction/literal checks and production fixtures cover both authored
locations, null/foreign owner guards, unsupported-effect isolation, rider vector
bounds, matrix/camera order, distinct active/slot-zero cameras, pitch clamps,
distance and preservation of nonzero springs/impulses. Native altitude/trig
services are mocked. Vita compile/link and SELF/VPK packaging pass. This is not
on-device camera or Garil entry acceptance.

This supersedes the placement-pending note below for the early-game path.
Seven native addresses / 19 sites in the partial structural audit now have
owned bindings. Scheduling, hide/light-wing support, undecoded script paths,
WORLD hardware acceptance, and actual Garil entry/control/return remain open.

### Garil native cutscene-state actions — 2026-11-04

The zero-argument shared VM boundary now resolves three additional actions
only for the actual A5 file owner: `060728E0` clears script +5C, `060728F8`
sets it to 1, and `060728A4` completes the active cutscene only when the field
and child exist and +5C is zero. All eleven recovered sites use ordinary
opcode 14. Foreign files, unsupported addresses and the shared animation-data
binding are unchanged.

Completion follows original `06071A36`: mark the child finished, clear the
script child and +5C, then preserve camera tracking when +78 is nonzero;
otherwise choose the native restore/no-restore camera service from child flag
2. The native first flags word belongs to distinct translated C++ task types.
The adapter uses the actual `s_cutsceneTask`, `s_cutsceneTask2` or
`s_cutsceneTask3` type, not the upstream unconditional task-2 cast. An unknown
child type leaves state untouched and declines the binding rather than
reporting successful completion.

Original instruction checks and production fixtures cover both state writes,
guarded completion, typed retirement, tracking preservation, both restore
branches, foreign dispatch and unknown-child isolation. Native camera/fade
services are mocked in these fixtures. Vita compile/link and SELF/VPK pass;
live A5 cutscene behavior is not hardware-accepted.

This supersedes the pending list below for these three addresses. Five of the
ten addresses in the partial structural inventory now have A5-owned bindings
(17 of 28 sites); four undecoded paths still make that inventory incomplete.
Placement actions `0605C020/54` were inspected but deliberately not bound:
their native placement/rotation helper also updates rider state, the light-wing
task and the direct camera reset. Existing translated helper stubs are not
proof of those semantics. Scheduling, hide/light-wing, placement/camera and
undecoded script paths still require recovery, followed by actual Garil
entry/control/return testing.

### Owned Garil script fade actions — 2026-11-04

The shared field VM previously dispatched all A5 native calls through the
inherited A3 owner. The generated one-argument boundary now first resolves the
two recovered A5 fade actions only when the target's actual file owner is
`gFLD_A5`: out `0606FA5C`, in `0606FAB4`. Foreign files, unrelated addresses
and the zero-argument dispatcher are unchanged. No speculative global overlay
or dragon-animation table rebinding is performed, and `extern/Azel` is intact.

Both native functions compare sign-extended fade bytes, set both CLOFSL buffers
to 0010 when that guard passes, and operate on both fade channels with the
low-32-bit doubled authored duration. Fade out starts at each channel's own
current converted color; fade in starts black and targets signed native +48.
The six recovered call sites are ordinary opcode-14 actions, not polling calls.
Production fixtures cover ownership isolation, both channels/order/colors,
signed-byte boundary cases and duration wrapping. Original A5 instructions and
all six sites are checked read-only. Vita compile/link and SELF/VPK pass.

`tools/audit_garil_scripts.ps1 -DiscImage <Disc1.bin>` structurally traverses
the original 39-entry A5 table with explicit operands, branches, call
continuations and bounds. It is an audit tool, not a scanner in the runtime or
a replacement interpreter. The current traversal visits 1457 instructions,
finds 28 native-call sites at ten addresses, and stops four paths it cannot
decode: entries `06061897`, `060618C5`, `0606190D` begin with 7F; `060619CC`
uses opcode 30. Therefore this is an incomplete inventory, not a claim that
these are every possible A5 native call or that those entrypoints can safely
be skipped.

Other recovered callees still requiring bindings/recovery are `06054DCC`
(native scheduling through `060703E4`), `0605C020/54` (authored dragon position
and camera reset), `060728A4` (guarded cutscene completion), `060728E0/F8`
(script +5C state), and `0607C82A/86C` (dragon hide/collision plus light-wing
Draw callback control). Do not redirect them to Canyon addresses by numerical
similarity or treat unimplemented native services as successful. Garil control,
return/re-entry and all on-device WORLD acceptance remain outstanding.

### WORLD confirmation evidence / Garil entry audit — 2026-11-04

Locked WORLD confirmation now plays native sound 5 (`06058044..48`) instead
of silently ignoring the press. The same original availability guard remains
authoritative. Event-only `[WorldMapTravel]` lines record the pressed selection,
status/type/story gate, unlocked/readiness result, preparation completion and
arrival start before the existing final dispatch event. They do not traverse
additional geometry, run per frame, alter story state or add controls. This
allows one next capture to distinguish rejection, failed preparation, arrival
and final module handoff rather than assuming every stationary selection is
a renderer problem.

Garil data audit also confirms original Disc 1 FLD_A5.PRG at LBA244678,
316576 bytes. Entry `060540AC` supplies its own script table `0609D4A8`;
subfield-0 script 19hex points to `06061955`, a duration-60 location-name
display (opcode 27, string `060655C9`) followed by END. The route fixture now
checks that evidence in addition to WORLD/COMMON routing. It does not execute
A5 or establish playable entry. The compiled A5 backend still has unimplemented
services, and its `gCurrentFieldOverlay = gFLD_A3` / shared animation-table
bindings require recovery rather than a speculative pointer swap. Do not
report the linked backend as complete Garil gameplay.

Boundary tests cover locked sound and unchanged preparation/fade guards;
native instruction and Garil-entry data checks pass. Device acceptance remains
outstanding for the complete accumulated WORLD checkpoint.
Vita executable compile/link and SELF/VPK packaging pass; the travel events
are intended for that next device capture, not claimed device evidence.

### Native WORLD shoulder-button orbit — 2026-11-04

Camera Update `06057DFA..06057E1A` reads the new-button word before camera
state dispatch. Mask 8000 increments display +20C; mask 0800 decrements it.
These are separate signed-byte stores, not held-button motion or a reset on
release. The boundary omitted both operations, leaving the orbit controller
permanently at zero. It now forwards the native button word every update.
The existing display states retain their original -3..3 clamp, 3A41 velocity
acceleration, and 369D0 control gain; entry/return states still reset orbit.
No new mapping, camera formula, or debug control is introduced.

Original-disc checks cover both masks, byte stores and increment/decrement
instructions. Production fixtures cover release, both presses, unrelated
buttons, signed-byte wrapping, and positive/negative clamp/acceleration.
Host checks do not prove native camera feel on hardware.
Display/motion and boundary fixtures pass; Vita executable compile/link and
SELF/VPK packaging pass. This build includes the preceding water-composition,
angle-unit and message-controller corrections, all awaiting device acceptance.

### Native WORLD destination-message cutover — 2026-11-04

The live boundary now uses the previously recovered projection and VDP1
sprite-text services through WORLD's seven-state message controller
(`06057708`), replacing `DestinationName` / `UpdateDestinationName` and the
fixed-row destination fallback completely. The native text window is set once
after loading WORLDMAP.FNT. Location changes retire the old child, clear the
ordinary notice area, retain the selected record's anchor and create its text
child. The original 0085 story condition selects the Stronghold/Camp name;
locked destinations retain the native notice text. No story gate is changed.

The live camera is applied before projecting the anchor. Confirmation captures
that placement and runs `060575D0`'s fixed-point factor/centering calculation
against the original WORLD vector `06059C14` (0,80), preserving raw depth 0333.
Message requests use the original active/queued-mode rule. Map-table re-entry
resets mode 3 directly like the native camera initialization. Owned children are
retired on label replacement; normal parent teardown uses the native task
walker's recursive child deletion rather than dereferencing a retired child.

Original-disc instruction/target checks, 660 label bounds cases, production
message-state fixtures and the boundary camera-before-message binding pass.
Vita compile/link and SELF/VPK packaging pass. Native services remain mocked in
host fixtures: live text pixels, placement, confirmation and repeated re-entry
still need hardware acceptance. This supersedes earlier notes saying the
sprite-text and projection services are not connected; WORLD completion and
Garil progression are still unproven.

### WORLD matrix angle-unit correction — 2026-11-04

Original MAIN `06035E68` begins `SHLR16 R4`, then masks the 12-bit trigonometric
index. Its current-matrix wrapper `06035E64` falls through to that instruction.
WORLD orbit (`060549F0`), arrival displacement (`06055554`), and marker/trail
draw (`06055238`) reference these shifted-angle services. The adapter instead
called integer-angle `rotateMatrixY` / `rotateCurrentMatrixY` with the full
16.16 value. That discarded the meaningful heading bits and used fractional
bits as an angle, affecting orbit, travel displacement, and marker/trail spin.

All four sites now call Azel's existing shifted-angle wrappers. No camera
formula, winding, renderer, destination gate, or speed has been changed.
The production fixtures previously ignored matrix rotation; they now exercise
nonzero quarter-turn orbit and positive/negative arrival displacement, and
check marker/trail shifted-service binding. Original-disc assertions verify
the service instructions and WORLD call-table literals. Those fixtures pass;
hardware camera/animation/travel acceptance is still outstanding.
Vita compile/link and SELF/VPK packaging also pass with the corrected water
composition profile. This is a coherent hardware checkpoint, not proof that
WORLD navigation or the Garil transition is complete.

### Water-only capture / build profile correction — 2026-11-04

The replacement 30.297-second capture is water-only, not a boss/WORLD test.
Its sampled FPS remains 30. Deep Gulch records `CCCTL=0043 SPCTL=1323
BGON=001A`, matching the accepted water composition state. However, the new
`build/world-garil` cache had `LAGI_VDP2_SPRITE_COMPOSITE=OFF`, whereas the
accepted water log records `[VDP2SpriteComposite] active=1`. The earlier WORLD
build instructions omitted this option. This is a concrete configuration
regression, not evidence requiring new water geometry or guessed blending.

Apply the explicit profile when configuring this device-test build:

```powershell
$env:VITASDK='E:\dev\VitaSDK'
$env:PSP2CGC='E:\PSVITA\sdk\host_tools\bin\psp2cgc.exe'
cmake -S . -B build/world-garil -G Ninja -C cmake/world_garil_profile.cmake
cmake --build build/world-garil --parallel 8
```

The profile repairs the compositor setting even in an existing cache, enables
the native WORLD/battle route, and disables serialized GPU timing/motion tracing.
It deliberately does not change the experimental bridge/page/coefficient options.
Startup now reports `spriteComposite` and `decodedPages` alongside the existing
build settings. Water restoration still requires hardware confirmation; this
capture does not answer the boss-cloud or WORLD failures below.

Validation: configuration and generated compiler definitions both select
`LAGI_VDP2_SPRITE_COMPOSITE=1`; the Vita executable links and SELF/VPK packaging
passes. No post-correction hardware result is claimed.

### Hardware regression report / fade-group correction — 2026-11-04

Hardware acceptance **failed**: user reports missing canyon water, cinematic
cloud slowdown, black WORLD environment, wrong camera/indicator behavior,
missing WORLD clouds and no destination transition. Do not treat the earlier
host fixtures/link results as acceptance of any of these behaviors. Label
controller work is deferred while these failures are investigated.

The available `lagi_InGame.log` proves WORLD reaches mode 6, submits 63 native
models / 263 polygons, and enables RBG0/NBG0. It also records `CLOFSL=0010`
while A fades to `(0,0,0)` and B remains `(-256,-256,-256)`. Thus the RBG0
terrain retains fully black offset B despite native submissions. The adapter
omitted WORLD camera state 1's original signed `4C <= 4D` test and clearing of
**both** CLOFSL buffers. The same operation belongs to negative-target return.
`BeginSelectionFadeIn` now restores that native operation at both entry sites;
it does not fabricate geometry, adjust the camera, or disable fade processing.

Original Disc 1 `06057ED0..EE4` anchors and host checks for both buffers and
signed-byte boundary cases pass; Vita executable compile/link passes. Visibility
is not yet hardware-verified, and this does not claim to fix the other failures.
WORLD samples in the available log take about 33–34 ms, with roughly 26–28 ms
in GXM wait and about 6 ms CPU preparation (`gpuSerialized=0`). This is not
evidence that clouds alone cause the cost.

Capture availability at inspection: the requested first Downloads `lagi.log`
is absent. The supplied `Lagi - Lagi.mp4` is 50.048 seconds and starts directly
in WORLD; its opening nine sampled seconds also remain in WORLD, not the
described canyon/cinematic/boss route. `Lagi - InGame.mp4` is 55.573 seconds,
showing flight/menu/WORLD. The 12.367-second Saturn reference shows terrain,
clouds, normal selection and travel. Obtain the intended first log/video before
claiming a canyon-water or boss-cinematic timing diagnosis. Do not repeatedly
request hardware runs for each narrow source correction.

### Native sprite-text consumer — 2026-11-03 (controller cutover still pending)

`lagi_world_map_sprite_text.cpp` ports MAIN `060263D4` creation and `06026316`
Draw as a parent-owned typed task. It uses the existing native text parser and
active font in offscreen row 32, reads cells at VDP2 `7000`/`7080`, strips the
original 12-bit character index, and copies each nonempty glyph's two 32-byte
tiles into WORLD's reserved VDP1 range `7B600..7BFFF`. Blank cells keep their
8-pixel advance without consuming character memory. It clears the scratch
row and restores the same six cursor/window fields saved by the original.
Vita texture invalidation occurs once for the written range, not each frame.

Draw follows the live parent-owned placement and writes original normal-sprite
attributes: CTRL `1000`, PMOD `0080`, COLR `27C0`, SIZE `0110` (8x16).
X adds the glyph advance; Y negates placement plus glyph Y. MAIN `06026382`
multiplies placement's third component by the native one-over-far-clip input
for the packet depth key: that component is **not font scale**. Both context
command and packet cursors/counters advance through the normal native path.

`world_map_sprite_text_test.ps1 -DiscImage <Disc1.bin>` checks original task
and draw/glyph instruction anchors, then the production task with mocked
native services: glyph order, blanks, bitmap bytes, cursor restoration,
palette/size, live placement/depth, full 40-cell range bounds and task release.
Tests pass, and the actual Vita executable compiles/links. Font decoding and
GPU appearance are not proven by those mocks.

This consumer is not live yet: the native WORLD message/confirmation controller
must create/delete these children, update their placement, and perform the
authored center interpolation before the fixed-row implementation is retired.
No new hardware run is needed solely for this inactive consumer checkpoint.

### Native label projection producer — 2026-11-03 (not a live text cutover)

`lagi_world_map_label.cpp` now ports WORLD `060574E4`'s projection through
the existing native current-matrix transform, fixed divider and three-factor
multiply services. The bounds helper subtracts the signed string half-width
and Y offset 3 with low-word wrapping, rejects coordinates outside +/-512
to native offscreen position `(0,-256)`, clamps X sequentially to -160 and
`160 - 2*halfWidth`, and clamps Y to -88..104. Original `MOV #A8` is signed
**-88**, not positive 168. The third task coordinate remains native `0333`;
do not reinterpret it as font scale without tracing its consumer.

Disc 1 instructions, projection-service literals and the label-task creation
literals are pinned in `world_map_label_test.ps1`. The production producer
passes native-service binding checks and 660 independent bounds comparisons;
Vita compile/link passes. This helper is deliberately **not live** yet.
WORLD's current fixed VDP2 row remains until the real consumer is ready.
The authored label creation calls MAIN `060263D4` with VDP1 character memory
`25C7B600`, not `createDisplayFormationNameText`'s VDP2 blue-box task. Port
that sprite-text task and its glyph/resource lifetime before cutting over;
do not substitute a moving VDP2 row or a generic UI font.

No new hardware test is needed for this producer-only checkpoint; the prior
arrival/transition hardware checkpoint is still outstanding. No performance
claim is made without a fresh device log.

### Native arrival dragon animation/draw — 2026-11-03

The WORLD-local arrival child now uses the resident native dragon, loading it
only if absent as the original root does. Init invokes the existing
`setupDragonForTown`, animation zero from that dragon's own offset table/bundle,
`setupModelAnimation`, and the native runtime-delta reset. Update preserves
the separate child curve-frame counter (COMMON `206AC4`, 20-byte dragon rows),
then calls `stepAnimation` and `updateAnimationMatrices` before yaw and motion.
The normal retail curve selector remains zero; no debug curve editor was added.

Draw uses `submitModelAndShadowModelToRendering` with the native main/shadow
indices, the live child position/rotation and shadow height zero. Original
WORLD `060556DA` suppresses it when the display camera Y is greater than
`64000`; the adapter retains that gate, including equality. It does not hide
the dragon simply because movement has not yet begun. No rider or replacement
static model is appended to the map mesh.

`world_map_arrival_test.ps1 -DiscImage <Disc1.bin>` pins original instructions
and service literals, then compiles the production adapter with mocked native
services to check selection, call order, curve cycling and drawing parameters.
The boundary test checks animation-before-heading-before-movement. Both pass;
the actual Vita `build/world-garil` executable target compiles and links.
These are not device animation/texture/performance acceptance.

This is a meaningful hardware checkpoint: build the configured experimental
WORLD profile, enter through Map (and E014 if available), inspect the moving
dragon/shadow, select an ordinarily unlocked destination and return to WORLD.
Try Garil only when the real story gate exposes it; do not edit unlock bits.
Capture the current build identity and `lagi.log`, including selected/status
dispatch and frame timing. Native label placement, fleet-preview control and
the complete Garil field route still remain open. The goal stays active.

### Arrival live-yaw correction — 2026-11-03

Arrival now owns its original rotation vector, initialized with yaw `238E38E`.
WORLD `060555D0` holds that heading inside distance `1000`; `060555E2` eases
it toward the desired yaw through Azel's existing `interpolateRotation`, using
gain `1999`, maximum `38E38E`, and minimum `12345`. This runs in the idle
arrival phase too. Active movement now rotates by the child's **live eased
yaw**, not the desired destination angle. This supersedes the direct-heading
integration in the earlier arrival checkpoint.

Original instructions/literals are pinned in the display evidence fixture.
Host checks verify initialization, hold threshold, exact native service inputs,
readiness gating and live-yaw movement wiring. The actual rotation service is
mocked in those host fixtures; Vita compile/link and hardware behavior are
separate checks. Native arrival animation and its model/shadow draw remain
under recovery, not replaced by a static display model.

### Ordinary/fleet map switching — 2026-11-03

The negative-status authored destination now follows native camera state FF:
clear selection text, confirmation sound, 30-update fade to zero, wait for the
fade to stop, then invoke WORLD `06057CE0` and return through display state 4.
It never prepares files or dispatches a negative game status. Confirmation and
navigation are suppressed while that fade is pending, and return restores the
normal fade/15-update input delay.

The original story gate is byte `1A` mask `02`. The switch toggles only the
authored ordinary/fleet mode bit (`1B:08`), switches the original destination
table/count, and preserves ordinary selection in packed `DE`. Leaving the
fleet table restores that selection and packed route `87`. Native marker
color flag `40` now also follows fleet mode on initial WORLD entry. No story
unlock is fabricated. The display fixture checks the real service in both
directions and the no-gate case; original Disc 1 instructions/table literals
are pinned alongside it. The boundary fixture covers the negative-status
fade wait, input suppression, re-entry delay, and absence of module dispatch.

Host fixtures pass. Hardware visual/behavioral acceptance remains pending;
fleet advance preview/dragon tasks, native labels, and the complete Garil
entry/return route are not claimed complete by this checkpoint.

### Native destination file preparation — 2026-11-03

WORLD confirmation now calls the recovered field/town preparation boundary
before starting arrival or its close-up delay. Native signed preload selectors,
COMMON field definitions (font then PRG), and town-pair/overlay records select
the filenames. Preparation changes no story flags, current field, subfield,
or overlay dispatch pointer. A failed preparation leaves selection active.

`lagi_world_map_travel.cpp` delegates those requests to the Vita file service.
Since the service is synchronous and programs have independent host ownership,
it retains the exact prepared bytes instead of overwriting the live WORLD
program at Saturn `06054000`. Normal `loadFile`/`sSaturnMemoryFile` consumers
take a matching disc/name entry once; preparation alone does not report a
completed destination copy. Failed transactions retain no partial files.
WORLD clears preparation on entry and cancellation/teardown, but transfers
ownership after issuing its native next-status request. A disc change
invalidates retained bytes. There is no per-frame read or background worker.

`world_map_travel_test.ps1` checks native filename selection/order and cache
ownership. `disc_file_preparation_test.ps1` compiles the actual file adapter
and checks font/program consumption, failure, release and completion semantics.
The boundary fixture covers preparation failure and handoff teardown for all
four destination types. These pass, and `build/world-garil` target `lagi`
compiles/links. No SELF/VPK or hardware acceptance is claimed. Native child
visual/animation, name placement, navigation/cancel variants and live Garil
entry/control/return remain open; the goal is not complete.

### Native Garil backend build checkpoint — 2026-11-03

The experimental WORLD/A3-execution configuration now restores the original
`overlayStart_FLD_A5` dispatcher and all 28 native A5 translation units. The
native shared `fieldModelRender.cpp` model/LCS context and
`fieldAnimRingSubTask.cpp` exit animation service are included as dependencies;
no replacement stubs or upstream edits were introduced.

The isolated `build/world-garil` Release executable target `lagi` compiled and
linked successfully. Its ELF contains the A5 entry point and both shared native
services. `tests/world_a5_build_contract_test.ps1` passes for WORLD enabled and
for each disabled WORLD/execution gate; disabled configurations select no A5
modules or dispatcher. The original-disc Garil route fixture also passes.

This supersedes the source-exclusion finding below, not the remaining travel
work. No SELF/VPK, deployment, or hardware acceptance is claimed. Native WORLD
destination preparation, label/child presentation, navigation/cancellation,
field entry/control and return/re-entry still require completion or validation.

### Garil route / downstream backend audit — 2026-11-03

Original Disc 1 WORLD ordinary entry 6 is **Garil Desert**: story gate `006C`,
status `0056`, preload selector 3, type 0. COMMON `212EAC + 56*2` gives mode 3 /
field 3; native field definition `208C58 + 3*20` names `FLD_A5.PRG` and
`FLD_A5.FNT`. Do not confuse the WORLD 0x24-byte record with the 20-byte field
definition or select another desert overlay by filename resemblance.

`tests/world_map_garil_route_test.ps1 -DiscImage <Disc1.bin>` verifies this
complete read-only data chain, including the label, original gate and both asset
names. It passes. It does not change flags, assert that the destination is unlocked,
or prove live travel/player control.

At this audit checkpoint the Vita source selection removed `overlayStart_FLD_A5` from the field
dispatcher and does not compile `field/field_a5` modules. Thus WORLD dispatch
alone cannot finish this goal. Restore the native A5 backend and its shared
dependencies behind the experimental build boundary; do not reroute status 56
to A3, alter story availability, or label an unsupported destination as playable.

WORLD record +4 is now named `preloadIndex`: the original camera passes a
positive signed value to MAIN `06011E0A` for type 0 or `06030100` for types
1/2/3. Original MAIN instructions resolve both file-service calls to
`060100F2` (not the neighboring `0601016C` wrapper). Field preparation uses
20-byte records at COMMON `208C58`, optionally preparing the font at +8 with
destination zero, then the PRG at +4 into Saturn `06054000`, relocation zero.
Town preparation first reads the signed overlay index from the two-byte pair
at COMMON `2166E4`, then resolves the PRG from 16-byte records at `2165D8`,
also targeting `06054000`. The route fixture checks these instructions and
PC-relative literals. The retained-byte Vita preparation service above now
handles the host resource lifetime; native asynchronous GFS scheduling is not
claimed. These preparation calls must not be replaced
by `loadTownPrg` or forced field/subfield state changes merely because an older
upstream comment called them overlay loaders.

### 2026-11-03 arrival-motion checkpoint

The interim middle-distance `map_approach_profile` is removed. Original WORLD
`06055464..66` overwrites the local source Y with target Y before distance
calculation, so its `CCC0`/`6660` thresholds measure horizontal X/Z distance.
Heading uses target-minus-position. `060554A8` adds the rotated displacement;
the old port inverted both heading and integration direction. The recovered
motion service runs the same horizontal/vertical update in every distance band,
including the frame that sets child +3D. Once complete, target height changes
from the authored Y + `A000` to the authored Y, allowing descent during fade.

Boundary Update now evaluates camera/display against the previous child state,
then advances arrival after display and environment. Completion is consumed by
the camera/fade path on the following update. Arrival continues during the fade
until dispatch instead of stopping when its completion flag is consumed.

Original-disc distance/heading/addition instructions are checked alongside the
existing callback evidence. Production display tests exercise horizontal-only
distance, heading arguments, middle/near motion and the completion threshold.
The boundary fixture checks display-before-arrival ordering, next-update fade
and continued child motion during fade. Native matrix/atan/sine services remain
mocked in the host fixtures; both actual Vita sources pass syntax checking.
No package or hardware validation is claimed. Keep fixed-row name placement,
native child visual/animation behavior, navigation/cancel and destination
preparation/dispatch under audit before calling WORLD complete.

### 2026-11-03 live environment checkpoint

The experimental WORLD boundary now owns the native initial environment state:
plane height `1FB33`, scale `5B333`, vertical offset `013B`, drift speeds -409/-81
and the recovered coefficient-wave parameters. Update advances the two native
drift accumulators. Draw uses an environment-only camera copy, applying the
original `32 / scale` offsets to X and Z and subtracting their drift. The two
PC-relative loads both resolve to WORLD `060571C0`; the nearby -90-degree
literal belongs to rotation setup, not Z translation. An exactly zero pitch
uses the original environment fallback `-B60B6`.

After the rotation pass, Draw computes projected ground from the live matrix
and pivot through the native divider, applies the 424-entry wave to the selected
coefficient buffer, and writes native NBG0 origins and priority bands. The normal
environment control byte at +69 starts zero; WORLD's camera-height comparison
therefore chooses RBG priority 3 or 5. Desktop debug-keyboard mutations are not
introduced as Vita controls. No presentation/view mode was added.

This replaces the former constant `-81` plane-height argument, which was actually
the environment's Z drift speed, not its authored plane height. The display/game
camera is never modified by these texture-origin calculations.

Disc instruction/literal checks, 5,832 arithmetic comparisons, wave checks and
production-boundary tests pass. The latter executes actual Draw with mocked
matrix/sine/native services, checks both coefficient buffers, scroll/priority
writes and camera isolation. Vita source syntax passes. These are not rendered
pixel or full-package verification. Rebuild the experimental WORLD package before
hardware testing; no VPK was produced by this checkpoint. Check map entry, moving
background/marker placement through selection, close-up, and re-entry. Keep the
matching log. The later arrival-motion checkpoint above supersedes the interim
middle phase; fixed-row name placement remains open.

The older helper-only notes below describe earlier checkpoints; their live-hookup
limitations are superseded only by the environment integration described here.

The experimental WORLD scene is on `feature/light-state-index`. Both the Map
menu and the post-E014 cutscene request status `0x4F`, which enters mode 6.
The branch includes the original Disc 1 destination tables and assets,
marker, trails, camera, input, background passes and travel dispatch. The
experimental build also links `TWN_CAMP.PRG` for status `0x52`. WORLD now
initializes its own VDP2 state, clipping window, and projection. Disc 1
entry code at `06054016..54` sets near and far clip to `0x800` and
`0x400000` in 16.16 units.

Build in a separate directory with VitaSDK and `psp2cgc` available:

```sh
git submodule update --init
cmake -S . -B build-world -G Ninja -DCMAKE_BUILD_TYPE=Release -DLAGI_FULLRES=ON -DLAGI_BATTLE_NATIVE_BUILD=ON -DLAGI_BATTLE_EXECUTE_A3=ON -DLAGI_WORLD_MAP_BRINGUP=ON
cmake --build build-world --parallel 8
```

On the work PC the SDK paths are `C:/Dev/VitaSDK` and
`C:/Dev/sdk/host_tools/bin/psp2cgc.exe`. Set `VITASDK` and `PSP2CGC` for
the home machine before configuring. The mode 6 option defaults off.
Use the original Disc 1 CUE/BIN on Vita. The work PC input was
`Downloads/AZEL_1.cue` and `Downloads/AZEL_1.bin`. No disc data is checked in.

The Vita build compiles and links. Host checks passed for startup ownership,
display, menu dispatch, tile decoding, and mocked travel handoff. The enabled
startup fixture also verifies priority and clip setup and checks that a missing
background leaves VDP2 VRAM untouched. Hardware
verification remains pending. Test entry from the menu and after E014
separately. Verify background, marker, directional selection and the destination
label, then confirm an unlocked destination and verify control in the next
scene. Save `ux0:data/lagi/lagi.log` after each run. Look for
`[WorldMapStartup]` with `displaySeeded=1 sceneEnabled=1`, then
`[WorldMapTravel]` with the selected entry and requested status.
Use `Lagi-WORLD-branch-shaders.vpk` for the WORLD test. It uses the branch
shader flags and excludes the local water shader precision experiment.
SHA-256: `E8596B82026486E727EA2ED1F7140F1F099E823E0962E0C1AB7C49DF3682DE2D`. The earlier Camp VPK includes that experiment.

The destination label currently uses a fixed text row. The middle arrival
phase uses an interim approach toward the authored landing point. Verify
background composition and native arrival timing on Vita before enabling
WORLD by default. The sections below preserve earlier recovery checkpoints
and their limitations at the time they were written.

## 2026-10-20 display-controller checkpoint

The typed display controller now recovers native Update 06054F9E and its
retail helpers: all seven states, six fixed-point easing profiles, accelerated
springs, initial target positioning, marker arrival/visibility, camera orbit
and close-up transitions, the 16-entry trail ring, RGB-table blending, native
camera angles, directional light and depth falloff. No floating-point lerp or
replacement renderer is used. Original-disc checks verify the easing tables;
production-code fixtures exercise all states, selection changes, exact arrival,
travel-camera dependency, RGB endpoints, bounded trails and native service
calls. A separate signed-branch differential checks 400,000 easing cases.
Fixtures mock camera/matrix services; they do not prove final rendered pixels.
Native trail rendering 06055128 is also recovered: it walks the ring backward,
wrapping at 16, and submits original MCB slot 03DC through Azel's draw service.
Its fixture verifies three-point wrap, matrix push/pop balance and invalid-state
guards. Both experimental-battle and non-battle Vita configurations build.

**Important correction to the upstream dependency names:** native root Init
060540EA calls WORLD-local 0605571A (literal at 06054190), creating a 0x40-byte
child. It does not call the title `initLoadSavegameScreen` that upstream has
substituted. Display helper 060545F4 copies that child's live +8/+C/+10
position, not the current selected destination or an embedded Saturn pointer.
The new controller takes that position explicitly and refuses state 3 before
the native child is ready. The child's +3D controls marker shrink; it is named
`arrivalBusy`, not assumed to be a title save-screen flag.

Environment 06056928 is likewise an initial callback: it runs 06056658, then
installs Update 06056AD4 and Draw 06056DDE and sets ready bit 4. The latter
consumes the display's live camera/rotation and generates two native rotation
passes. Merely replacing its initial stub with a permanent Update function
would be incorrect. These environment and local-child routines are the next
recovery work, not an instruction to enable the placeholder graph.

The controller and camera/light application service compile in the Vita
experimental build, but are not wired to mode 6. The live map remains unready
for hardware testing. No new testing build is recommended at this checkpoint.

## Model setup checkpoint

`lagi_world_map_model.cpp` now ports WORLD's model initializer (06054504)
and renderer (060551AC) into typed host state. It reads the original map
hierarchy slot 01EC, static pose 05C8 and color animation 05D0 from
WORLDMAP.MCB. Visibility bit 20, translation/yaw/three-column scale order,
pose control and the four native RGB triplets are retained. No game data is
committed. `tests/world_map_model_test.ps1` checks the original-disc call
table/resource offsets and runs the actual production initializer/draw code
against mocked services. The full-resolution experimental Vita VPK builds.

This service is **not connected to mode 6 yet**. Its scale starts at zero and
visibility starts clear, exactly as native Init requires; WORLD Update owns
revealing and positioning it. Calling Draw alone is not map functionality.
Do not send this checkpoint for hardware map testing as though it were ready.

Remaining native recovery gates, in dependency order:

1. Connect the recovered display controller only after its native children
   and environment are available. 06054308's debug keyboard editor is excluded.
2. Environment initial callback 06056928, setup 06056658, Update 06056AD4 and
   Draw 06056DDE; control Init/Draw 06055740/06055924.
3. Camera task writes currently replaced by `Unimplemented()`, name task,
   destination confirmation, WORLD-local child 0605571A and native travel dispatch.
4. Owned root/overlay lifetime and shared mode-6 scene/view/composition gate.
5. Build and fixture checks, then Vita entry/selection/cancel/travel testing.

The existing black holding boundary remains deliberate until these gates
are satisfied; no stub task graph, fabricated destination screen or premature
travel path has been enabled.

## Original handoff evidence

The previous interpretation of status 0x4F as return-to-title was wrong.
Original-disc evidence:

- MAIN 06027C72..74 requests 0x4F after E014, as reconstructed.
- COMMON 00212F4A contains mode 6/subindex 0 for that status.
- COMMON dispatch slot 6 at 00212FDC points to MAIN 06027878.
- MAIN 06027880 loads the filename through 06027900 -> 0604382C: WORLD.PRG.
- Slot 5 instead points to 0602785A, which loads CHANGE.PRG. It is not WORLD.

The generated module dispatch now binds mode 6 in the experimental battle build
to a service that loads the selected native WORLD.PRG and returns a real live
child task. It does not change the story's requested status, fall through to
status zero, or reuse the module manager as its own child. This stops the
unsupported-slot -> intro loop. Other builds retain their disabled boundary.

**Display, navigation and travel are not implemented by this commit.** The
boundary intentionally remains at the outgoing black screen while the game
loop remains alive. Its one-time `[WorldMapStartup]` log explicitly says
`display/travel unported ... not a playable map`. This is not a substitute map,
temporary menu or claimed completion of world-map bring-up.

Enabling upstream `worldmap.cpp` wholesale is unsafe: Display Init/Update/Draw,
Env Update and Dragon Init/Draw are stubs; camera logic reads their absent state.
Upstream Root Init incorrectly substitutes the title save/load initializer,
which resets the same map allocator, for WORLD-local 0605571A. Its
`loadWorldOverlay` also returns its parent as the active overlay.
Those need native ports before enabling the map's task graph. The next work is
to recover WORLD 06054504/06054F9E/060551AC display state/rendering and its camera,
environment, dragon and resident-save child services; then expose mode 6 through
the shared scene bridge and verify navigation/travel on hardware.

Direct-disc tests verify the mode/entry/filename chain; the production boundary
fixture checks child ownership, data lifetime, missing-file handling and unchanged
status. Both Vita configurations compile. Neither check proves on-device map
rendering or even acceptance of this runtime handoff.

## WORLD-local arrival child verification

Disc 1 WORLD.PRG at LBA 244259 matches the recovered root call and
display easing table. The child factory at 0605571A passes size 0x40 and
the callback table at 060587FC to 06009BB6. The table contains Init
0605532A, Update 0605568C, Draw 060556DA and a null Delete. The Init
stores 1 at child +3C before returning. This is a separate WORLD task
with its own callbacks, so the display controller must receive the live
child state rather than a title save-screen handle or a static map target.

The menu's world confirmation and the post-E014 sequence both request
status 0x4F. The current menu checks the mode 6 dispatch slot, consistent
with Disc 1 COMMON.DAT. A non-null slot alone is insufficient to advertise
a playable map while the mode 6 presentation gate is closed.

WORLD.PRG names its environment resources separately from the model
bundle: FNS_WLD.SCB/PNB at 06059410/0605941C and FRS_WLD.SCB/PNB at
06059428/06059434. The camera text path names WORLDMAP.FNT at 06059C04.

The environment setup at 0605668E..AC loads FNS_WLD.SCB at VRAM
0x10000, FNS_WLD.PNB at 0x1E000, FRS_WLD.SCB at 0x40000, and
FRS_WLD.PNB at 0x60000. It passes configuration tables 06059390,
0605933C and 060593C4 to MAIN services 060326DC, 06033C20 and
0603420C respectively. The plane table at 060593D0 holds sixteen
absolute VRAM addresses, all in 25E60000..25E63800. The file copies
alone do not establish the rotation parameters or select the layers.

WORLD Draw 06056DDE copies the display's camera position and rotation
into its environment work area before querying VDP1 clipping, local
coordinates and projection at MAIN 06030DD8, 06030DEE and 06030CF6.
It opens pass A through 06029028, calls WORLD 06056FA8 to build the
map-specific transform, then calls 0603254A with 6 and 0602905E to
commit. The following call at 06056E52 enters WORLD 06056D7C. The
battle rotation implementation shares the begin/commit services, but
its map transform cannot be substituted for 06056FA8.

Camera state 8 at 0605839A reads root +0x14, then tests the arrival
child's byte at +0x3D. Zero returns without starting the fade. The
nonzero path reads the selected destination's signed status at +2.
The display also consumes +0x3D for marker shrink, so the arrival
child must produce this flag and its live position together.

## Experimental Vita verification

The local device profile enables LAGI_WORLD_MAP_BRINGUP together with
LAGI_BATTLE_EXECUTE_A3. Test menu entry and the E014 handoff separately.
For each, record whether the map background and marker appear, whether
directional input changes the destination label, and which scene opens
after confirming an unlocked destination. Preserve ux0:data/lagi/lagi.log
for each run. WorldMapStartup reports whether resources initialized and
whether the scene path is enabled. WorldMapTravel records the selected
entry and status when the request reaches the module manager.

The current label uses a fixed text row. The arrival launch refreshes
distance after movement as an interim step, and the full native arrival
phase sequence and background composition still need device verification.

Disc 1 destination records 0..3 carry statuses 50 (Above Excavation
Site), 51 (Excavation Site), 52 (Camp), and 53 (Valley). COMMON routes
50/53 to field indices 1/2, which both use FLD_A3.PRG in the generated
build. Statuses 51/52 route to alternate town indices 15/7. Record the
actual selected status during testing so a failed destination overlay
can be investigated separately from the map's navigation and fade.

## Remaining environment motion — corrected native trace, 2026-11-03

`world_map_environment.h` also ports the projected-ground producer and the
424-entry coefficient-wave update. Ground uses the live rotation matrix's
third row, signed coefficient pivot and Zst, retains low-32-bit products/sums,
and delegates division to the supplied native divider before SHLR16/EXTS.W.
The wave helper takes the current coefficient table and native sine service;
it advances local phase per entry but stored phase by only the frame speed.
An undersized table is rejected before mutation rather than silently shortened.
The original initializer literals are speed `0011102A`, frequency `0005B059`,
amplitude `02B3`, and normal-background vertical offset `013B`.

Production-helper host checks cover wave coefficient products, phase wrap,
424 writes, untouched suffix, rejection without mutation and divider argument/
result binding; sine and divide services are mocked. Original-disc initializer,
rotation-matrix and divider anchors pass, as does Vita syntax checking.
These helpers are not yet attached to the live environment: native state
updates and window/zoom handling remain required before an atomic draw cutover.

The normal-background origin arithmetic is now ported to
`include/lagi/world_map_environment.h`. Original `06056E94..06056EF8`
adds environment +0x40 to the signed projected-ground value with native wrap.
For a sum at least 196, the consumer uses its negated 16.16 value; otherwise
it uses literal `FF140000` (-236 in 16.16). This is a signed threshold branch,
not a clamp to 236. The consumer selects NBG0 and calls MAIN `06028986`.
The stored environment +8 value remains `(256-ground)<<16`, distinct from
that final consumed Y. Horizontal origin preserves twelve arithmetic shifts,
then left shift ten and negation, including low-32-bit wrap.

The production header passes 5,832 host comparisons against a separate
64-bit arithmetic oracle, including threshold discontinuity, negative yaw and
signed overflow boundaries. Disc anchors for the consumer branch/call and
literals pass; the header/test also passes Vita syntax checking. It is not
wired into Draw yet: the projected-ground producer, live environment +0x40,
zoom/window consumers and coefficient-wave lifecycle must be connected together.
No visible behavior or hardware acceptance is claimed by this checkpoint.

Disc 1 WORLD Draw at `06056E54..88` stores the result of `06056D7C`
in environment +0x38. It derives the horizontal origin as
`-(yaw >> 12) * 0x400` and the vertical origin as
`(0x100 - scrollOffset) * 0x10000`, then calls `060565C0`.
The earlier description of `060565C0` as an NBG0 scroll-table generator was
incorrect. Its selected-table load and multiply loop read existing rotation
coefficients, then overwrite each with the fixed-point product of that value
and `0x10000 + MTH_Mul(amplitude, sine)`. It processes `0x1A8` entries. Sine
index is `(phase >> 16) & 0xFFF`; environment +0x50 supplies amplitude, +0x4C
the per-entry phase step, +0x48 the per-frame phase step, and +0x54 the phase.
The table is selected using the current rotation pass and VDP2 buffer index.
The literal at `06056650` is native fixed-point multiply `06035644`, and
`06056654` points to the native sine table `20216E80`.

Normal-background X/Y origins are separate environment fields +4/+8, derived
by the caller before wave modulation. Their consumer and the environment's
live phase/amplitude initialization/update must be recovered before hookup;
do not invent a per-line NBG0 scroll buffer from the `0x1A8` loop count.
The same wave arithmetic is recognizable in upstream
`shared/vdp2PlaneTask.cpp::vdp2ApplyWaveDistortion`, but its work-area offsets
are different and cannot be aliased to the WORLD environment.

`tests/world_map_environment_evidence_test.ps1 -DiscImage <Disc1.bin>` checks
the original instruction anchors, fixed-point/sine literals and call target
using read-only sector access. It passes on the current US Disc 1 image.
This is evidence preservation, not execution of the routine or rendered
environment acceptance. No runtime change is included in this checkpoint.
