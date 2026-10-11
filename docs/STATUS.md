# Lagi Development Status

Current milestone: **native FLD_A3 flight and shared Neptune rendering — user accepted**

Last updated: 2026-10-10

Lagi is a native PlayStation Vita runtime for *Panzer Dragoon Saga* / *Azel*. Reconstructed game logic executes directly on ARMv7; Saturn rendering and platform-facing behavior are translated to VitaSDK and native SceGxm.

## Current development stage

Cinepak width-fit/overlay follow-up: movie output fills the Vita width without stretching, cropping vertically only if needed. Native cinematic matte and subtitle snapshots are now composed above video, before fade. Source reconstruction, title sizing and the accepted dither filter are unchanged. Renderer syntax and CPU/source tests pass; the new package still needs Vita playback/overlay confirmation.

The screen-space dither checkpoint `508bf75` was accepted by the user on Vita on 2026-10-10. The supplied log identifies `enabled=1 ... sampling=lookup-point-v1`; the user reports effectively no dither-specific gameplay penalty compared with hardware bilinear. Full-screen menus still drop below 30 fps in both modes and remain an accepted limitation of this merge, not a solved performance target. The final merged main package has not been rebuilt/retested here. See `docs/DITHER_PERFORMANCE.md`.

Current main extends the earlier public v0.3.0alpha hardware milestone through native FLD_A3 flight, E006 cinematic and Excavation arrival. Native audio and Cinepak remain integrated; this merge is a development checkpoint, not a new packaged release.

The current hardware path reaches:

```text
Lagi startup
  ↓
azelInit() / resetEngine()
  ↓
MOVIE1.CPK
  ↓
title / menu
  ↓
New Game
  ↓
EVT000_1.CPK
  ↓
FLD_D5 name-entry sequence
  ↓
name confirmation
  ↓
EVT002.CPK
  ↓
Azel module manager
  ↓
TWN_RUIN.PRG
  ↓
native town task graph
  ↓
playable Ruins sequence
  ↓
elevator choice
  ↓
EVT004_1.CPK / EVT004_2.CPK
  ↓
FLD_A3.PRG / native field task graph
  ↓
E006 in-engine cinematic
  ↓
Excavation arrival
```

The normal path does **not** use the old direct-Ruins loader to choose or start `TWN_RUIN`. Azel remains responsible for game status, module transitions, scripts, task creation, movies, fades, camera/gameplay state, and scene ownership.

The current build also connects Azel's native three-slot save/load flow to Vita
storage. Title Continue now reflects valid on-disk saves, and the in-game
System menu can enter Azel's Save and Load tasks. Compile, link, and VPK
packaging are verified; on-device menu, persistence, and transition validation
remain pending.

## Runtime architecture

The current ownership model is:

```text
Azel decides.
Lagi services.
Neptune renders.
```

The old `runtime_smoke_*` host loop has been retired.

Current host structure:

```text
src/integration/lagi_runtime.cpp
    native Azel host loop

src/integration/lagi_input_bridge.cpp
    Vita physical input -> Saturn/Azel input state

src/integration/lagi_diagnostics.cpp
    smoke checks and runtime diagnostics

src/integration/lagi_scene_bridge.cpp
    active Azel scene -> generic presentation adapter

src/platform/vita/neptune_renderer.cpp
    native SceGxm presentation
```

The renderer-facing API is mode-agnostic. Lagi no longer conceptually takes "town ownership" to synchronize or publish a frame.

Mode 1 currently dispatches to the existing town adapter because it is the first native 3D scene class supported by the presentation bridge.

## Generic presentation boundary

Current flow:

```text
runTasks()
    ↓
capture Azel VDP1 / renderer-facing state
    ↓
sync active scene presentation state
    ↓
acquire presentation slot
    ↓
publish immutable frame
    ↓
Neptune render thread
```

Frame synchronization is deliberately acquired at the publish boundary rather than before `runTasks()`. This prevents a mode transition inside the Azel task pass from deadlocking with another presentation owner such as movie playback.

The platform API now uses generic names such as:

- `presentation_wait_frame_slot()`
- `presentation_publish_frame()`
- `presentation_set_player()`
- `presentation_set_camera()`
- `presentation_set_vdp2_text()`
- `presentation_fade_in()`
- `presentation_fade_out()`

Some Neptune internal variables still use historical `g_town*` names. Those are implementation cleanup items, not ownership semantics.

## Hardware-proven v0.3.0alpha systems

Verified on real Vita/Vita TV hardware:

