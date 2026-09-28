# Building Lagi

## Requirements

- Windows/Linux/macOS environment capable of running VitaSDK.
- VitaSDK configured through the `VITASDK` environment variable.
- CMake.
- Ninja is recommended.
- VitaSDK ARM toolchain.

The current hardware development environment uses Ninja and the VitaSDK CMake toolchain.

## Configure

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

After the build directory is configured, normal incremental builds are:

```powershell
cd E:\dev\Lagi
git pull
cd build
cmake ..
cmake --build . -j 8
```

## GXM shaders

The current native GXM debug-view shaders are checked into the source tree as precompiled GXP byte arrays. A normal VitaSDK installation is sufficient; `psp2cgc` is **not** required.

The readable Cg shader sources remain under `shaders/` as reference for future shader development. If those shaders change later, the vendored GXP data must be regenerated separately before committing.

## Game data

No PDS game data is distributed by Lagi.

For current hardware testing, place the user's own Disc 1 BIN/CUE dump at:

```text
ux0:data/lagi/Disc 1/
```

Example:

```text
ux0:data/lagi/Disc 1/
    Panzer Dragoon Saga Disc 1.cue
    Panzer Dragoon Saga Disc 1.bin
```

The CUE parser currently supports MODE1/2352 and MODE1/2048 data tracks. Lagi mounts the ISO9660 filesystem inside the BIN and reads files such as `COMMON.DAT` and `DRAGON0.MCB` directly.

## Hardware controls

Current development controls:

- SELECT: switch between debug/status screen and 3D viewer.
- START + SELECT: exit.
- Left analog stick: rotate the Basic Wing viewer.
- Triangle: reset viewer rotation.
- L / R: change debug rendering mode.

These controls are development-only and may change as the real Saturn input layer comes online.

## Expected runtime status

A healthy current build should progress through PASS entries for:

- Vita platform/framebuffer
- Saturn memory readers
- Disc 1 CUE/BIN + ISO9660
- COMMON.DAT dragon/battle tables
- sound table 79/79
- dragon COMMON data
- DRAGON0 hierarchy/hotpoint validation
- DRAGON0 geometry validation
- GXM Basic Wing viewer readiness
- Azel root task
- active task loop

## Runtime log

Each Vita launch creates a fresh persistent runtime log at:

`ux0:data/lagi/lagi.log`

The file is truncated on startup and flushed after each logged message. After a hardware test, exit Lagi and retrieve/open `lagi.log` with VitaShell. The log currently includes platform startup, COMMON.DAT loading, dragon hierarchy/geometry validation, and Basic Wing VDP1 polygon/texture descriptor diagnostics.

## Textured GXM shader compiler

The first textured Basic Wing renderer uses readable Cg sources at `shaders/texture_v.cg` and `shaders/texture_f.cg`. CMake compiles these with Sony's `psp2cgc` and embeds the resulting GXP binaries with `arm-vita-eabi-objcopy`. The current Windows development setup is detected automatically at `E:/PSVITA/sdk/host_tools/bin/psp2cgc.exe`; other setups can expose the compiler through PATH, `PSP2CGC`, or `SCE_PSP2_SDK_DIR`.

After pulling this milestone, rerun `cmake ..` once before building so the generated shader-object rules are added to the existing build directory.
