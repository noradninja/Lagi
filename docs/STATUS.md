# Lagi Development Status

Current milestone: **0.030-alpha**

Last updated: 2026-10-03

Lagi is a native PlayStation Vita runtime for *Panzer Dragoon Saga* / *Azel*. Reconstructed Saturn-era game logic executes directly on ARMv7, with rendering and platform behavior translated to native VitaSDK/SceGxm.

## Development stage

Version 0.030-alpha builds Cinepak playback services on the hardware-proven first-Ruins and Neptune VDP2/VDP1 runtime slice. The current runtime drives the scene through Azel's real town tasks, scripts, player state, collision, camera, lock-on logic, dynamic objects, VDP1 command generation, and VDP2 text/window state. The playable sequence reaches the elevator decision and fades to the movie handoff; Azel remains responsible for invoking and sequencing that handoff.

The first-Ruins execution path is:

```text
Disc 1
  -> COMMON.DAT / TWN_RUIN resources
  -> town bootstrap
  -> tasks + scripts
  -> world grid / cells
  -> static + task-owned objects
  -> collision / Edge / camera / visibility
  -> processed models + VDP1 commands + VDP2 state
  -> Lagi / Neptune presentation translation
  -> SceGxm
```

## 0.030-alpha Phase 1 Cinepak path

Build-validated on 2026-10-03. Real Vita movie/audio playback remains to be
validated before this path can be called hardware-proven.

- Azel remains the movie-sequencing owner through a thin Lagi movie-backend
  bridge. The backend does not choose movies, transitions, fades, or scenes.
- Sega FILM parsing covers FDSC/STAB metadata, bounded sample access,
  per-stream PTS/duration, Cinepak keyframe flags, planar signed PCM, and the
  container's independent video/audio clocks.
- CPK samples stream from either a normal file or Lagi's mounted MODE1/2048 or
  MODE1/2352 BIN/CUE image instead of loading a complete movie into memory.
- The CPU Cinepak reference decoder supports persistent strip codebooks,
  full/partial 4-byte and 6-byte codebook chunks, V1/V4 vectors, interframe
  skipped blocks, strip inheritance, and Sega's 2/6-byte header variants.
- Decoded frames use a dedicated Neptune/GXM movie texture. When no movie is
  active, the existing VDP1 UI, VDP2 text/window/matte, MSAA, town fade, and
  resident-scene rendering paths are unchanged.
- Native SceAudioOut PCM playback runs through Lagi's platform audio backend
  on a dedicated producer/consumer thread. Movie code does not call Vita audio
  APIs directly.
- The decode/present contract is stable so Phase 2 can replace CPU pixel
  reconstruction with SGX-assisted reconstruction without changing FILM
  demux, timing, audio, or Azel sequencing ownership.
- Deterministic host tests cover FILM metadata/sample extraction, V1 and V4
  reconstruction, skipped interframes, and Sega's short header variant.

## Hardware-proven runtime

Verified on real Vita/Vita TV hardware:

- Vita process/platform startup
- persistent logging
- CUE/BIN access
- MODE1/2352 and MODE1/2048 support
- ISO9660 traversal
- `COMMON.DAT` parsing
- town overlay/resource loading
- direct development boot into `TWN_RUIN`
- Azel fixed-point support
- Azel heap/task runtime
- Saturn-addressed memory helpers
- native SceGxm renderer
- script-driven fade-in from black
- Vita-to-Saturn physical controller bridge
- town LCS / Lock-On state and target selection
- native GXM translation of VDP1 scaled-sprite and polyline UI commands
- native GXM translation of VDP1 normal-sprite UI commands
- VDP2 area-name, item-pickup, interaction, subtitle, and choice text
- GPU-rendered NBG1 framed windows
- line-scroll-driven lower cinematic matte
- animated elevator-choice selector using the original resident menu sprite data
- elevator choice and script-driven fade to the FMV handoff
- 30 Hz presentation

## Town systems

### Tasks and scripts

The first Ruins scene is driven by the town task/script structure rather than by a standalone room viewer.