- Vita process/platform startup
- persistent logging
- Disc 1 CUE/BIN access
- MODE1/2352 and MODE1/2048 support
- ISO9660 traversal
- `COMMON.DAT` parsing
- native `azelInit()` / `resetEngine()` startup
- Azel initial-task switching
- authentic boot movie launch
- native title task/menu flow
- New Game transition
- native D5 field load
- name-entry keyboard input
- entered player name display
- "this is your real name" confirmation flow
- pre-Ruins `EVT002.CPK` movie launch
- Start movie skip without leaking the same Start edge into gameplay
- native module-manager load of `TWN_RUIN.PRG`
- entry into game mode 1 / first Ruins town task graph
- continuous native first-Ruins world presentation
- native Edge animated hierarchy presentation
- task-owned switch/object presentation
- live Azel light/falloff state on static and dynamic submissions
- Full, Texture, Lighting, Quads, and Wires renderer views
- Vita-to-Saturn physical controller bridge
- Sega FILM demux
- Cinepak playback through the post-elevator movie pair
- source-resolution SGX Cinepak reconstruction with screen-space lookup-dithered final presentation
- native SceAudio output
- native BGM and sound-effect playback
- runtime ARM SCSP DSP translation
- Azel VDP1 command capture
- native GXM VDP1 normal/scaled sprite translation
- native GXM VDP1 polyline translation
- live VDP2 VRAM/CRAM upload
- title NBG presentation
- D5 NBG0 keyboard presentation
- D5 NBG3 subtitle/text presentation
- screen-space lookup-dithered front-end/menu/name-entry text presentation
- D5 VDP1 cursor data/CRAM decoding
- RBG0 SGX program registration after register-pressure reduction
- native RBG0 parameter/coefficient state capture
- generic presentation producer/consumer synchronization

## Current front-end rendering state

### Title

The title screen now follows Azel's live TVMD state into a native 720x408 Vita framebuffer and addresses the full 704x448 source raster. Hardware testing confirms the title artwork now matches the expected presentation closely enough for the current milestone.

Raw VRAM/CRAM reads remain point-exact. The title artwork's reduction into the Vita framebuffer filters only after palette lookup / RGB555 decode, so packed Saturn memory and palette indices are never interpolated.

The live title/menu text layer (PRESS START, CONTINUE, NEW GAME and related prompts) is decoded separately to RGBA and composited through SGX with screen-space lookup dithering. The high-resolution title artwork is independently decoded and dithered; palette interpretation remains exact.

### D5 name-entry sequence

The D5 field is being rendered from Azel's real VDP2 state.

Confirmed state includes:

- `BGON=0x101A`
- `RPMD=3`
- RBG0 parameter A/B data
- live coefficient tables
- native window registers
- native line-window table
- NBG0 keyboard
- NBG3 text
- VDP1 sprite stream

The upper portion of the RBG0 scene has shown recognizable/correct-looking portions on hardware, proving that the native content and part of the addressing/rotation path are valid.

Remaining D5 issues:

- the name-entry RBG0 composition still does not visually match Saturn;
- parameter A/B and window behavior require more accuracy work;
- lower-half composition has been a primary mismatch;
- complete VDP2 priority/color-calculation behavior is not yet implemented;
- D5 sprites/text/background compositing still need continued hardware comparison.

The shared fade/color-offset path is now hardware-matched closely enough to the Saturn reference for title, cinematic, and first-Ruins transitions. The remaining name-entry mismatch is therefore being treated as VDP2 composition work rather than a generic fade defect.

The Saturn reference capture is authoritative for content, timing, color, fades, and layer relationships. Its external HDMI converter stretches the source to 16:9; The HDMI converter stretch is not representative of the intended aspect; Vita presentation retains the Saturn-authored framing instead.

## VDP2 direction

Neptune is being refactored toward a generic VDP2 renderer rather than screen-specific shaders.

Conceptual direction:

```text
Azel
  ↓
VRAM / CRAM / VDP2 registers
  ↓
Neptune
  ├─ generic NBG pipeline
  ├─ generic RBG pipeline
  ├─ back/line/color state
  ├─ windows
  └─ composition
  ↓
SGX
```

The RBG0 shader was split from the NBG path after PSP2CGC hit internal-compiler/register-pressure limits.

Current RBG0 strategy:

- invariant Saturn rotation terms are precomputed once per host frame;
- SGX performs coefficient lookup, rotated coordinate generation, tile lookup, CRAM lookup, and pixel rendering at the Saturn-authored 352x224 resolution;
- native scenes resolve RBG0 to an SGX render texture and point-scale that conventional RGBA result into the 480x272 gameplay framebuffer, avoiding redundant raw-memory shader evaluation for duplicate output pixels;
- line-window visibility is handled at the compositor level rather than inside the heavy RBG0 fragment shader.

