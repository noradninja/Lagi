# Lagi

**Lagi** is a native PlayStation Vita runtime for *Panzer Dragoon Saga* / *Azel*. It executes reconstructed game logic directly on the Vita's ARM CPU and translates the original Saturn rendering and platform behavior to VitaSDK and native SceGxm.

Lagi is **not a Sega Saturn emulator** and does **not use VitaGL**.

> **Current status (2026-10-01):** the first Ruins town is running on real Vita hardware through the native town task/script pipeline. Edge movement and animation, the follow camera, town visibility, collision, dynamic lock/switch objects, textured RGB555-style Gouraud rendering, and Edge's original VDP1 mesh shadow are working. The current priority is to keep bringing the complete Ruins scene online through Azel's original systems rather than replacing them with room-specific logic.

## Project goals

The guiding rule is simple:

> Port the original Azel/PDS pipeline where practical; translate only at the hardware boundary.

That means gameplay ownership remains with reconstructed Azel systems — tasks, scripts, town grids, model selection, collision, camera state, animation, and object lifetimes — while Lagi adapts the parts that must differ on Vita, such as SceGxm submission, input, storage, timing, and audio.

The intended execution path is:

```text
PDS disc data
    |
Azel / ATOLM reconstructed logic
    |
Lagi native runtime (ARMv7)
    |
VDP1/VDP2 translation + Vita platform layer
    |
SceGxm / SceCtrl / SceAudioOut
    |
PlayStation Vita
```

## Current first-Ruins pipeline

The current hardware-tested path is:

```text
Disc 1 BIN/CUE
    |
COMMON.DAT + TWN_RUIN resources
    |
native town bootstrap
    |
Azel-style town tasks / scripts
    |
world grid / cells / task-owned objects
    |
native town collision
    |
Edge movement + animation
    |
Azel follow camera + town visibility
    |
sProcessed3dModel submissions
    |
Lagi VDP1 translation layer
    |
SceGxm
```

Current working pieces include:

- Direct development boot into the first Ruins town.
- Disc 1 CUE/BIN + ISO9660 access.
- `COMMON.DAT`, `TWN_RUIN.PRG`, `COMMON3`, `RUINMP`, and related first-scene resources.
- Azel task scheduler and town task/script spine.
- Town world-grid/cell ownership and static-object submission.
- Task-owned dynamic Ruins lock/switch objects.
- Native `processTownMeshCollision()`-based town collision parsing/registration.
- Edge movement, animation, interpolation, and collision body registration.
- Azel-style follow camera.
- Azel town visibility/LOD selection before the Vita render bridge.
- Native SceGxm textured VDP1-style rendering.
- Saturn RGB555 color behavior and Gouraud lighting approximation.
- Original VDP1 mesh-mode Edge shadow using the real `COMMON3.CGB` texture mask.
- 30 Hz presentation on real hardware.

## Rendering strategy

Lagi preserves original Saturn render intent without emulating the VDP1 rasterizer cycle-by-cycle.

The current Ruins renderer uses a **480x272 internal GXM render target** and presents to the Vita's **960x544 display**. This leaves enough GPU/CPU headroom for the game systems while retaining a clean 2x presentation.

### Gouraud lighting

The exact four-edge Saturn-style fragment reconstruction remains useful as a visual/reference implementation, but it was too expensive for the Vita SGX543 in close views.

The active `Full` path therefore uses a cached 3x3 subdivision per original Saturn quad:

- 9 unique generated vertices per source quad.
- 8 GXM triangles per source quad.
- original four-corner Gouraud values are approximated across the subdivision.
- RGB555 add/clamp/quantization is retained.
- static topology, UVs, and positions are cached where possible.
- dynamic polygons update only the data that changes.

This keeps the approximation inside the **Vita backend**; Azel still owns the model, material, lighting inputs, visibility, animation, and scene behavior.

### VDP1 mesh transparency

Edge's shadow is now rendered from the original shadow model and its real `COMMON3.CGB` texture mask.

The Vita backend reproduces the Saturn behavior as:

```text
original shadow texture alpha
        |
oval silhouette
        |
CMDPMOD mesh mode
        |
alternating destination pixels
        |
ordered/two-sided GXM draw
```

The mesh primitive is handled as an ordered VDP1-style draw rather than relying on ordinary GXM depth behavior.

## User-facing scene views

L/R cycles through five views:

```text
Full -> Texture -> Lighting -> Quads -> Wires
```

The on-screen labels are:

- **Full** — textured + current Gouraud path.
- **Texture** — texture-only reference.
- **Lighting** — Gouraud/lighting diagnostic.
- **Quads** — polygon-color diagnostic.
- **Wires** — two-sided wireframe topology.

The old dragon/demo views remain useful as internal regression code but are no longer part of the user-facing mode cycle.

Normal startup enters **Full** directly with the diagnostic console hidden.

## Current controls

- **L / R** — previous / next scene view.
- **Triangle** — Azel follow-camera modifier.
- **Right stick while holding Triangle** — select the native follow-camera side/rear direction.
- **START + SELECT** — exit.
- **SELECT** — diagnostic console toggle when needed during development.

The normal player movement path is owned by the town/Edge runtime rather than the old free-orbit viewer controls.

## Performance

The game remains capped at 30 presented frames per second to match the current simulation/presentation target.

Recent first-Ruins hardware captures typically measured about **20-23 ms of render work** before the deliberate 30 Hz presentation wait, corresponding to roughly **44-50 FPS of render throughput if uncapped**. The cap is intentional; simulation timing is not currently being converted to a variable-rate model.

## Game data

**No Panzer Dragoon Saga game data is included in this repository.**

Current development builds use a user-supplied Disc 1 dump:

```text
ux0:data/lagi/Disc 1/
    <disc>.cue
    <disc>.bin
```

The runtime parses the CUE, mounts the MODE1 data track, and reads the original game files directly from ISO9660.

## Source lineage

Lagi builds on community reverse-engineering work including:

- **Azel** — primary reconstructed PDS runtime/source reference.
- **ATOLM** — decompilation and accuracy reference.
- **pds-tools** — asset/format documentation and tooling.
- **Yabause / Vita Yabause** — Saturn hardware/behavior reference where useful.

The current Azel reference is pinned under `extern/Azel`; Lagi-owned Vita adaptations live in this repository.

## Building

A working VitaSDK installation and Sony `psp2cgc` are required.

See:

- [docs/BUILDING.md](docs/BUILDING.md)
- [docs/STATUS.md](docs/STATUS.md)

## Near-term direction

The immediate milestone is **not** "make this one room look complete." It is:

> Load Lagi and bring up the Ruins scene through the same systems the original game expects.

That means continuing to port, in dependency order, the remaining town object types, scripts, LCS/interaction behavior, effects, audio, transitions, UI, and eventually the broader field/battle/VDP2 systems.

## Legal

Lagi is an independent community reimplementation project. Sega, Panzer Dragoon, Panzer Dragoon Saga, and related names and assets belong to their respective owners.

This repository does not distribute copyrighted game data. Users are responsible for supplying required files from copies they are legally entitled to use.

## Acknowledgements

Lagi exists because of years of Saturn and Panzer Dragoon reverse-engineering work by the Azel, ATOLM, pds-tools, Yabause, and wider Sega Saturn development communities.
