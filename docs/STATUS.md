# Lagi Development Status

Last updated: 2026-09-29

Lagi is a native PlayStation Vita reimplementation path for Panzer Dragoon Saga built around reconstructed Azel/ATOLM game logic. The current project executes reconstructed Saturn-era game/data logic directly on ARMv7 and translates the original rendering intent to native VitaSDK/SceGxm rather than emulating the Saturn CPUs or using a generic modern renderer.

## Current hardware-proven baseline

The following paths have been verified on real PS Vita hardware:

### Runtime and game-data integration

- Vita process/platform startup.
- Native 960x544 framebuffer/display operation.
- Azel fixed-point runtime.
- Azel heap allocator.
- Azel hierarchical task scheduler.
- Root task creation and Update/Draw execution.
- Big-endian Saturn memory readers and `sSaturnPtr` traversal.
- Persistent hardware logging to `ux0:data/lagi/lagi.log`.
- Direct Disc 1 CUE/BIN reading from `ux0:data/lagi/Disc 1/`.
- MODE1/2352 and MODE1/2048 data-track handling.
- ISO9660 filesystem traversal inside the disc image.
- `COMMON.DAT` loading directly from the mounted PDS disc image.
- 9 dragon-level stat tables parsed.
- 27 battle overlay descriptors parsed.
- 27 battle activation entries parsed.
- 79 sound configuration records parsed.
- Dragon COMMON morph/model/animation metadata parsed.

### Basic Wing reconstruction

- `DRAGON0.MCB` hierarchy correlated against `COMMON.DAT`.
- 31-bone Basic Wing hierarchy.
- 6 decoded hotpoints.
- 31 model nodes.
- 300 source model vertices.
- 212 original Saturn VDP1 quads.
- Original VDP1 polygon command data retained:
  - `lightingControl`
  - `CMDCTRL`
  - `CMDPMOD`
  - `CMDCOLR`
  - `CMDSRCA`
  - `CMDSIZE`
- Original per-polygon lighting payload retained.
- Basic Wing hardware data currently resolves to lighting mode 3 for all 212 quads: four normals per original Saturn quad.

## Current native GXM renderer

The standalone Basic Wing viewer is now a hardware-proven Saturn-faithful rendering reference path.

### M1 reusable VDP1 submission layer

The hardware-proven Basic Wing path has been refactored so the viewer is now a regression client of a model-agnostic VDP1 submission interface rather than owning the draw logic directly.

Current M1 interface:

- `Vdp1ModelSource`: non-owning view of triangulated Saturn model vertices, original quad metadata, decoded mode-1 textures, polygon texture indices, and four-corner RGB555 Gouraud values.
- `Vdp1DrawState`: WVP matrix plus the existing renderer mode.
- `prepare_vdp1_model()`: uploads the source into the native Vita VDP1 GPU resource slot.
- `submit_vdp1_model()`: binds the proven shader/state path and submits the model while preserving original quad identity for Gouraud reconstruction.
- Basic Wing-specific camera, input, morph-screen animation clock, and test lighting remain outside the reusable submission function.

M1 intentionally keeps one resident prepared model at a time. This is sufficient for the Basic Wing regression viewer and for the first live Azel-model integration. Multi-model residency/batching is deferred until the actual Azel scene render boundary is connected, so the proven renderer is not destabilized prematurely.

### Output and presentation

- Native SceGxm rendering.
- 960x544 Vita output.
- Double-buffered GXM color surfaces and sync objects.
- `SCE_DISPLAY_SETBUF_NEXTFRAME` presentation.
- Fixed 30 Hz viewer presentation cadence.
- Vita scanout remains 60 Hz, but the application presents one game/render frame every two display intervals.
- Camera/input behavior is therefore consistent between cheap and expensive viewer modes.

### Camera

- Perspective projection.
- 50-degree vertical FOV.
- Native 960/544 aspect ratio.
- Near/far planes: 0.1 / 100.
- Left stick: yaw/pitch.
- Right stick Y: camera dolly.
- Camera range: 0.75 to 8.0.
- Default distance: 3.0.
- Triangle resets yaw, pitch, and distance.

## Saturn VDP1 texture path

The textured renderer uses the original `DRAGON0.CGB` data rather than replacement artwork.

