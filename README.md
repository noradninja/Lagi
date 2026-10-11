# Lagi

**Current development milestone: native FLD_A3 flight and shared Neptune rendering**

**Lagi** is a native PlayStation Vita runtime for *Panzer Dragoon Saga* / *Azel*. Reconstructed game logic runs directly on the Vita's ARM CPU, while Saturn-era rendering and platform behavior are translated to VitaSDK and native SceGxm.

Lagi is not a Sega Saturn emulator and does not use VitaGL.

## Current status

Current main extends the authentic Azel boot path through the opening movies, title, New Game, D5 name entry, playable Ruins, elevator/Cinepak handoff, native FLD_A3 flight, E006 in-engine cinematic and Excavation arrival. Azel owns the native field task graph, camera, visibility cells and transitions. This development state extends the earlier public v0.3.0alpha release; it does not imply a new packaged release or complete game support.

Working systems include:

- native FLD_A3 entry, dragon/rider, radar, LCS and cell-driven environment rendering
- shared native VDP2 rotation-background and priority-aware composition
- native image particles, collection orbs/trails, save-station effects and destructible-object effects
- native actor placement through cinematic and Excavation camera changes
- reusable texture residency, a CDRAM-preferred upload arena, and range-aware image/LUT/palette invalidation

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
- native Neptune translation of VDP1 normal/scaled sprites and polyline UI commands
- original LCS cursor, target marker, and selection-box behavior
- Azel-authored VDP2 text, framed windows, and cinematic matte presentation
- area-name, item-pickup, interaction, subtitle, and multi-choice text
- native Vita BGM and sound-effect playback
- runtime ARM translation of Sega Saturn SCSP DSP programs
- hardware-validated SCSP DSP execution, with ARM now the normal default backend
- Sega FILM demuxing and Cinepak playback with native SceAudio
- SGX-assisted Cinepak reconstruction at source resolution
- hardware-accepted screen-space 2x2 coordinate dithering for final Cinepak presentation
- Edge's original textured/stippled VDP1 mesh shadow
- Saturn-accurate Azel-driven black/white fade direction, timing, and color-offset presentation bridged through Neptune
- native 980x544 gameplay presentation with MSAA disabled
- native 720x408 high-resolution title presentation
- stable 30 Hz presentation through the supported 3D sequence

The public **v0.3.0alpha** release is the current hardware milestone. Its main additions are native audio, the runtime ARM SCSP DSP translator, and the cleaned-up Cinepak presentation path. The renderer and platform layers continue to follow the same ownership rule: Azel decides game state and timing, Lagi provides Vita services, and Neptune renders the published Saturn-era presentation state.

## Architecture

The 2026-10-10 development checkpoint adds optional native 960x544 gameplay/movie presentation (`LAGI_FULLRES=ON`; default OFF) and accepted lookup-based dithering (`LAGI_STOCHASTIC_FILTER=ON`, `LAGI_DITHER_LOOKUP=ON`; both default ON). The user accepted Vita build `508bf75` as effectively free versus hardware bilinear in tested gameplay. Full-screen menus still fall below 30 fps with either filter; that existing limitation is not resolved by this merge. See [dither performance](docs/DITHER_PERFORMANCE.md) for scope and evidence boundaries.

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

The current native boot path is:

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
playable Ruins sequence -> elevator choice
    |
EVT004_1.CPK / EVT004_2.CPK
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

Lagi uses multiple Vita display framebuffer modes according to Azel's presentation state. Native 3D gameplay currently renders and scans out at 480x272 with MSAA disabled. When Azel switches the title screen into its 704-dot high-resolution VDP2 mode through TVMD, Neptune renders and scans out a 720x408 framebuffer directly. The title path therefore preserves substantially more of the source VDP2 raster instead of reducing it through the gameplay framebuffer mode.

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
- screen-space 2x2 lookup dithering for decoded title, text, VDP1 UI/particle sprites, menu/window graphics and UI backgrounds; radar remains point-filtered

The current authentic-boot path no longer depends on a direct-Ruins loader to establish scene ownership. The bridge translates Azel's commands rather than recreating lock-on gameplay or UI behavior in Vita-specific code.

### VDP2 presentation

Azel's live VDP2 state supplies front-end and in-game presentation. Neptune translates the layers currently required by the authentic boot and first-Ruins path:

- NBG3 font/text map for area names, item pickups, interactions, subtitles, and choices
- NBG1 16x16-character window map through a cached GPU tile atlas
- the animated lower cinematic matte from Azel's vertical line-scroll table
- live CRAM palette data for the VDP2 layers and bank-color VDP1 UI sprites

Composition preserves Azel's authored layer relationships while allowing decoded presentation assets to use hardware filtering. Front-end/menu text is reconstructed into a 352x224 RGBA text layer and composited by SGX with linear filtering; decoded VDP1 selector/cursor sprites are likewise linearly filtered after palette decode. The title NBG0 artwork is decoded once into an RGBA presentation surface and then scaled by SGX with hardware linear filtering, avoiding repeated raw VDP2 decode work per output fragment. Azel remains responsible for the strings, window contents, choice state, cursor animation, fades, and scripted timing.

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

The public 0.3.0-alpha build exposes the controls currently needed for the supported boot/Ruins/movie path:

- **Right analog stick** — walk
- **Square** — run / action
- **Cross** — lock on / cancel
- **Select** — cycle Neptune rendering debug views
- **Start** — start the game / skip FMV

The underlying Vita input bridge still presents Saturn-style input state to Azel. As additional game modes come online, the public mapping will continue to follow the actions Azel expects rather than duplicating gameplay decisions in Vita-specific code.

## Performance

The game is currently presented at 30 FPS.

The authentic first-Ruins path is hardware-proven at the intended **30 Hz** presentation rate with live world geometry, Edge, task-owned objects, textures, and Gouraud lighting active. Static geometry/material caching is kept independent from live lighting state so normal light updates do not invalidate and rebuild the room.

The 30 Hz cap remains in place because the game/simulation timing path has not been converted to a variable-rate model.

On 2026-10-10 the user accepted the rendering milestone after reporting smooth play and no visual/performance regression on the tested range-invalidation route. The latest hardware capture has a sampled native render median of 23.887 ms; 3960 completed field frames contain two cold frames of 39.078/41.906 ms. Those spikes are accepted for now, not evidence of universal 33.3 ms compliance. The final captured-palette dependency refinement has host/syntax validation but no separate hardware capture yet.

Known remaining work includes the incorrect VDP2 floor in E006/Excavation, cold resource preparation, broader scene coverage and deferred native ray/laser rendering. See [the detailed rendering handoff](docs/FLIGHT_MODE_BRINGUP.md).

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
