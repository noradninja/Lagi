# Lagi Development Status

Last updated: 2026-09-28

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

## Viewer controls

- SELECT: toggle status console / Basic Wing viewer.
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
