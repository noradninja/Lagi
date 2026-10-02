# Building Lagi

Last updated: 2026-10-01

## Requirements

The current Vita build requires:

- Windows 11 or another VitaSDK-supported host.
- VitaSDK.
- CMake.
- Ninja.
- C++20-capable VitaSDK GCC/G++.
- Sony `psp2cgc` for native GXM shader compilation.

Lagi uses native SceGxm. VitaGL is not part of the renderer.

## Example environment

A typical Windows checkout may look like:

```text
C:\Dev\Lagi
C:\Dev\VitaSDK
C:\Dev\sdk\host_tools\bin\psp2cgc.exe
```

Another currently used development machine has the repository/VitaSDK under `E:\dev` and the official shader tools under `E:\PSVITA\sdk`.

The build supports locating `psp2cgc` through:

- `PSP2CGC`
- `SCE_PSP2_SDK_DIR`
- PATH / known host-tools locations

Set `VITASDK` to the active VitaSDK root.

## Initial configure

From the repository root:

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
New-Item -ItemType Directory build | Out-Null
cd build

cmake .. `
  -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE="$env:VITASDK/share/vita.toolchain.cmake" `
  -DCMAKE_BUILD_TYPE=Debug
```

## Normal incremental build

```powershell
git pull
cd build
cmake --build . -j 8
```

If `CMakeLists.txt`, shader sources, shader embedding rules, or generated build inputs changed:

```powershell
cd <Lagi checkout>
git pull
cd build
cmake ..
cmake --build . -j 8
```

## Azel source

Azel is pinned as a submodule under:

```text
extern/Azel
```

Current reference:

```text
c52329fb257561abff05531a8335e34137d67098
```

Lagi-owned Vita adaptation work should normally be made in this repository rather than editing the pinned submodule.

Initialize/update submodules when needed:

```powershell
git submodule update --init --recursive
```

## GXM shaders

Readable shader sources live under:

```text
shaders/
```

The current live Ruins path uses several native GXM programs, including:

- `texture_v.cg` / `texture_f.cg`
- `gouraud_subdiv_v.cg`
- `textured_gouraud_subdiv_f.cg`
- `gouraud_subdiv_gray_f.cg`
- `mesh_f.cg`

Additional exact/reference Gouraud shaders remain in the tree for renderer validation and experiments.

CMake invokes `psp2cgc`, then embeds the generated GXP binaries with `objcopy`:

```text
-I binary -O elf32-littlearm -B arm
```

## Game data

No PDS data is distributed with Lagi.

For current hardware development, place a user's own Disc 1 CUE/BIN dump under:

```text
ux0:data/lagi/Disc 1/
```

Example:

```text
ux0:data/lagi/Disc 1/
    Panzer Dragoon Saga Disc 1.cue
    Panzer Dragoon Saga Disc 1.bin
```

The runtime parses the CUE, finds the MODE1 data track, mounts ISO9660, and loads the first-scene resources directly from the disc image.

Current first-Ruins work uses files including:

- `COMMON.DAT`
- `TWN_RUIN.PRG`
- `COMMON3.MCB` / `COMMON3.CGB`
- `RUINMP.MCB` / `RUINMP.CGB`
- `RUINSCR.SCB` / `RUINSCR.PNB`
- `EVTRUIN.FNT`

## Persistent runtime log

Each launch creates/truncates:

```text
ux0:data/lagi/lagi.log
```

The Vita-native logger flushes frequently so useful state often survives abnormal exits.

Useful log areas now include:

- platform startup.
- disc/ISO9660 mounting.
- COMMON/town table parsing.
- direct-boot target resolution.
- town/grid/cell setup.
- task-owned object creation.
- Edge state and animation.
- collision setup.
- material/texture decode.
- GXM initialization and failures.

## Current app behavior

Normal startup:

1. initializes the native runtime/GXM path,
2. resolves the first Ruins town from Disc 1,
3. starts the town task/script pipeline,
4. enters the scene in **Full** view,
5. keeps the diagnostic console hidden unless explicitly toggled.

Internal render resolution is currently:

```text
480x272 GXM -> 960x544 Vita display
```

Presentation target is 30 Hz.

## Development controls

Current scene controls:

- **L / R** — cycle views:
  `Full -> Texture -> Lighting -> Quads -> Wires`
- **Triangle** — hold the Azel follow-camera modifier.
- **Right stick + Triangle** — choose side/rear follow-camera direction.
- **SELECT** — toggle the diagnostic console when needed.
- **START + SELECT** — exit.

The old Basic Wing free-camera controls are no longer the normal town controls.

## Current renderer validation

When changing the live renderer, verify at minimum:

- **Full** retains correct room/Edge/dynamic-object textures and lighting.
- **Texture** matches geometry/material placement without Gouraud contribution.
- **Lighting** shows the expected four-corner lighting field.
- **Quads** shows the submitted filled polygon topology.
- **Wires** exposes the same topology two-sided.
- no giant camera-crossing polygons reappear in Texture.
- Edge animation speed remains stable.
- dynamic Ruins locks/switches retain the correct materials.
- Edge's shadow keeps its oval mask and alternating VDP1 mesh stipple.
- the shadow remains visible over the floor without depth fighting.
- normal geometry still uses the intended culling/depth state after mesh draws.
- scene presentation remains a stable 30 FPS.

## Performance reference

A recent good first-Ruins build typically measured about:

```text
20-23 ms render work
```

before the deliberate 30 Hz presentation wait.

Use the hidden profiling overlay when investigating regressions; the normal OSD intentionally omits those diagnostics.

## Notes on the Gouraud path

The active Full renderer is **not** the older exact heavy fragment solve.

It uses a cached 3x3 subdivision representation of each original Saturn quad to approximate the four-corner lighting field cheaply enough for Vita hardware while retaining RGB555-style color math.

The exact/reference paths remain in source and should not be removed casually; they are useful as a visual oracle when validating future optimizations.

## Vita application assets

The VPK packages:

```text
sce_sys/icon0.png
sce_sys/livearea/contents/bg0.png
sce_sys/livearea/contents/startup.png
sce_sys/livearea/contents/template.xml
```

If packaging rules or these assets change, rerun CMake configuration before rebuilding.
