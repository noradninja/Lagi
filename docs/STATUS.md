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

## First ruin room texture bring-up

### Ruin command-4 solid polygons

Hardware logging showed the 16 previously unresolved "mode 0" records are not textured sprites at all. They are VDP1 command-4 polygons:

```text
CMDCTRL & 0xF = 4
CMDSRCA = 0
CMDSIZE = 0
CMDCOLR = 0x8000
```

Pinned Azel dispatches command 4 to `PolyDrawGL()`, which uses `CMDCOLR` directly as RGB555. Lagi now classifies these separately and represents their flat color as a synthetic 1x1 material so the reduced room can stay on a single GXM textured submission path. They no longer count as unresolved texture descriptors.

### Ruin mode-0 palette resolution

Pinned Azel's `ruinBgInit()` shows that the ruin scene copies 0x200 bytes from `TWN_RUIN.PRG:0x0605EBF8` into `vdp2Palette`. In Azel, `vdp2Palette` is CRAM byte offset `0xC00`, so this supplies CRAM palette indices `0x600-0x6FF`.

The reduced room decoder now uses that exact overlay palette for VDP1 color mode 0:

```text
4bpp texel -> dot
palette index = CMDCOLR | dot
palette byte offset = palette index * 2
0xC00-0xDFF -> TWN_RUIN:0x0605EBF8 + (offset - 0xC00)
```

Mode-0 descriptors are also logged with object/polygon IDs and VDP1 command words so any remaining mismatch can be diagnosed from hardware data without weakening the polygon-color fallback.


The M3C room diagnostic now attempts to bind the original `RUINMP.CGB` texture data to the reconstructed static room.

Because the room models are decoded from their unpatched MCB command words, the texture addressing follows the same hardware-proven rule as the Basic Wing path:

```text
CMDSRCA << 3 -> byte offset in RUINMP.CGB
CMDCOLR << 3 -> 16-entry LUT offset for VDP1 mode-1 polygons
```

The reduced room decoder currently supports:

- VDP1 color mode 1 (4bpp LUT), including SPD and end-code behavior,
- direct RGB555 LUT entries,
- VDP1 color mode 5 direct RGB555 texture data.

If a LUT entry references VDP2 CRAM, or a room polygon uses a bank-color mode that requires live CRAM, Lagi records that dependency and keeps Mode 5 on the proven polygon-color fallback rather than presenting a partially incorrect textured scene.

When every room texture is resolved from the CGB alone, Mode 5 automatically switches to the native point-filtered textured VDP1 path. The debug screen reports the decoded texture count and `lagi.log` records the room color-mode distribution.

## Ruin cell-space world transform correction

The first static-room reconstruction originally applied each 0x18-byte object's local translation/rotation directly to its model vertices. That produced a coherent room but placed the entire room several world units away from Edge.

Pinned Azel's actual town draw path applies one additional transform first:

```text
sWorldGridCellTask::Init:
    mC_position = readSaturnVec3(cell)

sWorldGridCellTask::Draw:
    translateCurrentMatrix(cellPosition)
    for each static object:
        generateObjectMatrix(objectTranslation, objectRotation)
        addObjectToDrawList(model)
```

Lagi now applies the cell's 16.16 world translation before each object's local transform. This is source-derived and awaits hardware confirmation. It should bring the static ruin room into the same world-space frame as Edge and the recovered initial town camera without changing FOV, model scale, or clip distances.

## Azel town grid visibility + LOD hook

The authentic ruin path now follows the normal Azel town submission hierarchy instead of using a custom object-frustum/AABB system.

For the active town cell, Lagi retains:

```text
cell world origin
grid cell size
gWorldGrid.m2C coarse cell radius
town LOD depth thresholds
per-object world origin
per-object LOD model table entries
```

At runtime, mode 7 mirrors `sWorldGridCellTask::Draw()`:

```text
translate cell into camera space

reject if:
    cellZ < nearClip - cellRadius

horizontalLimit =
    cellZ * widthRatio
    + cellRadius * widthRatio2

reject if:
    cellX < -horizontalLimit
    or cellX > +horizontalLimit
```