All additional VDP2 work follows the same SGX-first design rule. In-game
rotation-background and world layers use point filtering; decoded UI/text/title
and Cinepak presentation use screen-space dithering by default. ARM NEON is reserved for
measured CPU-bound preparation or decode stages where it is the best exact
implementation; GPU-bound work is not moved to the CPU merely to use NEON.

A fragment-side line-window implementation caused a real SGX GPU crash and was removed.

## Fade / color-offset status

The generic Azel-to-Neptune fade path is now hardware-proven against direct Saturn capture for the current boot sequence.

The integration corrects a reconstruction error in the host-side fade step direction while leaving Azel authoritative for target color and duration. Neptune consumes Azel's VDP2 color-offset state with signed 9-bit wrapping semantics, and VDP2 reinitialization republishes the live fade-channel values so newly constructed front-end layers cannot flash full-bright during preload.

Verified current behavior includes:

- title fade-in;
- title fade-out;
- Cinepak/cinematic fade-in and fade-out;
- first-Ruins transition fade.

No screen-specific Vita fade constructor is required for these paths.

## Movie pipeline

Movie sequencing remains Azel-owned.

```text
Azel movie task/state machine
       ↓
Lagi movie backend
  ├─ disc streaming
  ├─ Sega FILM demux
  ├─ Cinepak decode
  ├─ native SceAudio
  └─ Neptune presentation
```

The movie backend does not choose the next game mode.

A Start edge used to skip a movie is consumed at the movie handoff so the same physical press cannot open the gameplay pause menu on the first scene frame.

A previous retained-frame bug that allowed the last Cinepak frame to cover a scene after Azel had already advanced has also been corrected.

## Native audio and SCSP DSP

The current release carries BGM and sound effects through the native Vita audio service. Azel's SCSP-facing code remains authoritative for sequencing and DSP program state; Lagi supplies the Vita output path and the translator used to execute SCSP DSP programs efficiently on ARM.

Runtime ARM translation is now the normal backend. A standard install does not require `ux0:data/lagi/dsp_backend.txt`. Explicit backend files remain supported for diagnostic comparison.

Hardware validation of the 84-step program measured a 2,268 us median and 2,392 us P95 across the matched test window, compared with a corrected 5,264 us predecoded median. The corresponding whole audio quantum measured 5,297 us median against a 5,804 us budget, with zero short writes. Disc-entry crackle is nearly eliminated on the ARM path.

The runtime keeps the generic interpreter as the safe fallback while an ARM translation is pending or unavailable. The legacy predecoded fast path is used only when explicitly selected.

## First Ruins systems already proven in earlier direct-boot work

The earlier bring-up remains valuable because the underlying systems were hardware-proven before authentic boot integration:

- native town tasks/scripts
- world-grid/cell ownership
- static and dynamic object submission
- `processTownMeshCollision()` pipeline
- Edge movement
- Edge animation
- Edge collision
- Azel follow camera
- visibility/LOD
- Ruins lock/switch objects
- Lock-On/LCS
- VDP1 cursor/marker/selection-box behavior
- VDP2 text/windows
- elevator choice flow
- Edge's original textured/stippled VDP1 mesh shadow
- stable 30 Hz town presentation

Those renderer/platform capabilities are being reconnected to the authentic boot path without restoring direct-loader ownership.

## Renderer

### Resolution

Current gameplay/front-end target:

```text
3D gameplay/movie framebuffer:        480x272 default; 960x544 with LAGI_FULLRES=ON
high-resolution title framebuffer:    720x408
title source VDP2 raster:             704x448
target presentation:                  30 Hz
```

The active front-end framebuffer mode follows Azel's VDP2 TVMD state. The title path scans out 720x408 directly; gameplay/movie output follows `LAGI_FULLRES` without changing source-resolution Cinepak reconstruction.

Saturn-authored content is presented at its intended aspect rather than stretched to match a 16:9 capture device.

### Diagnostic views

The existing scene renderer retains:

```text
Full
Texture
Lighting
Quads
Wires
```

These modes are renderer diagnostics and are independent of game-mode ownership.

### Gouraud

The current Vita renderer retains the hardware-tested subdivision approximation used for first-Ruins bring-up:

```text
Saturn quad
 -> 3x3 subdivision
 -> 8 GXM triangles
 -> four-corner lighting field
 -> RGB555-style add/clamp/quantize
```

The exact inverse-bilinear reference path remains useful for comparison but is too expensive for the active Vita scene path.

## Known architectural debt

