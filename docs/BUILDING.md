# Building Lagi

## Requirements

- Windows/Linux/macOS environment capable of running VitaSDK.
- VitaSDK configured through the `VITASDK` environment variable.
- CMake.
- Ninja is recommended.
- VitaSDK ARM toolchain.
- `psp2cgc` for native SceGxm shader compilation.

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

## GXM shader compiler

The native GXM viewer compiles Cg shaders to Vita GXP programs during the CMake build.

CMake first looks for `psp2cgc` / `psp2cgc.exe` on `PATH`. If it is installed elsewhere, set:

```powershell
$env:PSP2CGC = "C:\path\to\psp2cgc.exe"
```

Then rerun:

```powershell
cmake ..
cmake --build . -j 8
```

The compiled `.gxp` files are embedded into the Vita executable as binary objects.

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
