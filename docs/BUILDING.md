# Building Lagi

Current development milestone: **0.030-alpha**

Last updated: 2026-10-03

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
- direct-boot target resolution
- town/grid/cell setup
- task-owned object creation
- Edge state and animation
- collision setup
- material/texture decode
- VDP1 UI command diagnostics
- VDP2 text/window and line-scroll presentation
- movie sequencing, FILM/Cinepak decode, and SceAudio PCM output
- GXM initialization and failures

## Current startup behavior

A normal development launch:

1. initializes the runtime and GXM,
2. resolves the first Ruins town from Disc 1,
3. restores the resident VDP1 menu data and VDP2 startup state expected by the town runtime,
4. starts the town task/script pipeline,
5. enters the scene in **Full** view,
6. leaves the diagnostic console hidden,
7. after the elevator sequence, plays `EVT004_1.CPK` and `EVT004_2.CPK`
   from Disc 1 before Azel requests the next game status.

Current display framebuffer configuration:

```text
3D gameplay:            480x272, no MSAA
high-resolution title:  720x408, no MSAA
presentation cadence:   30 Hz
```

The title framebuffer mode follows Azel's live VDP2 TVMD state. Neptune changes the GXM render target and the dimensions supplied to `sceDisplaySetFrameBuf()` when Azel enters or leaves the high-resolution title mode.

## Controls

Current Vita-to-Saturn town mapping:

- **Square** -> Saturn A
- **Cross** -> Saturn B
- **Circle** -> Saturn C
- **Triangle** -> Saturn Y
- **L / R** -> Saturn L / R
- **Start** -> Saturn Start
- **Left analog stick** -> Saturn analog X/Y
- **D-pad Left / Right** -> cycle `Full -> Texture -> Lighting -> Quads -> Wires`
- **D-pad Up / Down** -> Saturn D-pad Up / Down
- **SELECT** -> performance timing OSD
- **START + SELECT** -> full retained debug/status screen

For the current walk-mode input table, A/C enter or select Lock-On targets and B runs while moving or cancels Lock-On.

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

First-Ruins captures taken before 2x MSAA was enabled showed approximately:

```text
20-23 ms render work
```

before the deliberate 30 Hz presentation wait.

Updated on-device timing should be captured with 2x MSAA enabled before using that figure as the current renderer cost.

## Vita application assets

The VPK packages:

```text
sce_sys/icon0.png
sce_sys/livearea/contents/bg0.png
sce_sys/livearea/contents/startup.png
sce_sys/livearea/contents/template.xml
```
