# Lagi Runtime Architecture

Lagi is a native PlayStation Vita host for the reconstructed *Panzer Dragoon Saga* runtime in `extern/Azel`.

## Overview

The runtime is divided into three primary layers:

```text
Azel
    game logic and Saturn-era runtime state

Lagi
    Vita platform integration and presentation transport

Neptune
    native SceGxm rendering
```

Azel remains authoritative for startup state, game modes, module transitions, title and menu state, New Game flow, movies, field/town/battle task graphs, scripts, camera behavior, collision, animation, visibility, object lifetime, VDP1 command generation, VDP2 state, fades, and transition timing.

Lagi provides the Vita-facing services used by that runtime: input translation, disc/filesystem access, VBlank and timing services, native audio, memory/resource adaptation, frame synchronization, and renderer-facing presentation snapshots.

Neptune consumes those snapshots and presents them through native SceGxm.

`extern/Azel` remains an upstream source tree. Vita-specific adaptation is implemented outside it.

## Shared field/town rendering and resource lifetime

Native FLD_A3 visibility tasks choose active cells/models; Lagi publishes stable identities, transforms and native camera state, and Neptune services their presentation. Static-instance residency is independent of changing lighting/visibility. Dynamic actors and image particles remain native submissions. Camera-space paths are classified/scoped before removing the native view transform; do not synthesize a Vita follow camera or blanket-convert every submission.

Field geometry uses Azel's native horizontal orientation without town's extra Saturn-to-GXM X mirror or winding XOR. Town/Ruins retains its historical mirror. The field radar map intentionally uses point filtering to preserve mesh/checker pseudo-transparency. Its 48x48 size in the 480x272 reference layout scales to 96x96 at 960x544; the source texture remains 48x48.

Native textures retain GPU storage across geometry-only changes. An 8 MiB CDRAM-preferred startup arena is reserved after GXM initialization, with system-memory/overflow fallback and renderer-owned release. Producer callbacks publish bounded atomic write blocks rather than modifying caches: 512-byte VDP1 blocks and 32-byte CRAM blocks. Offsets and absolute Saturn addresses normalize to the same ranges; unknown ranges retain full invalidation.

Decoded materials carry opacity and palette-read metadata. Image bytes, LUT bytes and recorded CRAM blocks determine refresh dependencies. Only previously valid unaffected CPU/GPU generations advance; older stale entries remain stale until used. RGB555 LUT entries do not depend on CRAM; zero-valued palette reads still do. Unknown producers retain conservative behavior. No scene-ID overrides are used. The final captured-CRAM refinement is host/syntax-validated but awaits separate hardware evidence.

Shared VDP2 snapshots carry Azel's rotation parameters, maps, windows, priorities and fades to SGX sampling/composition. The incorrect E006/Excavation floor remains under investigation; original map-table presence does not justify forcing a map address. Persistent per-model GPU resources remain the scaling direction for larger towns such as Zoah; the current flattened active buffers are not the final universal scene-resource architecture.

## Native runtime host

The normal Vita host loop is implemented in `src/integration/lagi_runtime.cpp`.

```text
Vita input
    ↓
Lagi input bridge
    ↓
Azel input state

VBlank / fade service
    ↓
begin Azel presentation capture
    ↓
runTasks()
    ↓
capture renderer-facing state
    ↓
scene bridge
    ↓
presentation publish
    ↓
Neptune render thread
```

The earlier `runtime_smoke_*` host has been retired. The Saturn memory-reader check and runtime diagnostics now live in dedicated diagnostic code.

## 0.040 authentic boot path

The 0.040 branch enters the game through Azel's native startup path:

```text
Disc 1
  ↓
azelInit()
  ↓
resetEngine()
  ↓
native Azel initial task graph
  ↓
MOVIE1.CPK
  ↓
title / New Game
  ↓
EVT000_1.CPK
  ↓
FLD_D5 name-entry sequence
  ↓
EVT002.CPK
  ↓
Azel module manager
  ↓
TWN_RUIN.PRG
  ↓
native town task graph
```

`TWN_RUIN` is selected and started by Azel's module flow. The earlier direct-Ruins bootstrap remains in the source tree for development and regression use but is not part of the normal 0.040 boot path.

## Scene presentation bridge

Scene ownership remains on the Azel side. Lagi exposes renderer-facing state through a generic scene bridge:

```text
Azel active scene
    ↓
lagi::scene_bridge::sync_presentation_state()
    ↓
mode-specific adapter
    ↓
generic presentation state
    ↓
Neptune
```

Mode 1 currently dispatches to the existing town adapter because town scenes are the first native 3D scene class connected to Neptune. The adapter publishes camera, player, VDP1/VDP2, and related presentation state without taking ownership of the scene itself.

The platform-facing renderer interface is mode-agnostic:

```text
show_game_presentation()

presentation_wait_frame_slot()
presentation_publish_frame()

presentation_set_player()
presentation_set_camera()
presentation_set_vdp2_text()

presentation_fade_in()
presentation_fade_out()
```

Some Neptune internals still use historical `g_town*` names. Those names reflect the origin of the first 3D renderer path rather than the current ownership model.