Implemented and hardware-proven:

- Pre-relocation `CMDSRCA << 3` texture addressing directly into `DRAGON0.CGB`.
- Pre-relocation `CMDCOLR << 3` 16-entry LUT addressing.
- Basic Wing color mode 1 decoding.
- Two 4-bit texels per source byte.
- High nibble then low nibble order.
- SPD transparent-pixel behavior.
- VDP1 end-code handling.
- Direct RGB555 LUT colors.
- 71 unique decoded Basic Wing textures.
- Polygon-to-texture mapping.
- All four `CMDCTRL` texture-flip orientations.
- Half-texel UV endpoints.
- Point/nearest texture filtering.
- No mipmapping.
- Transparent texel discard.
- Linear A8B8G8R8 Vita texture upload.

The source texture colors remain Saturn RGB555-derived even though the Vita texture and framebuffer storage are RGBA8888.

## Saturn Gouraud lighting path

The current lighting implementation is based on pinned Azel behavior rather than conventional modern vertex lighting.

### Original quad semantics

The Saturn polygon is treated as one four-corner primitive even though GXM ultimately submits:

```text
0,1,2
0,2,3
```

Using ordinary triangle interpolation produced a visible diagonal lighting wedge across broad Saturn quads.

Lagi now:

1. Projects the original four Saturn corners.
2. Reconstructs the original quad coordinate in the fragment shader with inverse bilinear mapping.
3. Bilinearly interpolates all four Saturn Gouraud corner values.
4. Applies the resulting lighting across the original quad rather than independently across the two GXM triangles.

This preserves the characteristic Saturn quad-wide Gouraud behavior while using triangle hardware underneath.

### Azel-style RGB555 Gouraud representation

Pinned Azel converts lighting to a 5-bit value and stores each channel as a signed additive offset:

```text
offset = (gouraud5 - 16) / 31
```

Lagi preserves that representation.

The lit fragment path performs:

```text
original RGB555 texture color
        +
bilinearly interpolated signed Gouraud RGB offset
        ↓
clamp to Saturn 0..31 channel range
        ↓
quantize to integer RGB555
        ↓
expand to RGBA8888 only for Vita output
```

The final 5-bit quantization is intentional. Visible color/lighting banding is part of the target appearance; a smooth 8-bit gradient is considered incorrect for this Saturn-faithful path.

## Current standalone light

The current Basic Wing viewer reproduces the dragon morph-screen light defaults found in pinned Azel:

```text
setupLight(0, 0, 0x10000, 0x161918)
generateLightFalloffMap(0x030102, 0, 0)
```

Implemented:

- Azel light-vector sign convention.
- Morph-viewer RGB light color ordering.
- Exact 32-entry quadratic falloff-map generation.
- Per-corner transformed model normals.
- Camera-relative light behavior for the standalone orbiting viewer.
- Per-frame RGB555 Gouraud regeneration.

The viewer's mapping from its free-camera view depth into the morph-screen falloff domain remains a standalone adaptation. Live field/battle camera and light state will replace this when the renderer is connected to game scenes.

## Morph-screen animation

The Basic Wing viewer decodes and plays the default dragon morph-screen animation selected by pinned Azel:

```text
dragonAnimOffsets[0] -> DRAGON0.MCB table entry 0x10C
```

Implemented:

- Native animation header parsing.
- Per-bone track decoding.
- Supported Azel animation update modes 0, 1, 3, 4, and 5.
- Rebuilding the 31-bone hierarchy for decoded animation poses.
- Animated geometry.
- Animated transformed lighting normals.
- Predecoded viewer animation frames.
- Independent exact 30 Hz animation clock based on Vita process time.
- Catch-up by elapsed animation ticks rather than slowing animation when rendering becomes expensive.

The animation therefore retains its intended playback speed even if a rendered frame takes longer.

## Viewer modes

Current mode cycle:

```text
Mode 0 — original decoded Saturn texture baseline
Mode 1 — texture + quad-bilinear RGB555 Gouraud lighting
Mode 2 — same RGB555 Gouraud result shown as grayscale
Mode 3 — polygon debug colors
Mode 4 — wireframe
```

Culling:

- Modes 0-3: `SCE_GXM_CULL_CW`
- Mode 4: `SCE_GXM_CULL_NONE`

CW is the hardware-validated effective front-face winding for Lagi's current row-vector WVP/GXM viewport convention.

Wireframe intentionally remains two-sided so hidden/back-facing topology remains visible for debugging.

Depth:

- Filled modes: LESS_EQUAL.
- Wireframe: strict LESS to avoid competing equal-depth shared edges.

## First ruin room geometry bring-up

M3C now reconstructs the first validated 1x1 ruin town cell directly from `TWN_RUIN.PRG` and the town model bundle selected by that setup.

For this diagnostic stage, Lagi:

- resolves the sole grid cell through the overlay's grid EA,
- reads the static-object list at cell + `0x0C`,
- walks the original 0x18-byte object records,
- uses the first LOD model entry, matching the current town default depth threshold,
- loads the referenced MCB bundle through the original bundle offset table,
- decodes Azel processed-model vertices, quads, VDP1 command metadata, and lighting payload,
- applies each object's Saturn translation and 12-bit Z/Y/X rotation,
- flattens the static room into a geometry-only `Vdp1ModelSource`,
- normalizes the aggregate bounds only for this diagnostic viewer.

Viewer mode 5 displays this real room geometry in polygon-color mode. Modes 0-4 remain the hardware-proven Basic Wing regression modes. L/R now cycle across all six modes when the room mesh is available.

Textures, live Azel camera state, scripts, player/NPC tasks, LCS, audio, and VDP2 are intentionally not part of this first static-room visualization.

## First town overlay preflight

M3B now loads the resolved first-scene town overlay directly from Disc 1 into a Saturn-addressed `sSaturnMemoryFile` at the canonical overlay base `0x06054000`.

For the current Disc 1 target, Lagi validates the same initial TWN_RUIN structures used by pinned Azel:

- town setup at `0x0605E984`,
- initial script entry at `0x06054398`,
- edge data at `0x0605E990`,
- town-grid setup and grid EA reached through the setup structure,
- 12-entry town script pointer table,
- environmental LCS target metadata.

It also verifies the immediate ruin asset set is present before town execution begins: `COMMON3.MCB/CGB`, `RUINMP.MCB/CGB`, `RUINSCR.SCB/PNB`, and `EVTRUIN.FNT`.

This stage deliberately stops before constructing Azel's town tasks. The next stage can therefore port the task/grid/matrix execution path against a hardware-verified overlay and asset set rather than treating the town loader as a single large dependency jump.

## First-scene direct boot target

The development boot target is now pinned to Azel game status `0x04`, the first 3D scene reached after the startup movie, D5 name-entry state, and second movie.

Pinned Azel's module-manager table resolves the startup chain as:

```text
0x01 -> movie 0
0x02 -> FLD_D5 name entry
0x03 -> movie 1
0x04 -> game mode 1, town entry 0x10
```

Lagi does not hard-code the resulting town overlay filename. At startup it resolves town entry `0x10` through the same COMMON.DAT dispatch tables used by Azel's `loadTownSub()` / `loadTownPrg()`, then verifies that the resolved overlay exists on Disc 1. The debug screen reports the resolved direct-boot target.

This is the M3A boundary before importing the full module/town runtime. The next stage will instantiate the minimum town/module execution spine needed to enter this resolved target, while skipping the preceding movie/name presentation states only in development mode.

## Azel live-render integration boundary

M2 tracing identified Azel's native 3D submission seam at `addObjectToDrawList()` / `addBillBoardToDrawList()`. At that point Azel has already selected an `sProcessed3dModel` and, in the desktop backend, captures the current model matrix, light vector/color, local screen offset, and billboard state before placing the object into the renderer queue.

Lagi now provides Vita-side definitions for those submission symbols through `azel_render_bridge.cpp`. The bridge records per-frame live model submissions, captures the current Azel model matrix and light vector/color when those full-engine globals are linked, and converts the submitted `sProcessed3dModel` CPU data into an owned `LiveVdp1Model`.

The live adapter currently preserves:

- original indexed model vertices,
- original quad identity,
- `lightingControl`,
- `CMDCTRL`,
- `CMDPMOD`,
- `CMDCOLR`,
- `CMDSRCA`,
- `CMDSIZE`,
- per-quad/per-corner normal payload,
- lighting-mode-2 per-corner color payload.

Each Azel quad is expanded to the same six-vertex `0,1,2 / 0,2,3` representation consumed by the proven Vita VDP1 path. The renderer now accepts geometry-only `Vdp1ModelSource` objects so this live path can be brought up first in polygon/debug form.

Live Azel texture memory is deliberately not connected yet. Desktop Azel decodes processed-model texture commands against emulated VDP1 VRAM/VDP2 CRAM; Lagi still needs a native equivalent of that live memory source. Until that is connected, the adapter leaves the live texture set empty rather than substituting incorrect artwork or reusing the DRAGON0 viewer texture assumptions.

## Viewer controls

- SELECT: toggle status console / Basic Wing viewer.
- L: previous viewer mode.
- R: next viewer mode.
- START + SELECT: exit.
- Left stick: rotate.
- Right stick Y: dolly.
- Triangle: reset camera.
- L / R: cycle viewer mode.

These are development controls and are separate from the eventual Saturn input mapping.

## Current comparison with Saturn rendering

The Vita is not emulating the VDP1 rasterizer cycle-by-cycle. Instead, SceGxm is used to reproduce the rendering rules that materially define the current PDS object path:

- original Saturn texture data,
- nearest-neighbor sampling,
- original quad identity,
- four-corner Gouraud data,
- quad-wide interpolation instead of triangle-diagonal interpolation,
- signed additive Gouraud color calculation,
- RGB555 output limits and visible banding,
- Saturn-derived face visibility,
- 30 Hz game-style presentation.

The hardware implementation is native Vita GPU rendering, but its visible constraints are deliberately Saturn-like.

## Known issue

One isolated stray triangle remains visible in the Basic Wing model. The rest of the mesh, texture path, culling, and lighting are stable, so this is currently treated as a likely model/reconstruction-data issue rather than a renderer-wide winding problem.

It is intentionally deferred.

## Historical GXM bring-up summary

The native renderer was brought up incrementally on hardware:

1. `sceGxmInitialize()`
2. ring/USSE allocation and mapping
3. context creation
4. render-target creation
5. color/depth/sync surfaces
6. shader patcher
7. GXP validation/registration
8. patched vertex/fragment programs
9. empty scene submission
10. off-screen triangle draw
11. triangle display scanout
12. reconstructed Basic Wing draw
13. double-buffered interactive viewer
14. perspective camera
15. Saturn texture decoding/upload
16. lighting payload preservation
17. RGB555/quad Gouraud reconstruction
18. CW culling validation
19. morph-screen lighting
20. morph-screen animation
21. fixed 30 Hz animation and presentation

These stages are historical validation milestones, not current configuration alternatives.

## Next development direction

The standalone Basic Wing viewer is now the graphics reference implementation.

Next work should move beyond viewer-only rendering and begin reconnecting the proven pieces to live PDS state:

- reusable model submission,
- live animation state rather than predecoded viewer playback,
- live scene camera matrices,
- live PDS light vector/color/falloff,
- VDP1 transparency/color-calculation modes,
- VDP2 layer/compositing behavior,
- broader field/battle object rendering.

The immediate priority is to preserve the now-proven Saturn visual behavior while moving it into real game execution rather than replacing it with a more conventional rendering path.


## Vita presentation assets

The current VPK build now includes the repository's custom Vita shell assets:

- `sce_sys/icon0.png`
- `sce_sys/livearea/contents/bg0.png`
- `sce_sys/livearea/contents/startup.png`
- `sce_sys/livearea/contents/template.xml`

The LiveArea template uses `bg0.png` as the background. `startup.png` is intentionally a fully transparent 280x158 indexed PNG so the gate artwork is invisible; any residual focus/selection outline is Vita system UI rather than app artwork. These assets are packaged directly by `vita_create_vpk()`.

## Debug-screen convenience

- Successful GXM initialization now collapses to one status entry: `[PASS] GXM INITIALIZATION + VDP1 READY`.
- GXM failures still surface individually.
- Status output wraps into a second column after 32 rows instead of running off the bottom of the screen.