Active pieces include:

- town root/task lifecycle
- town script task
- world-grid and cell ownership
- static object submission
- task-owned dynamic object submission
- live render bridge from processed models to the Vita renderer

### Collision

The current collision path uses the recovered town collision data and the `processTownMeshCollision()` pipeline.

Implemented pieces include:

- collision mesh decoding
- collision body registration
- Edge collision body
- room/floor collision used by the current scene

Body-to-body interaction remains incomplete.

### Edge

The current Edge path includes:

- town-owned position and yaw
- movement
- walk/run/idle animation selection
- animation stepping and interpolation
- transformed geometry and normals
- collision body registration
- live renderer submission

### Camera

The first Ruins scene uses Azel's town camera and camera-state transitions. Vita analog input is normalized at the platform boundary and then consumed by Azel's original movement/camera logic.

### LCS / Lock-On

Town Lock-On is now driven by the upstream Azel LCS path.

Current hardware-proven behavior includes:

- A/C entering Lock-On mode
- B cancelling Lock-On
- B acting as the walk-mode run modifier outside Lock-On
- automatic target acquisition near interactable Ruins objects
- target selection while retaining the independent white free cursor
- separate selected-target NEAR marker
- shrinking VDP1 polyline selection rectangle
- original VDP1 cursor/marker texture descriptors translated to native GXM

The Vita backend does not recreate LCS gameplay logic. Azel produces the physical-button interpretation, target state, camera behavior, and VDP1 commands; Neptune translates those commands to efficient screen-space GXM primitives.

### Town text, windows, and choices

The current first-Ruins sequence uses Azel's live VDP2 and VDP1 state rather than Vita-authored replacements.

Hardware-proven presentation includes:

- the `Ruins - Bottom Floor` area banner
- item-pickup text
- object-interaction text
- subtitle/dialog text
- blue framed NBG1 windows rendered from the original 16x16-character map
- the animated lower cinematic matte driven by Azel's vertical line-scroll table
- the `Ride the Elevator` / `Don't Ride` choice box
- the original animated VDP1 selection arrow

Direct boot restores VDP2 initialization and services queued VDP2 transfers at the normal frame boundary. It also loads `MENU.CGB` at the VDP1 address where the full startup path leaves it resident. Neptune then composes the VDP2 backing, VDP1 selector, and text in the original layer relationship. Choice state, text, animation, and fade timing remain upstream-Azel-owned.

### Ruins lock/switch objects

The first task-owned Ruins lock/switch objects are functional.

Current support includes:

- creation from cell data
- task-owned state and transform
- collision registration
- native disable/wait script entry points
- live material and texture resolution
- authentic task-owned world transforms passed to Neptune without double-applying the town camera

The scene currently allows the locks to be targeted and selected. Their later activation/translation depends on normal game progression state, including acquisition of the required weapon/item state.

## Renderer

### Resolution and presentation

```text
internal GXM render: 480x272
multisampling:       native 2x MSAA, hardware resolved
display output:       960x544
presentation target:  30 Hz
```

### Scene modes

```text
Full
Texture
Lighting
Quads
Wires
```

- **Full** uses textures and the active Gouraud path.
- **Texture** shows the scene without Gouraud contribution.
- **Lighting** shows the lighting field by itself.
- **Quads** shows filled polygon topology.
- **Wires** draws the four original edges of each Saturn quad rather than the internal GXM triangle diagonal.

The Basic Wing/dragon views remain in code as regression/reference paths but are not part of the normal runtime mode cycle.

### Texture/material path

The live Ruins renderer resolves materials and textures for:

- static room geometry
- task-owned dynamic objects
- Edge
- Edge's shadow

Texture mode uses the same live-town visibility set as the other scene views.

### VDP2 UI presentation

The first Ruins VDP2 path currently translates:

- the NBG3 text/font map and live palettes
- the NBG1 framed-window map through a cached GPU tile atlas
- the vertical line-scroll table used for the lower cinematic matte

