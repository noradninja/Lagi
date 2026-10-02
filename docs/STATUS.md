# Lagi Development Status

Last updated: 2026-10-01

Lagi is a native PlayStation Vita runtime for *Panzer Dragoon Saga* / *Azel*. Reconstructed Saturn-era game logic executes directly on ARMv7; original rendering intent is translated to native VitaSDK/SceGxm. Lagi does not emulate the Saturn CPUs and does not use VitaGL.

## Current milestone

The project has moved beyond standalone geometry/viewer bring-up.

The current development target is the **first Ruins town running as a real scene** through Azel's original ownership and update pipelines. The room is no longer treated as a static renderer demo.

Current high-level path:

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

## Hardware-proven runtime/data path

Verified on real Vita/Vita TV hardware:

- Vita process/platform startup and persistent logging.
- Direct CUE/BIN access from `ux0:data/lagi/Disc 1/`.
- MODE1/2352 and MODE1/2048 handling.
- ISO9660 traversal.
- `COMMON.DAT` parsing.
- town overlay/resource loading.
- pinned first-scene direct boot into `TWN_RUIN`.
- Azel fixed-point support.
- Azel heap/task runtime.
- reconstructed Saturn memory/pointer helpers.
- native SceGxm renderer and shader pipeline.
- stable 30 Hz presentation.

## First Ruins systems now active

### Town task/script pipeline

The runtime now uses the actual town-oriented structure rather than a room-only viewer:

- town root/task lifecycle.
- town script task.
- world-grid/cell task ownership.
- static cell geometry submissions.
- task-owned dynamic object submissions.
- render bridge from Azel's selected processed models into the Vita backend.

The architectural rule is to preserve Azel ownership and move platform-specific adaptation downstream.

### Collision

The first Ruins path uses the native town collision data/functions already recovered from Azel, including the `processTownMeshCollision()` pipeline.

Current collision work includes:

- collision mesh decoding.
- collision body registration.
- Edge body registration.
- floor/scene collision sufficient for the current room.

Broader body-to-body/interaction behavior remains incomplete.

### Edge

Current Edge path includes:

- town-owned position/yaw.
- movement.
- run/walk/idle animation selection.
- animation frame stepping/interpolation.
- transformed geometry/normals.
- collision body.
- renderer submission through the live town model path.

### Camera

The old free-orbit camera is no longer the active town camera.

The first Ruins path uses the Azel-style follow camera. Vita Triangle is mapped as the camera modifier, with the right stick selecting the original side/rear follow directions while held.

### Ruins lock/switch objects

The first task-owned Ruins lock/switch objects are live:

- object creation from cell data.
- task-owned transform/state.
- collision registration.
- native disable/wait script entry points.
- animated translation/removal lifecycle.
- correct material/texture resolution in the live town path.

This proves the room can contain real dynamic town objects rather than only flattened static geometry.

## Current rendering path

### Resolution / presentation

Current configuration:

```text
internal GXM render: 480x272
display output:       960x544
presentation target:  30 Hz
```

The 480x272 internal target is an intentional performance choice; display presentation remains native Vita size.

### User-facing modes

L/R cycles:

```text
Full
Texture
Lighting
Quads
Wires
```

OSD labels use only those names.

- **Full**: textured + Gouraud.
- **Texture**: texture-only reference.
- **Lighting**: lighting diagnostic.
- **Quads**: polygon-color diagnostic.
- **Wires**: two-sided wireframe.

The old Basic Wing/dragon demo views remain in code as regression/reference paths but are removed from the normal runtime mode cycle.

The app enters **Full** automatically and keeps the diagnostic console hidden during normal startup.

### Town texture/material path

The first Ruins renderer now resolves original town materials/textures for:

- static room geometry.
- task-owned dynamic objects.
- Edge.
- Edge's shadow.

Texture-only rendering uses the same live-town visibility set as the other authentic views, fixing the earlier giant/clipped polygons when geometry crossed the camera.

### Gouraud strategy

The original exact projected-perimeter/four-edge shader remains a correctness reference.

On Vita hardware it was too fragment-expensive near large close polygons, so the active Full path uses a cached 3x3 subdivision approximation:

```text
source Saturn quad
 -> 9 unique subdivision vertices
 -> 8 GXM triangles
 -> four-corner shade approximation
 -> RGB555 add / clamp / quantize
```

Current optimizations:

- immutable subdivision topology cached.
- UVs cached.
- static positions cached.
- dynamic positions refreshed only where required.
- 9 unique vertices rather than 24 duplicated vertices per source quad.
- visible index lists compacted each frame.
- lightweight fragment combine.

The approximation is intentionally confined to the Vita translation layer. It does not alter Azel's scene/task/model representation.

### Edge VDP1 mesh shadow

Edge's visible shadow is now restored from the original game data.

Current path:

1. Edge's animation table supplies the shadow model key, matching `sEdgeTask::Draw()`.
2. The shadow model is loaded from `COMMON3.MCB`.
3. The real shadow texture/mask is decoded from `COMMON3.CGB`.
4. The texture alpha defines the oval silhouette.
5. `CMDPMOD` mesh mode applies alternating destination-pixel stippling.
6. The mesh draw is treated as two-sided.
7. The Vita backend uses ordered VDP1-style depth behavior for the shadow rather than allowing the floor Z-buffer to reject it.

This is the first explicit native translation of Saturn VDP1 mesh-mode behavior in the live scene.

## Performance status

The current scene is solid at the 30 FPS target on real hardware.

Recent profiler captures before the intentional presentation wait typically showed:

```text
render work: ~20-23 ms
```

That is roughly equivalent to **44-50 FPS** of render throughput if uncapped, leaving approximately 10-13 ms of headroom inside a 33.33 ms 30 FPS frame budget.

The application remains intentionally capped to 30 Hz because simulation/game timing has not been converted to a variable-rate model.

## Important architectural boundaries

These should remain true as more systems come online:

- Renderer subdivision is a Vita backend detail; it must not leak into Azel tasks/models.
- Town objects remain task/grid owned.
- Collision remains separate from render approximation.
- Visibility/model selection is performed by the town runtime before the GXM backend.
- New VDP1 modes should dispatch from original command/material state such as `CMDPMOD`, not from room-specific hacks.
- Exact/reference shaders should remain available for visual validation even when a cheaper Vita implementation is active.

## Known technical debt / follow-up

- Material cache lifecycle/invalidation should be hardened before frequent town transitions.
- Current subdivision buffer uses 16-bit vertex indexing; broader scenes will eventually need per-model/per-cell batching.
- Collision body-to-body behavior is not complete.
- Ruins lock sound effect is not wired.
- Full LCS/target interaction is not yet implemented.
- Only a small subset of Ruins task-owned object definitions has been ported.
- Audio is still largely outside the current first-room milestone.
- VDP2/background/compositing systems remain future work.
- Battle systems, broader field systems, menus/UI, movies, save flow, and scene transitions are not yet integrated as a complete game runtime.

## Historical reference work retained

The earlier Basic Wing renderer remains useful as a regression/reference implementation for:

- original VDP1 texture decoding.
- model hierarchy and hotpoints.
- animation decoding.
- Gouraud/RGB555 experiments.
- shader bring-up.
- GXM resource validation.

It is no longer the primary development target.

## Immediate priority

Continue bringing up the **complete Ruins scene through original Azel pipelines**.

Likely next systems include:

- remaining town object task types.
- script coverage.
- LCS/interaction behavior.
- effects.
- sound.
- scene transitions.
- UI/state required by the Ruins sequence.

The target is a reusable native PDS runtime, not a bespoke recreation of this one room.