### Historical town naming inside Neptune

The public presentation API is now generic, but internal renderer data still contains names such as:

- `g_townPlayerReady`
- `g_townCameraPosition`
- `buildLiveTownFrame()`

These names are expected to migrate toward generic scene/VDP1 presentation terminology as additional game modes come online.

This is naming and organization debt rather than a runtime ownership dependency.

### Direct-boot source files

Earlier direct-boot/bootstrap source remains compiled for development/reference purposes.

Authentic boot currently avoids state established exclusively by:

- `init_town_bootstrap()`
- `init_town_runtime()`
- manual `overlayStart_TWN_RUIN()`
- static-room viewer registration

Renderer data for the native path is sourced from generic platform initialization or live Azel output.

### Material/resource lifetime

Native textures now have generation-aware CPU/GPU residency and bounded write-range invalidation. Image/LUT writes and recorded CRAM dependencies determine staleness; callbacks only publish atomic notifications and the render thread owns cache/resource mutation. Unknown dependencies retain conservative invalidation. Static-instance/model caching reduces traversal rebuilding, but cold material preparation and flattened shared-buffer/index limits remain architectural debt.

### Batching/index limits

The current live 3D renderer still flattens active work into shared buffers with 16-bit indices. Larger scenes are expected to require multiple resident batches while preserving Azel draw ordering, visibility, materials, and dynamic updates.

## Current development focus

The user accepted the flight/rendering milestone on 2026-10-10 and authorized merging the rendering branch to main. Hardware confirms native field entry, correct field orientation, cinematic/Excavation actor placement, LCS layering and visible collection/save/destructible effects. Native field has no additional Saturn-to-GXM X mirror or winding XOR; town retains its historical mirror. The radar map stays point-filtered. A follow-up corrects its fixed-output-pixel sizing: 48x48 in the 480x272 reference layout becomes 96x96 at 960x544. That follow-up is CPU/syntax-checked; hardware alignment and checker appearance still need confirmation.

Latest authoritative capture: 913219 bytes, 20:45 local, range-invalidation checkpoint 6f74bf2. Sampled RBG-enabled native render median 23.887 ms, p90 25.163 ms. Completed field windows contain 3960 frames, 513 over 25 ms and two over 33.333 ms (39.078/41.906 ms). Completed presentation windows have zero >2-vblank intervals; cold outliers lack presentation baselines. The user explicitly accepts these two spikes for now. This is acceptance of the milestone, not proof that every scene/frame meets the original deadline.

Final runtime checkpoint dd579eb records the actual CRAM blocks read by native decoding. Dependency tests, 5184 decoder comparisons and Vita syntax validation pass; no separate hardware capture validates that refinement. No final merged package was built in this session. Keep the incorrect E006/Excavation VDP2 floor open; its cause is not established. Native ray/laser rendering and broader scene coverage remain deferred. Audio profiling PR #11 is separate and is not merged by this rendering task.

Longer-term work includes additional game modes, broader scene/resource lifetime handling, migration of historical `town_*` renderer naming, and continued movement of presentation work toward SGX where it improves the Vita path without taking ownership away from Azel.

## Historical reference paths

The older direct-Ruins and Basic Wing work remains useful for:

- regression testing;
- VDP1 texture decoding;
- hierarchy/hotpoint validation;
- animation decoding;
- collision validation;
- Gouraud/RGB555 comparison;
- shader bring-up;
- GXM resource validation.

They are no longer the intended normal execution path.


### 2D filtering

Decoded 2D presentation resources now use accepted screen-space 2x2 lookup dithering when scaled (`LAGI_STOCHASTIC_FILTER=ON`, `LAGI_DITHER_LOOKUP=ON`, both default ON):

- VDP1 UI and Gouraud particle sprites, including Lock-On/LCS cursors and menu selectors; the field radar map remains point-filtered
- NBG1 menu/window atlas
- VDP2 subtitle, interaction, item, title-menu, and name-entry text through a 352x224 logical text layer
- decoded title artwork and resolved Cinepak final presentation

Raw Saturn memory fetches remain point-exact. VDP2 image-plane filtering is tracked separately because those backgrounds are still decoded directly from VRAM/CRAM during composition.

Frontend timing reports `[VDP2Perf]` with framebuffer dimensions, total front-end render time, and GPU wait time. The high-resolution title background uses a decode-once RGBA layer surface followed by SGX lookup-dithered presentation, removing repeated raw VDP2 decode cost from the normal output path. Hardware bilinear remains available with `LAGI_STOCHASTIC_FILTER=OFF`; the raw title fallback retains arithmetic dithering.