## Frame synchronization

The presentation bridge uses a one-frame producer/consumer boundary between the Azel/game thread and the Neptune render thread.

```text
runTasks()
    ↓
collect staging state
    ↓
sync scene presentation
    ↓
acquire presentation slot
    ↓
publish immutable frame
    ↓
Neptune renders the frame
    ↓
render thread returns the slot
```

The presentation slot is acquired at the publish boundary. This allows Azel to change modes during `runTasks()` without holding a renderer slot that may subsequently be needed by movie or front-end presentation.

Movie playback and front-end VDP2 rendering participate in the same renderer ownership protocol.

## VDP1 presentation

Azel generates VDP1 state and commands. Lagi captures the renderer-facing output and Neptune translates the supported command stream to native GXM.

Current hardware-tested coverage includes:

- normal sprites;
- scaled sprites;
- polylines;
- first-Ruins world geometry;
- task-owned dynamic objects;
- Edge's animated hierarchy submitted through Azel's normal `addObjectToDrawList()` boundary;
- per-submission Azel light vector, RGB, and falloff state;
- Edge's original mesh-shadow path.

Lock-On state, menu behavior, cursor behavior, animation state, and other gameplay decisions remain part of Azel's task graph.

## VDP2 presentation

Neptune's VDP2 path is organized around Saturn VDP2 state rather than individual game screens.

```text
Azel
  ↓
VRAM / CRAM / VDP2 registers
  ↓
Neptune VDP2
  ├─ NBG
  ├─ RBG
  ├─ back / line / color state
  └─ windows and composition
  ↓
SGX
```

The current front-end path consumes live Azel state for:

- title NBG presentation;
- D5 NBG0 keyboard;
- NBG3 text;
- D5 RBG0 rotation-map state;
- line-window composition;
- live CRAM;
- VDP2 color offsets and fade selection.

D5 uses the same VDP2 facilities as the rest of the runtime; it is not treated as a separate authored background implementation.

The remaining RBG0 work is primarily the D5 name-entry accuracy pass around A/B composition, windows, priority behavior, and color calculation. Shared fade direction/timing/color is now handled by the generic Azel-to-Neptune VDP2 path.

VDP2 implementation is SGX-first. Azel remains authoritative for registers,
VRAM/CRAM contents, timing, and layer ownership; Neptune translates that state
into GPU work at the Saturn-authored logical resolution and lets SGX perform
composition and final scaling. CPU-side work is limited to state capture,
infrequent resource preparation, and operations that are demonstrably cheaper
or unavailable on SGX. ARM NEON is used only for measured CPU-bound stages where
vectorization preserves exact Saturn-visible results; it is not a substitute
for moving scalable raster work to SGX.

In-game VDP2 rotation backgrounds and world presentation use point filtering.
Decoded UI/text/title and Cinepak presentation use screen-space coordinate
dithering by default; hardware bilinear remains an A/B fallback.

The native-scene RBG0 path follows these rules explicitly: the coefficient,
map, character, and palette program resolves one sample per 352x224 Saturn
pixel into an RGBA surface, then SGX point-scales that surface into the 480x272
gameplay framebuffer. This preserves the existing point-exact appearance while
avoiding duplicate execution of the heavy Saturn memory-interpretation shader
at Vita output resolution.

## Movie pipeline

Movie sequencing remains part of Azel's movie task/state machine.

```text
Azel movie task/state machine
       ↓
Lagi movie backend
  ├─ disc/filesystem reads
  ├─ Sega FILM demux
  ├─ Cinepak decode
  ├─ PCM/audio timing
  └─ Neptune presentation
```

Azel supplies movie selection, sequencing, skip state, fades, subtitle-task creation, and post-movie status transitions. Lagi supplies the platform services required to execute those decisions on Vita.

The current Vita path includes native SceAudio and SGX-assisted Cinepak presentation.

## Native audio and SCSP DSP

Azel remains authoritative for SCSP-facing state, sequence changes, and DSP program contents. Lagi provides the native Vita audio service and the execution backend used to run SCSP DSP programs efficiently on ARMv7.

```text
Azel SCSP state / MPRO
    ↓
Lagi SCSP service boundary
    ↓
runtime ARM translation + native-code cache
    ↓
256-frame audio worker quantum
    ↓
SceAudioOut
```

Runtime ARM is the normal backend. No backend-selection file is required for standard operation. `ux0:data/lagi/dsp_backend.txt` exists as a diagnostic override only.

The translator is keyed by complete DSP instruction contents and length rather than scene identity. New or changed MPRO contents invalidate the active native routine; the generic interpreter remains available while translation is pending or if runtime executable memory is unavailable. The legacy predecoded fast path is reserved for explicit diagnostic selection.

## Direct-boot compatibility

The earlier direct-Ruins path remains useful for isolated renderer/runtime tests and regression work.

Normal authentic boot does not depend on state established exclusively by the direct-boot bootstrap, including manual Ruins overlay startup or static-room viewer registration. Renderer state needed by the native path is sourced from platform initialization or live Azel presentation data.

