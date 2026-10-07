# Building Lagi

Current development milestone: **0.3.0-alpha**

Last updated: 2026-10-06

## Requirements

The current Vita build uses:

- VitaSDK
- CMake
- Ninja
- a C++20-capable VitaSDK GCC/G++
- Sony `psp2cgc` for native GXM shader compilation

Lagi uses native SceGxm. VitaGL is not part of the renderer.

## Example Windows layout

One development setup uses:

```text
C:\Dev\Lagi
C:\Dev\VitaSDK
C:\Dev\sdk\host_tools\bin\psp2cgc.exe
```

Another uses the repository/VitaSDK under `E:\dev` and the official shader tools under `E:\PSVITA\sdk`.

The build can locate `psp2cgc` through `PSP2CGC`, `SCE_PSP2_SDK_DIR`, PATH, or known host-tool locations.

## Initial configure

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
New-Item -ItemType Directory build | Out-Null
cd build

cmake .. `
  -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE="$env:VITASDK/share/vita.toolchain.cmake" `
  -DCMAKE_BUILD_TYPE=Debug
```

## Incremental build

```powershell
Set-Location E:\dev\Lagi

git switch main
git pull --ff-only origin main

$env:PSP2CGC = 'E:\PSVITA\sdk\host_tools\bin\psp2cgc.exe'
cmake --build build --parallel 8
```

When CMake configuration or shader build rules change:

```powershell
cd <Lagi checkout>
git switch main
git pull --ff-only origin main
cd build
cmake ..
cmake --build . -j 8
```

## Azel source

Azel is included as a submodule under:

```text
extern/Azel
```

Current reference:

```text
c52329fb257561abff05531a8335e34137d67098
```

Submodules can be initialized with:

```powershell
git submodule update --init --recursive
```

Lagi-specific Vita code is kept in the main repository so the pinned Azel tree remains a clean reference.

## GXM shaders

Readable shader sources live under:

```text
shaders/
```

The current Ruins path uses, among others:

- `texture_v.cg`
- `texture_f.cg`
- `gouraud_subdiv_v.cg`
- `textured_gouraud_subdiv_f.cg`
- `gouraud_subdiv_gray_f.cg`
- `mesh_f.cg`

The older exact/reference Gouraud shaders are still present for comparison and renderer validation.

CMake compiles GXP programs with `psp2cgc` and embeds them with `objcopy` using:

```text
-I binary -O elf32-littlearm -B arm
```

## Game data

No PDS game data is distributed with Lagi.

Current development builds read a user-supplied Disc 1 CUE/BIN dump from:

```text
ux0:data/lagi/Disc 1/
```

Example:

```text
ux0:data/lagi/Disc 1/
    Panzer Dragoon Saga Disc 1.cue
    Panzer Dragoon Saga Disc 1.bin
```

The runtime parses the CUE, locates the MODE1 data track, mounts ISO9660, and loads first-scene resources directly from the disc image.

Current Ruins work uses files including:

- `COMMON.DAT`
- `MENU.CGB`
- `TWN_RUIN.PRG`
- `COMMON3.MCB` / `COMMON3.CGB`
- `RUINMP.MCB` / `RUINMP.CGB`
- `RUINSCR.SCB` / `RUINSCR.PNB`
- `EVTRUIN.FNT`
- `EVT004_1.CPK` / `EVT004_2.CPK`

## Runtime log

Each launch creates/truncates:

```text
ux0:data/lagi/lagi.log
```

Useful sections currently include:

- platform startup
- disc/ISO9660 mounting
- COMMON/town table parsing
- Azel startup, module, and game-status transitions
- town/grid/cell setup
- task-owned object creation
- Edge state and animation
- collision setup
- material/texture decode
- VDP1 UI command diagnostics
- VDP2 text/window and line-scroll presentation
- movie sequencing, FILM/Cinepak decode, and SceAudio PCM output
- SCSP DSP backend selection, VM probe, translation, and audio timing
- GXM initialization and failures

## Current startup behavior

A normal development launch follows Azel's native startup path rather than the old direct-Ruins bootstrap. It enters through the opening movie and title, continues through New Game and the D5 name-entry sequence, plays the pre-Ruins cinematic, loads `TWN_RUIN.PRG` through Azel's module manager, and reaches the playable Ruins sequence. The current supported slice continues through the elevator choice and post-elevator Cinepak playback; flight mode is not yet connected, so execution fades to black when Azel advances beyond that point.

The native audio path is active during this sequence. Runtime ARM translation is the default SCSP DSP backend and requires no `ux0:data/lagi/dsp_backend.txt` file. That file remains available only as a diagnostic override for `predecoded`, `reference`, `auto`, `aot`, or explicit `arm` selection.

Current display framebuffer configuration:

```text
3D gameplay:            480x272, no MSAA
high-resolution title:  720x408, no MSAA
presentation cadence:   30 Hz
```

Cinepak payload data remains point-sampled while it is decoded by the SGX reconstruction pass. The reconstructed source-resolution RGBA image is then presented through the normal hardware-linear texture path into the active Vita framebuffer.

The title framebuffer mode follows Azel's live VDP2 TVMD state. Neptune changes the GXM render target and the dimensions supplied to `sceDisplaySetFrameBuf()` when Azel enters or leaves the high-resolution title mode.

## Controls

The current public 0.3.0-alpha build uses:

- **Right analog stick** — walk
- **Square** — run / action
- **Cross** — lock on / cancel
- **Select** — cycle Neptune rendering debug views
- **Start** — start the game / skip FMV

The platform bridge continues to translate Vita input into Saturn-style state for Azel; these bindings describe the currently exposed release controls rather than a separate Vita gameplay layer.

## Renderer reference behavior

The current live scene is expected to show:

- consistent geometry and materials between Full and Texture
- the same visibility set across the main scene modes
- original quad boundaries in Wires
- stable Edge animation
- correctly textured dynamic Ruins objects
- working town Lock-On with independent white cursor and target marker
- native VDP1 selection-polyline rendering
- native VDP1 normal/scaled-sprite UI rendering
- VDP2 area-name, item, interaction, subtitle, and multi-choice text
- GPU-rendered NBG1 framed windows
- the line-scroll-driven lower cinematic matte
- the animated elevator-choice selector
- Edge's oval stippled VDP1 mesh shadow with ordered draw/depth behavior
- stable 30 FPS presentation

The hidden profiling overlay remains available for performance work.

## Performance reference

The supported 3D path is hardware-proven at a consistent 30 FPS in the current Ruins sequence, with sub-22 ms render work reported for the release route. MSAA is disabled across Neptune's active presentation paths.

The native SCSP DSP path is also hardware-proven. The 84-step program measured approximately 2.3 ms median with runtime ARM translation, down from approximately 5.3 ms on the predecoded path, keeping the tested audio quantum within budget.

## Vita application assets

The VPK packages:

```text
sce_sys/icon0.png
sce_sys/livearea/contents/bg0.png
sce_sys/livearea/contents/startup.png
sce_sys/livearea/contents/template.xml
```