The `widthRatio` and `widthRatio2` values are derived from the same Saturn VDP1 projection state as `initVDP1Projection()`, rather than from Vita/NDC clip-space.

If the cell survives, each static object's camera-space depth runs the same LOD threshold walk used by Azel:

```text
lod = 0
while objectDepth > gTownGrid.m3C[lod]:
    ++lod
```

`TWN_RUIN` leaves the default town threshold `0x7FFFFFFF` in place, so all 17 current static objects resolve to LOD 0. This means the visible geometry for the first ruin room remains unchanged, but the runtime visibility/LOD path is now structurally correct for later towns that provide real depth thresholds.

The earlier experimental per-object AABB frustum scaffolding was removed; it is not part of Azel's normal town draw path.

## No-sqrt Gouraud experiment: one-step Newton inverse

The exact Mode 7 path remains unchanged as the visual reference.

A new authentic-camera Mode 10 uses the same:

```text
960x544 render target
town camera/projection
Azel cell visibility
texture batches
RGB555 Gouraud values
four-corner bilinear lighting equation
```

but replaces the closed-form inverse-bilinear quadratic solve with a no-sqrt iterative solve.

For each fragment:

```text
1. Solve the affine approximation:
       p ~= a + e*u + f*v

2. Use that (u,v) as the initial estimate.

3. Perform one Newton correction on:
       P(u,v) = a + e*u + f*v + g*u*v

   Jacobian columns:
       dP/du = e + g*v
       dP/dv = f + g*u

4. Clamp the corrected (u,v) to the unit square.

5. Use the same four-corner Gouraud interpolation and RGB555
   add/clamp/quantization as Mode 7.
```

This removes the fragment-stage quadratic discriminant, square root, root selection, and associated exact-solver control flow. The experimental path uses only 2x2 cross products and reciprocal-based linear solves.

Mode mapping for hardware comparison:

```text
7  exact inverse-bilinear Gouraud
8  textured only
9  flat color
10 one-step Newton Gouraud, no sqrt
```

Mode 10 is intentionally experimental until screenshot and FPS comparison against Mode 7 is hardware-validated.

## Gouraud vs RGB555 fragment isolation

Modes 13 and 14 split the remaining lit fragment workload after Mode 12 showed that removing inverse-coordinate recovery alone did not restore 30 FPS.

Mode 13 keeps:
- heavy Gouraud payload interface
- texture sample + alpha test
- four-corner RGB Gouraud interpolation
- float-domain texture + lighting add/clamp

Mode 13 removes:
- RGB555 recovery from the sampled texture
- 5-bit clamp/round/floor quantization

Mode 14 keeps:
- heavy Gouraud payload interface
- texture sample + alpha test
- RGB555 recovery/add/clamp/floor/quantization

Mode 14 removes:
- four-corner Gouraud interpolation

For Mode 14, a single per-quad Gouraud corner value is used as the constant lighting term so the RGB555 path remains active without bilinear color interpolation.

Both modes keep otherwise-unused inverse payload values live through a negligible output dependency so the compiler cannot collapse the interpolator/register footprint.

Hardware comparison target:

```text
Mode 11  heavy payload + texture only                     30 FPS
Mode 12  Gouraud + RGB555, no inverse solve              20 FPS
Mode 13  Gouraud interpolation, no RGB555 quantization    ? FPS
Mode 14  RGB555 quantization, no Gouraud interpolation    ? FPS
```

Interpretation:

```text
13 = 20, 14 = 30  -> Gouraud interpolation is dominant
13 = 30, 14 = 20  -> RGB555 conversion/quantization is dominant
13 = 20, 14 = 20  -> either path alone exceeds the fragment budget
13 = 30, 14 = 30  -> their combined cost crosses the SGX fragment budget
```

## No-inverse Gouraud arithmetic probe

Mode 12 isolates the cost of Gouraud/RGB555 fragment arithmetic from the cost of recovering bilinear quad coordinates.

It uses the same authentic-camera room path, heavy Gouraud payload interface, texture sampling, alpha test, four-corner Gouraud values, RGB555 add/clamp, and final 5-bit quantization as the lit path.

The only deliberate shortcut is the interpolation coordinate source:

```text
Mode 7/10:
    recover quad-local (u,v) from screen position

Mode 12:
    st = saturate(vTexcoord)
```