VDP1 bank-color UI sprites use live CRAM, while town materials retain their validated bundle-relative texture path. This keeps transient UI palette state from changing world-material resolution.

### Multisample antialiasing

Neptune now creates both full- and half-resolution render targets in `SCE_GXM_MULTISAMPLE_2X` mode. Every fragment-program variant is patched for the same mode, color surfaces use GXM's MSAA downscale/resolve path, and depth/stencil storage is allocated at sample resolution.

The complete Vita package builds with this configuration. Updated on-device image-quality and timing measurements remain to be recorded.

### Gouraud lighting

The exact projected-perimeter/four-edge fragment implementation remains available as a reference.

The active `Full` path uses a cached 3x3 subdivision approximation:

```text
Saturn quad
 -> 9 unique subdivision vertices
 -> 8 GXM triangles
 -> four-corner lighting field
 -> RGB555 add / clamp / quantize
```

Cached data includes:

- subdivision topology
- UVs
- static positions

Dynamic polygons refresh the changing position/shade data.

### Edge VDP1 mesh shadow

Edge's original shadow is now reproduced from the game data:

1. The shadow model key is read from Edge's animation table.
2. The model is loaded from `COMMON3.MCB`.
3. The texture/mask is decoded from `COMMON3.CGB`.
4. Texture alpha provides the oval silhouette.
5. `CMDPMOD` mesh mode provides alternating-pixel coverage.
6. The primitive is rendered two-sided.
7. Ordered VDP1-style draw behavior is used so the shadow remains visible over the floor: the mesh shadow is submitted after the environment with depth test forced to pass and depth writes disabled, matching the relevant Saturn draw-order behavior.

## Performance

The pre-MSAA scene was stable at the 30 FPS presentation target. The presentation target remains 30 Hz, but the new MSAA configuration still needs an updated hardware timing capture.

Captures taken before 2x MSAA was enabled showed roughly:

```text
20-23 ms render work
```

before the deliberate 30 Hz presentation wait.

That corresponds to approximately 44-50 FPS of render throughput if uncapped. This is a pre-MSAA baseline, not a current measured cost.

## Remaining work

Major systems still incomplete or not yet integrated include:

- broader town object coverage
- additional script behavior
- additional VDP1 UI command types beyond the current normal-sprite, scaled-sprite, and polyline subset
- some collision interactions
- Ruins sound/effects
- connecting the elevator script's movie handoff to the Azel-owned sequencing call
- on-device Cinepak video/audio timing and drain validation
- broader UI/state flow beyond the first Ruins sequence
- broader VDP2 background and compositing behavior beyond the current text/window/matte subset
- battle systems
- broader field systems
- remaining movies, menus, save flow, and complete game progression

Renderer-side technical debt currently falls into two concrete areas:

- **Material-cache lifetime/invalidation.** The live town material cache currently keys resolved polygon-to-texture mappings by model identity and polygon count. That is sufficient for the present Ruins scene, where resources are effectively stable, but it is not yet tied to a scene/resource generation. After town transitions or bundle reloads, pointer reuse could make an old cache entry appear valid for a different model with the same shape. Failed resolutions also should not become permanent cache state. Before frequent town transitions are enabled, the cache needs explicit invalidation on town/resource teardown or a stronger key that includes stable bundle/model identity plus a generation/version.
- **Batching and 16-bit index limits.** The current live renderer flattens the active town into shared CPU/GPU buffers and uses 16-bit indices. The 3x3 Gouraud subdivision expands each Saturn quad to 9 vertices and 24 indices, so large towns can exhaust a single 65,535-vertex/index address space much sooner than the original model data would. The current Ruins room fits comfortably, but broader towns will need the renderer to split work into multiple resident batches—most likely per model, object group, or world-grid cell—while preserving Azel's original draw ordering, material state, visibility decisions, and dynamic-object updates.

## Historical reference paths

The earlier Basic Wing work remains useful for:

- VDP1 texture decoding
- hierarchy/hotpoint validation
- animation decoding
- Gouraud/RGB555 comparison
- shader bring-up
- GXM resource validation

It is no longer the primary development target.
