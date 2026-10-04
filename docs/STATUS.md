# Lagi Development Status

Current milestone: **0.040-alpha — authentic boot flow**

Last updated: 2026-10-04

Lagi is a native PlayStation Vita runtime for *Panzer Dragoon Saga* / *Azel*. Reconstructed game logic executes directly on ARMv7; Saturn rendering and platform-facing behavior are translated to VitaSDK and native SceGxm.

## Current development stage

The 0.040 milestone replaces the first Ruins room's special direct-boot path with Azel's own startup, movie, title, New Game, field, module-manager, and native town presentation flow. The milestone is hardware-proven through a playable first Ruins scene.

The current hardware path reaches:

```text
Lagi startup
  ↓
azelInit()
  ↓
resetEngine()
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
```

The normal 0.040 path does **not** use the old direct-Ruins loader to choose or start `TWN_RUIN`.

Azel remains responsible for game status, module transitions, scripts, task creation, movies, fades, camera/gameplay state, and scene ownership.

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

## Hardware-proven 0.040 boot systems

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
- Cinepak playback
- SGX Cinepak presentation
- native SceAudio output
- Azel VDP1 command capture
- native GXM VDP1 normal/scaled sprite translation
- native GXM VDP1 polyline translation
- live VDP2 VRAM/CRAM upload
- title NBG presentation
- D5 NBG0 keyboard presentation
- D5 NBG3 subtitle/text presentation
- hardware-linear front-end/menu/name-entry text presentation
- D5 VDP1 cursor data/CRAM decoding
- RBG0 SGX program registration after register-pressure reduction
- native RBG0 parameter/coefficient state capture
- generic presentation producer/consumer synchronization

## Current front-end rendering state

### Title

The title screen now follows Azel's live TVMD state into a native 720x408 Vita framebuffer and addresses the full 704x448 source raster. Hardware testing confirms the title artwork now matches the expected presentation closely enough for the current milestone.

Raw VRAM/CRAM reads remain point-exact. The title artwork's reduction into the Vita framebuffer filters only after palette lookup / RGB555 decode, so packed Saturn memory and palette indices are never interpolated.

The live title/menu text layer (PRESS START, CONTINUE, NEW GAME and related prompts) is decoded separately to RGBA and composited through SGX with hardware linear filtering. The high-resolution title artwork path itself is not modified by that text filtering pass.

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

- RBG0 composition still does not visually match Saturn;
- parameter A/B and window behavior require more accuracy work;
- lower-half composition has been a primary mismatch;
- fade/flash presentation remains visibly incorrect;
- complete VDP2 priority/color-calculation behavior is not yet implemented;
- D5 sprites/compositing still need continued hardware comparison.

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
- SGX performs coefficient lookup, rotated coordinate generation, tile lookup, CRAM lookup, and pixel rendering;
- line-window visibility is handled at the compositor level rather than inside the heavy RBG0 fragment shader.

A fragment-side line-window implementation caused a real SGX GPU crash and was removed.

## Fade / color-offset status

Azel's fade state is running and hardware logs show the expected changing VDP2 offset values.

Neptune currently bridges Azel color offsets into native presentation. The bridge now applies signed 9-bit VDP2 wrapping semantics instead of naïve clamping.

Visible fades are still not Saturn-accurate.

Remaining work includes:

- honoring the appropriate VDP2 enable/select semantics;
- validating A/B offset selection by layer;
- matching Saturn white/black flashes;
- ensuring movie, title, field, and scene transitions use the same generic VDP2 color path.

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

## First Ruins transition status

Azel is confirmed to:

- finish/skip `EVT002.CPK`;
- load `TWN_RUIN.PRG`;
- enter status `0x04`, mode `1`;
- create native town state;
- run the first town fade sequence.

This proves the game transition itself is no longer dependent on the old loader bypass.

The remaining black-screen/first-scene work is in the native scene-to-Neptune presentation path.

Historically, Neptune's live-town rendering depended on readiness flags initialized by `load_static_room_viewer()`, which belongs to the earlier direct-boot reconstruction path. The 0.040 work is removing those hidden dependencies so authentic boot can source all required presentation state from live Azel output.

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
3D gameplay framebuffer:              480x272
high-resolution title framebuffer:    720x408
title source VDP2 raster:             704x448
target presentation:                  30 Hz
```

The active front-end framebuffer mode follows Azel's VDP2 TVMD state. The title path scans out 720x408 directly; gameplay retains the established 480x272 path.

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

The live material cache still lacks generation-aware lifetime invalidation for frequent scene/resource transitions.

### Batching/index limits

The current live 3D renderer still flattens active work into shared buffers with 16-bit indices. Larger scenes are expected to require multiple resident batches while preserving Azel draw ordering, visibility, materials, and dynamic updates.

## Current development focus

With the authentic first-Ruins handoff now hardware-proven, current work is concentrated on D5 RBG0 A/B and window composition, generic VDP2 fade/color-offset accuracy, migration of remaining historical `town_*` renderer state into scene/presentation terminology, broader scene/resource lifetime handling, and moving more of the active Saturn lighting work from CPU preparation into SGX where practical.

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

Decoded 2D presentation resources now use SGX linear filtering when scaled:

- VDP1 UI sprites, including Lock-On/LCS cursors and menu selectors
- NBG1 menu/window atlas
- VDP2 subtitle, interaction, item, title-menu, and name-entry text through a 352x224 logical text layer

Raw Saturn memory fetches remain point-exact. VDP2 image-plane filtering is tracked separately because those backgrounds are still decoded directly from VRAM/CRAM during composition.

Frontend timing now reports `[VDP2Perf]` with framebuffer dimensions, total front-end render time, and GPU wait time. The current high-resolution title shader performs multiple VDP2 decodes per output pixel during filtered sampling; a decode-once layer surface is the planned optimization for that path.