The texture coordinates are therefore used directly as a cheap diagnostic interpolation coordinate. This is not intended to be visually correct on flipped or distorted quads; its purpose is strictly to measure the fragment cost of the remaining Gouraud and RGB555 arithmetic with all inverse-mapping work removed.

The otherwise-unused inverse payload is kept live through a negligible output dependency so compiler dead-code elimination does not reduce the interpolator/register footprint.

Comparison target:

```text
Mode 11  heavy payload + texture only
Mode 12  heavy payload + texture + Gouraud/RGB555, no inverse solve
```

If Mode 12 remains at 30 FPS, inverse-coordinate recovery is the bottleneck. If it drops to 20 FPS, the remaining Gouraud/RGB555 arithmetic is the dominant cost.

## Heavy-payload texture-only probe

Mode 11 isolates fragment interpolator/register pressure from Gouraud arithmetic.

It uses the same authentic-camera room path, texture batching, Gouraud payload vertex shader, and complete fragment input interface as the lit modes:

```text
TEXCOORD0  texture UV
TEXCOORD1  inverse0
TEXCOORD2  inverse1
TEXCOORD3  inverse2
TEXCOORD4  Gouraud R
TEXCOORD5  Gouraud G
TEXCOORD6  Gouraud B
WPOS       fragment coordinate
```

The fragment shader performs only texture sampling and alpha test. All heavy payload inputs are deliberately kept live through a negligible output dependency so the shader compiler cannot optimize the diagnostic interface away.

Comparison target:

```text
Mode 8   simple texture pipeline
Mode 11  same texture result with heavy Gouraud payload interface
```

If Mode 11 falls to the lit-mode frame rate, the payload/interpolator pressure is the bottleneck. If Mode 11 remains at 30 FPS, the cost lies in the Gouraud/inverse mapping arithmetic rather than the payload itself.

## Authentic fragment-cost diagnostic ladder

Three authentic-camera ruin modes now share the same:

```text
960x544 framebuffer
world-space room geometry
initial scripted town camera
Azel town cell visibility
depth state
projection
static object set
```

Only the fragment/material path changes:

```text
Mode 7
  textured + Saturn four-corner Gouraud
  full inverse-bilinear fragment solve

Mode 8
  textured only
  existing simple texture fragment shader
  no Gouraud/inverse-bilinear work

Mode 9
  polygon color
  existing minimal probe fragment shader
  no texture sampling
  no Gouraud/inverse-bilinear work
```

These modes are intended to separate full Gouraud cost from texture/fill cost and raw geometry/depth/fill cost at the same camera position. The diagnostic ladder awaits hardware FPS comparison.

## Inverse-bilinear fragment optimization pass 2: root-division reduction

Pass 1 produced no measurable FPS change in the close-wall hardware case, strongly indicating the expensive portion is the quadratic/root solve rather than quad-invariant setup arithmetic.

Pass 2 keeps the same inverse-bilinear solution and exact fallback behavior, but removes two common-path fragment divisions:

```text
vertex stage:
    inv2k2 = 1 / (2 * k2)

fragment stage:
    v1 = (-k1 - sqrt(discriminant)) * inv2k2
    v2 = (-k1 + sqrt(discriminant)) * inv2k2
```

The fragment shader also tests whether exactly one root has `v` inside `[0,1]`. If that root's computed `u` is also inside `[0,1]`, it is already the valid inverse-bilinear unit-square solution and the shader returns immediately after one `computeU()` division.

Ambiguous, degenerate, or out-of-range cases fall back to the previous two-root `computeU()` and distance comparison. This preserves the prior selection semantics while reducing the normal convex-quad path from roughly four fragment divisions to one, with the square root still remaining.

This pass awaits hardware visual/FPS validation.

## Inverse-bilinear fragment optimization pass 1: coefficient reduction

The lit Saturn-quad shader now moves quad-invariant inverse-bilinear setup out of the fragment stage.

The CPU payload remains unchanged. The Gouraud payload vertex shader derives, per generated vertex:

```text
a = p0
e = p1 - p0
f = p3 - p0
g = p0 - p1 + p2 - p3

k2     = cross(g, f)
k1Base = cross(e, f) - cross(a, g)
k0Base = -cross(a, e)
```