## Upstream integration

Azel's desktop-facing host interfaces are adapted at the Lagi boundary for Vita builds. The Vita prelude supplies declarations and platform substitutions required for compilation while gameplay and task/state-machine behavior remains in the upstream implementation.

## Save and load ownership

Azel owns the native save payload, weighted checksum, three-slot selection UI,
load restoration, and game-status transitions. Lagi supplies the Vita storage
boundary at `ux0:data/lagi/save/0` and maps the original internal-backup device
to durable files named `PANDRA_3_01` through `PANDRA_3_03`.

Title Continue is enabled only when at least one slot passes Azel's version and
checksum rules. It enters Azel's load/save overlay through game status `0x4A`.
The in-game System entry is a Vita replacement for the pinned upstream stub; it
launches Azel's existing `createSaveTask()` and `createLoadTask()` rather than
implementing another slot UI or save format. A successful in-game load returns
to Azel's module manager, which performs the saved scene transition and field
entry restoration.


## 0.040 native Ruins presentation

The completed 0.040 path reaches the first Ruins scene without using the direct-Ruins bootstrap. Azel creates and updates the scene, then Lagi captures renderer-facing output at the existing Azel boundaries.

Static world submissions preserve world-space transforms before Neptune applies the native town camera. Dynamic Edge hierarchy submissions are converted from Azel's view-relative matrix state back into world space at the render bridge. The hierarchy, pose evaluation, movement, and animation remain owned by `sEdgeTask`.

Lighting is captured with each submission from Azel's active light state. Geometry/material cache identity is independent from lighting state; cached static polygons refresh their current light payload without forcing a room rebuild. Neptune's current Full/Lighting path still evaluates the Saturn-style Gouraud contribution on the CPU before GPU interpolation, leaving GPU-side lighting evaluation as a later renderer optimization rather than part of the 0.040 milestone.


## VDP2 display framebuffer modes

Front-end presentation follows Azel's live VDP2 TVMD state. Gameplay/movie output is 480x272 by default or 960x544 with `LAGI_FULLRES=ON`. The title screen enters Azel's 704-dot high-resolution mode and is presented through a 720x408 Vita framebuffer. Cinepak reconstruction retains its dedicated source-sized 480x272 target regardless of output selection.

Neptune renders directly into the active framebuffer size and passes that same width, height, pitch, and buffer to `sceDisplaySetFrameBuf()`.

The title path addresses the 704x448 VDP2 source raster, decodes the Saturn tile/palette data into an RGBA presentation surface, then uses screen-space lookup dithering into the 720x408 framebuffer. Raw VRAM and CRAM interpretation remains point-exact; filtering occurs only after palette lookup on decoded RGB pixels.


## 2D presentation filtering

Neptune keeps Saturn memory interpretation separate from presentation filtering. Raw VDP1/VDP2 VRAM, CRAM, pattern names, palette indices, and command data are addressed without texture filtering. Decoded 2D RGBA assets use Unreal-inspired ordered 2x2 coordinate dithering when rescaled into the active Vita framebuffer. The pattern comes from native fragment `WPOS`, never UV multiplied by output dimensions.

Filtered paths include decoded title artwork, VDP1 UI sprites such as Lock-On/LCS cursors and menu selectors, Gouraud particle sprites, the decoded NBG1 UI/window atlas and backgrounds, logical 352x224 VDP2 text, and final resolved Cinepak presentation. The field radar map remains point-filtered with resolution-scaled geometry (48x48 at 480x272, 96x96 at 960x544); world polygon shaders are unchanged.

The default `LAGI_DITHER_LOOKUP=ON` implementation point-samples a repeating 2x2 FP16 texture at `WPOS.xy * 0.5`. Its exact quarter-texel offsets are prebiased by -0.5 and scaled by CPU-computed reciprocal source dimensions. Texture unit 1 owns the immutable offset lookup; unit 0 point-samples the source RGBA texture. This replaces parity ALU with a cached lookup, not a reduction to one total texture fetch. Gouraud RGB555 correction and alpha discard remain intact. The arithmetic path is available with `LAGI_DITHER_LOOKUP=OFF`; decoded bilinear presentation with `LAGI_STOCHASTIC_FILTER=OFF`. Raw title fallback retains exact palette decode plus arithmetic dithering.

The user accepted Vita-tested `508bf75` as effectively free relative to bilinear in tested gameplay. Full-screen menus remain below 30 fps with both modes; no universal zero-cost or 30 fps menu claim is made. See `docs/DITHER_PERFORMANCE.md`.

The text layer is reconstructed at Saturn logical resolution and composited by SGX rather than expanded directly into the final framebuffer by the CPU.

Trilinear filtering is not currently used. These UI textures have a single mip level, so mip filtering would not improve presentation until a real mip chain exists.

Raw VDP2 image planes remain a separate presentation class from text/UI. The title already uses a decoded layer surface before final composition, preserving exact VRAM/CRAM reads while allowing SGX to filter decoded RGB pixels. The upcoming D5/name-entry work will extend the generic Neptune VDP2 architecture rather than introduce a screen-specific image path.
