# Lagi Development Status

Current milestone: **0.2.0**

Last updated: 2026-10-03

Lagi is a native PlayStation Vita runtime for *Panzer Dragoon Saga* / *Azel*. Reconstructed Saturn-era game logic executes directly on ARMv7, with rendering and platform behavior translated to native VitaSDK/SceGxm.

## Development stage

Version 0.2.0 is centered on the first Ruins town as a live upstream-Azel runtime slice. Earlier work proved the data path, Basic Wing reconstruction, texture decode, animation, and native GXM rendering. The current runtime now drives the scene through Azel's real town tasks, scripts, player state, collision, camera, lock-on logic, dynamic objects, and VDP1 UI command generation.

The first-Ruins execution path is:

```text
Disc 1
  -> COMMON.DAT / TWN_RUIN resources
  -> town bootstrap
  -> tasks + scripts
  -> world grid / cells
  -> static + task-owned objects
  -> collision / Edge / camera / visibility
  -> sProcessed3dModel
  -> Lagi VDP1 translation
  -> SceGxm
```

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

The current scene is stable at the 30 FPS presentation target.

Recent captures show roughly:

```text
20-23 ms render work
```

before the deliberate 30 Hz presentation wait.

That corresponds to approximately 44-50 FPS of render throughput if uncapped.

## Remaining work

Major systems still incomplete or not yet integrated include:

- broader town object coverage
- additional script behavior
- additional VDP1 UI command types beyond the current scaled-sprite/polyline subset
- some collision interactions
- Ruins sound/effects
- town transitions
- UI/state flow
- VDP2/background/compositing behavior
- battle systems
- broader field systems
- movies, menus, save flow, and complete game progression

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
