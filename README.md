# Lagi

**Current development milestone: 0.040-alpha — authentic boot flow**

**Lagi** is a native PlayStation Vita runtime for *Panzer Dragoon Saga* / *Azel*. Reconstructed game logic runs directly on the Vita's ARM CPU, while Saturn-era rendering and platform behavior are translated to VitaSDK and native SceGxm.

Lagi is not a Sega Saturn emulator and does not use VitaGL.

## Current status

The 0.040-alpha milestone completes the transition from the old direct-Ruins development path to Azel's authentic boot and module flow. The Vita build enters through `azelInit()` / `resetEngine()`, follows Azel's native startup task graph through movies, title, New Game, the D5 name-entry sequence, and the pre-Ruins cinematic, then lets Azel's module manager load `TWN_RUIN.PRG`, create the native town task graph, and present the playable first Ruins scene through Neptune.

Working systems include:

- Disc 1 CUE/BIN and ISO9660 access
- `COMMON.DAT` and town resource loading
- authentic Azel boot through title, New Game, D5 name entry, and the pre-Ruins cinematic
- native module-manager load of `TWN_RUIN.PRG`
- Azel task scheduling and town task/script flow
- world-grid and cell ownership
- static and task-owned dynamic object submission
- native Edge hierarchy submission through Azel's normal render boundary
- native town collision
- Edge movement and animation
- Azel-style follow camera
- town visibility and LOD selection
- textured SceGxm rendering
- live Azel directional/falloff lighting carried through the presentation bridge
- RGB555-style Gouraud lighting
- Ruins lock/switch objects
- Saturn-style physical pad input translated from Vita controls
- town LCS / lock-on state and target selection
- native Neptune translation of VDP1 scaled-sprite and polyline UI commands
- native Neptune translation of VDP1 normal-sprite UI commands
- original LCS cursor, target marker, and selection-box behavior
- Azel-authored VDP2 text, framed windows, and cinematic matte presentation
- area-name, item-pickup, interaction, subtitle, and multi-choice text
- elevator choice flow through the script-driven fade and two-part Cinepak FMV
- Sega FILM demuxing, Phase 1 CPU Cinepak reconstruction, and native SceAudio PCM output
- Edge's original textured/stippled VDP1 mesh shadow
- Azel-driven fade state bridged to Neptune color-offset presentation
- native GXM 2x multisample antialiasing
- stable 30 Hz presentation

Version **0.040-alpha** is the authentic-boot-to-Ruins milestone. It is published as **v0.3.0-alpha**. The current branch keeps game-mode, scene, movie, title, script, task, and transition ownership in Azel while Lagi supplies Vita platform services and a generic presentation bridge. The Cinepak work introduced in 0.030 remains in place, including Sega FILM demuxing, SGX-assisted Cinepak presentation, and native SceAudio PCM output.

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

The game-side runtime owns tasks, scripts, game modes, scene selection, collision, camera state, animation, visibility, object lifetimes, and transition timing. Lagi owns the boundary between that runtime and the Vita: input, filesystem/disc access, timing/VBlank services, audio, memory/resource adaptation, frame synchronization, and presentation snapshots. Neptune renders those snapshots.

The current 0.040 boot path is:

```text
Disc 1 BIN/CUE
    |
azelInit() / resetEngine()
    |
native Azel startup task graph
    |
MOVIE1.CPK -> title -> New Game
    |
FLD_D5 name-entry sequence
    |
EVT002.CPK
    |
Azel module manager
    |
TWN_RUIN.PRG + native town task graph
    |
generic Lagi scene/presentation bridge
    |
Neptune
    |
SceGxm
```

The runtime architecture is summarized as:

```text
Azel decides.
Lagi services.
Neptune renders.
```

## Rendering

Lagi uses multiple Vita display framebuffer modes according to Azel's presentation state. Native 3D gameplay currently renders and scans out at 480x272 with 2x MSAA. When Azel switches the title screen into its 704-dot high-resolution VDP2 mode through TVMD, Neptune renders and scans out a 720x408 framebuffer directly. The title path therefore preserves substantially more of the source VDP2 raster instead of reducing it through the gameplay framebuffer mode.

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

Azel retains ownership of town LCS and menu state and emits its original VDP1 command stream. Neptune currently translates the command types required by the first Ruins sequence directly to native GXM:

- normal sprites -> cached textured screen-space quads
- scaled sprites -> cached textured screen-space quads
- polylines -> native GXM line primitives
- independent white free cursor and selected target marker
- original shrinking selection rectangle during target acquisition
- animated selector for the elevator multi-choice menu

The current authentic-boot path no longer depends on a direct-Ruins loader to establish scene ownership. The bridge translates Azel's commands rather than recreating lock-on gameplay or UI behavior in Vita-specific code.

### VDP2 presentation

Azel's live VDP2 state supplies front-end and in-game presentation. Neptune translates the layers currently required by the authentic boot and first-Ruins path:

- NBG3 font/text map for area names, item pickups, interactions, subtitles, and choices
- NBG1 16x16-character window map through a cached GPU tile atlas
- the animated lower cinematic matte from Azel's vertical line-scroll table
- live CRAM palette data for the VDP2 layers and bank-color VDP1 UI sprites

Composition preserves the original layer relationship: VDP2 window and matte backing, VDP1 UI sprites, then text. Azel remains responsible for the strings, window contents, choice state, cursor animation, and scripted timing.

## Renderer diagnostic views

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

The authentic first-Ruins path is hardware-proven at the intended **30 Hz** presentation rate with live world geometry, Edge, task-owned objects, textures, and Gouraud lighting active. Static geometry/material caching is kept independent from live lighting state so normal light updates do not invalidate and rebuild the room.

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
