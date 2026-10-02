# Lagi

**Lagi** is a native PlayStation Vita runtime for *Panzer Dragoon Saga* / *Azel*. Reconstructed game logic runs directly on the Vita's ARM CPU, while Saturn-era rendering and platform behavior are translated to VitaSDK and native SceGxm.

Lagi is not a Sega Saturn emulator and does not use VitaGL.

## Current status

Development is currently focused on the first Ruins town. The scene now runs on real Vita hardware through the native town task/script pipeline rather than as a standalone geometry demo.

Working systems include:

- Disc 1 CUE/BIN and ISO9660 access
- `COMMON.DAT` and town resource loading
- direct development boot into `TWN_RUIN`
- Azel task scheduling and town task/script flow
- world-grid and cell ownership
- static and task-owned dynamic object submission
- native town collision
- Edge movement and animation
- Azel-style follow camera
- town visibility and LOD selection
- textured SceGxm rendering
- RGB555-style Gouraud lighting
- Ruins lock/switch objects
- Edge's original VDP1 mesh shadow
- stable 30 Hz presentation

The current milestone is to continue bringing up the complete Ruins scene using the same systems and ownership boundaries as the original game.

## Architecture

```text
PDS disc data
    |
Azel / ATOLM reconstructed logic
    |
Lagi native runtime (ARMv7)
    |
VDP1 / VDP2 translation + Vita platform layer
    |
SceGxm / SceCtrl / SceAudioOut
    |
PlayStation Vita
```

The game-side runtime owns tasks, scripts, town grids, collision, camera state, animation, visibility, and object lifetimes. Vita-specific code is concentrated in the platform and rendering translation layers.

The current first-Ruins path is:

```text
Disc 1 BIN/CUE
    |
COMMON.DAT + TWN_RUIN resources
    |
town bootstrap
    |
town tasks / scripts
    |
world grid / cells / task-owned objects
    |
collision / Edge / camera / visibility
    |
sProcessed3dModel submissions
    |
Lagi VDP1 translation
    |
SceGxm
```

## Rendering

The current Ruins renderer uses a 480x272 GXM render target and presents to the Vita's display, maintaining the original object 4:3 scale while rendering in a 16:9 aspect.

### Gouraud lighting

The exact four-edge Saturn-style fragment reconstruction remains in the project as a reference path. On Vita hardware it was too expensive in close views, so the active `Full` mode uses a cached 3x3 subdivision of each Saturn quad.

Each source quad is represented by:

- 9 unique subdivision vertices
- 8 GXM triangles
- four-corner Gouraud values interpolated across the subdivision
- RGB555-style add, clamp, and quantization

Static topology, UVs, and positions are cached where possible. Dynamic polygons update only the data that changes.

This approximation exists only in the Vita renderer. The Azel-side model and scene structures remain unchanged.

### VDP1 mesh shadow

Edge's shadow uses the original shadow model from `COMMON3.MCB` and its real texture mask from `COMMON3.CGB`.

The texture supplies the oval silhouette. VDP1 `CMDPMOD` mesh mode supplies the alternating-pixel stipple. The Vita backend renders that primitive two-sided and with ordered VDP1-style depth behavior so it remains visible over the floor.

## Scene views

L/R cycles through:

```text
Full -> Texture -> Lighting -> Quads -> Wires
```

- **Full** — textured scene with the active Gouraud path
- **Texture** — texture-only reference
- **Lighting** — lighting-only diagnostic
- **Quads** — filled polygon diagnostic
- **Wires** — original Saturn quad perimeter view

The older dragon/demo views remain in the source tree as regression/reference code but are not part of the normal mode cycle.

Normal startup enters **Full** with the diagnostic console hidden.

## Controls

- **L / R** — previous / next scene view
- **Triangle** — Azel follow-camera modifier
- **Right stick while holding Triangle** — side/rear follow-camera selection
- **SELECT** — diagnostic console toggle
- **START + SELECT** — exit

## Performance

The game is currently presented at 30 FPS.

Recent first-Ruins captures show approximately **20-23 ms** of render work before the deliberate presentation wait, corresponding to about **44-50 FPS** of render throughput if uncapped.

The 30 Hz cap remains in place because the game/simulation timing path has not been converted to a variable-rate model.

## Game data

No Panzer Dragoon Saga game data is included in this repository.

Current development builds use a user-supplied Disc 1 dump:

```text
ux0:data/lagi/Disc 1/
    <disc>.cue
    <disc>.bin
```

The runtime parses the CUE, mounts the MODE1 data track, and reads the original files directly from ISO9660.

## Source lineage

Lagi builds on several community reverse-engineering projects:

- **Azel** — primary reconstructed PDS runtime and source-level behavior reference
- **ATOLM** — decompilation and accuracy reference
- **pds-tools** — asset and format documentation/tooling
- **Yabause / Vita Yabause** — Saturn hardware and behavior reference

The current Azel reference is pinned under `extern/Azel`. Vita-specific adaptations live in the Lagi repository.

## Building

A working VitaSDK installation and Sony `psp2cgc` are required.

See:

- [docs/BUILDING.md](docs/BUILDING.md)
- [docs/STATUS.md](docs/STATUS.md)

## Legal

Lagi is an independent community reimplementation project. Sega, Panzer Dragoon, Panzer Dragoon Saga, and related names and assets belong to their respective owners.

This repository does not distribute copyrighted game data. Users are responsible for supplying required files from copies they are legally entitled to use.

## Acknowledgements

Lagi exists because of years of Saturn and Panzer Dragoon reverse-engineering work by the Azel, ATOLM, pds-tools, Yabause, and wider Sega Saturn development communities.
