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
matching log. Arrival's interim middle phase and fixed-row name remain open.

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