Because all six generated triangle vertices for one Saturn quad carry the same original corner payload, these derived values are identical across the quad.

The fragment shader now computes only the pixel-dependent terms:

```text
h  = p - a
k1 = k1Base + p.x*g.y - p.y*g.x
k0 = k0Base + p.x*e.y - p.y*e.x
```

The quadratic root selection, `computeU()`, four-corner Gouraud interpolation, RGB555 add/clamp, and final quantization are unchanged. This is intended to be visually identical while removing repeated vector construction and quad-invariant cross products from every covered fragment.

This pass awaits hardware compile/visual/FPS validation.

## VDP1 Gouraud performance pass 2: texture batching

Pass 2 keeps the pass-1 vertex-carried quad payload unchanged, but removes the one-draw-per-Saturn-quad submission pattern.

Each frame the lit path now:

```text
project visible original quad corners
update replicated Gouraud payload
count visible indices per texture
prefix-sum texture batches
write one compact visible U16 index list
submit one draw per used texture
```

This preserves the exact inverse-bilinear four-corner Gouraud reconstruction and the full 960x544 render target. The optimization changes draw submission only; it does not alter lighting, texture decoding, projection, camera state, or output resolution.

The implementation also caches per-frame quad visibility so projected corners are not recomputed a second time while building the texture-grouped index list.

This pass is source-complete and awaits hardware FPS/visual validation before any object/frustum-culling work.

## VDP1 Gouraud performance pass 1: vertex payload state

The hardware-correct Saturn four-corner Gouraud path originally uploaded five fragment-uniform vectors per quad (two projected-corner vectors plus RGB corner vectors) and reserved a fragment default-uniform buffer for every polygon draw.

Pass 1 removes that per-draw fragment-uniform traffic without changing draw-call count. A dedicated Gouraud payload vertex format now replicates the quad's projected four corners and RGB555 Gouraud corner values across the six generated triangle vertices. A new vertex shader forwards that payload through TEXCOORD1..5 to the existing inverse-bilinear fragment logic.

The lit path therefore remains one draw per original Saturn quad for this pass, deliberately isolating the cost of fragment-uniform reservation/upload from later batching work.

No resolution reduction is part of this optimization. Lagi continues targeting the full 960x544 Vita framebuffer.

## Shared Azel game-space projection

All 3D diagnostic/viewer paths now preserve Azel's native coordinate scale end-to-end.

Processed model vertices are decoded from Saturn `sVec3_S16_12_4` as:

```text
raw s16 * 0x10 -> Azel 16.16 fixed point
float game unit = raw / 4096
```

Town/object translations remain 16.16 game-space values. Neither Basic Wing nor the reconstructed ruin room is normalized or rescaled for display anymore. Debug viewers fit content by moving the camera only.

The renderer now uses one shared `initVDP1Projection`-equivalent projection builder for every 3D mode. The default viewer state matches Azel's normal `DEG_80 / 2, mode 0` projection, including the separate Saturn X/Y scale factors derived from the 352x224 VDP1 viewport and the original `352/320` / `224/240` corrections.

A centered 4:3 output correction is applied at presentation time for the Vita's 16:9 framebuffer; this does not alter game-space geometry.

Debug camera behavior is now:

```text
Basic Wing modes 0-4:
  native Azel model scale
  orbit/dolly debug camera
  shared Azel projection

Ruin modes 5-6:
  native Azel town/world scale
  orbit/dolly debug camera
  shared Azel projection

Ruin mode 7:
  native Azel town/world scale
  recovered initial game camera
  shared Azel projection
  authentic ruin near/far clip
  depth-selected Saturn light falloff
```

This removes scene/model scaling as a variable when validating the initial town camera. If mode 7 is still outside the visible frustum, the remaining issue is camera orientation/placement or clipping rather than mismatched model units.

## Initial ruin town camera bring-up

### Startup script camera state correction

Hardware diagnostics showed the raw Edge definition at `0x0605E990` was not the state used by the first rendered follow camera. The initial ruin script calls, in order:

```text
setNpcLocation(0, 0x0000A800, 0x00000E3D, 0x00028B33)
setNpcOrientation(0, 0, 0x0FE93E90, 0)
setupCameraFollowMode()
townCamera_setup(...)
```

The script-updated Edge position is approximately `(0.65625, 0.05562, 2.54375)`, which lies inside the reconstructed ruin room's hardware-observed world bounds. Lagi now replays camera-relevant NPC location/orientation calls from the initial script up to `setupCameraFollowMode()` before deriving `scriptFunction_6057058_sub0Sub0()`'s camera.

This replaces the earlier raw-spawn camera assumption. The change is source-derived and awaits hardware confirmation.


The static room diagnostic now retains both normalized viewer geometry and untouched assembled town-space geometry. Modes 5/6 continue to use the normalized regression mesh; the new mode 7 uses original town-space coordinates.

Pinned Azel derives the first ruin camera from Edge's initial NPC transform at `0x0605E990` when `setupCameraFollowMode()` calls `scriptFunction_6057058_sub0Sub0()`:

```text
focus = Edge.position + (0, 0x1800, 0)
basis = rotate Y(Edge.yaw), then X(Edge.pitch)
camera = focus + basis.Z * 0x199
target = focus + basis.Z * -0x1000
up = camera + (0, 0x10000, 0)
```

Lagi now reconstructs that exact initial camera from the overlay and preserves the ruin near/far clip values (`0x800` / `0xF000`).

Mode 7 also matches Azel's Saturn VDP1 projection math rather than substituting a generic 80-degree perspective. For mode 0, `initVDP1Projection(DEG_80 / 2, 0)` computes separate normalized X/Y scales from the 352x224 VDP1 viewport and the original `352/320` and `224/240` factors. The Vita output applies a 4:3 display correction to X so the Saturn-authored geometry keeps its intended physical proportions on the 16:9 framebuffer.

For mode 7, lighting distance falloff is no longer pinned to table entry zero. Lagi computes view-space depth from the authentic world-space quad position and initial camera, then reproduces Azel's `computeViewDepth()` + `GetDistanceFalloff()` fixed-point table-index calculation.

The remaining presentation difference is horizontal clipping: the projection is corrected into the central 4:3 region, but GXM still clips against the full Vita viewport. A later viewport/scissor step can reproduce the original Saturn horizontal clip boundary exactly.

Viewer modes now are:

```text
0 textured Basic Wing baseline
1 Basic Wing + RGB555 Gouraud
2 Basic Wing Gouraud grayscale
3 Basic Wing polygon debug
4 Basic Wing wireframe
5 ruin room textured, normalized diagnostic camera
6 ruin room textured + lighting, normalized diagnostic camera
7 ruin room world-space + initial town camera + depth-correct lighting
```

## First ruin room lighting bring-up

The reconstructed ruin room now retains the original per-polygon lighting payload and resolves the first scene light directly from the town script data.

Pinned Azel establishes the lighting path as:

```text
initial town script @ 0x06054398
    -> townCamera_setup(anglesEA, colorEA)
    -> rotate Y then X
    -> column 2 = world light direction
    -> setupLight(...)
    -> generateLightFalloffMap(colorEA+3, +6, +9)
```

Lagi scans the initial ruin script for the registered `townCamera_setup` call at `0x0605C55C`, validates its two overlay-pointer arguments, and recovers:

- the scene light direction,
- directional RGB intensity,
- all three falloff RGB triplets.

Static-object normals are rotated by each object's original Saturn Z/Y/X transform before being stored in the flattened room mesh. The original `lightingControl` mode and mode-2 per-corner color payload remain intact.

Viewer mode 5 remains the hardware-proven unlit textured room baseline. When scene lighting resolves, viewer mode 6 uses the same quad-bilinear, 5-bit-quantized Gouraud path already proven by the Basic Wing viewer.

The current diagnostic room is normalized and does not yet use the real town camera, so mode 6 deliberately uses the nearest entry of the authentic 32-entry falloff map. Directional light, scene RGB, polygon lighting modes, transformed normals, mode-2 colors, and final RGB555 quantization are authentic. Exact per-quad distance falloff will be enabled when the live town camera/matrix path replaces the diagnostic framing.

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
