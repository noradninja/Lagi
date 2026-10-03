# Lagi

**Current development milestone: 0.2.0**

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
- Saturn-style physical pad input translated from Vita controls
- town LCS / lock-on state and target selection
- native Neptune translation of VDP1 scaled-sprite and polyline UI commands
- original LCS cursor, target marker, and selection-box behavior
- Edge's original textured/stippled VDP1 mesh shadow
- script-driven town fade-in
- stable 30 Hz presentation

Version **0.2.0** marks the transition from a rendered Ruins scene to a genuinely interactive upstream-Azel town slice: Azel owns movement, camera, collision, scripts, lock-on state, target selection, dynamic objects, and UI command generation, while Lagi translates the platform and rendering boundaries to Vita.

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

The current Ruins renderer uses a 480x272 GXM render target and presents to the Vita's 960x544 display. Saturn-authored 3D and 2D presentation is aspect-corrected inside the widescreen output.

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

### VDP1 town UI

Azel retains ownership of town LCS state and emits its original VDP1 command stream. Neptune currently translates the command types required by the first Ruins lock-on sequence directly to native GXM:

- scaled sprites -> cached textured screen-space quads
- polylines -> native GXM line primitives
- independent white free cursor and selected target marker
- original shrinking selection rectangle during target acquisition

The bridge translates Azel's commands rather than recreating lock-on gameplay or UI behavior in Vita-specific code.

## Scene views

Vita D-pad Left/Right cycles through:

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

Lagi exposes the Vita controls to Azel as a Saturn-style physical pad so Azel's own walk/flight/battle action maps remain authoritative.

Current town mapping:

- **Square** -> Saturn A
- **Cross** -> Saturn B
- **Circle** -> Saturn C
- **Triangle** -> Saturn Y
- **L / R** -> Saturn L / R
- **Start** -> Saturn Start
- **Left analog stick** -> Saturn analog X/Y
- **D-pad Left / Right** -> Neptune renderer-mode cycle during current development builds
- **D-pad Up / Down** -> Saturn D-pad Up / Down
- **SELECT** -> performance timing OSD toggle
- **START + SELECT** -> full retained debug/status screen

In walk mode, the current controls match the original manual behavior: A/C enter or select in Lock-On mode, B runs while moving and cancels Lock-On.

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
