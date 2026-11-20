# Building Lagi

**Updated 20 November 2026.** These commands describe the current native A3/WORLD development configuration, not the October flight-only profile.

## Requirements

Windows 11, CMake, Ninja (or Rider), VitaSDK, Sony `psp2cgc.exe`, Git and your own Panzer Dragoon Saga CUE/BIN images.

The known home layout is `E:\dev\Lagi`, `E:\dev\VitaSDK` and `E:\PSVITA\sdk\host_tools\bin\psp2cgc.exe`. The work-machine layout uses `C:\Dev\Lagi`, `C:\Dev\VitaSDK` and `C:\Dev\sdk\host_tools\bin\psp2cgc.exe`.

## Update the checkout

After merging the development branch, `main` is the integration baseline:

```powershell
Set-Location E:\dev\Lagi
git status --short
git switch main
git pull --ff-only origin main
git -c core.longpaths=true submodule update --init --recursive
git rev-parse --short HEAD
```

Preserve local edits before switching. The Azel submodule stays pinned; Vita-specific changes are maintained outside it.

## Configure and build

```powershell
$env:VITASDK = 'E:\dev\VitaSDK'
$env:PSP2CGC = 'E:\PSVITA\sdk\host_tools\bin\psp2cgc.exe'
$env:PATH = "$env:VITASDK\bin;$env:PATH"

cmake -S . -B cmake-build-vita-release -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE="$env:VITASDK/share/vita.toolchain.cmake" `
  -DLAGI_FULLRES=ON -DLAGI_STOCHASTIC_FILTER=ON -DLAGI_DITHER_LOOKUP=ON `
  -DLAGI_AZEL_PROBE=ON -DLAGI_AZEL_RUINS_LINK_TRIM=ON `
  -DLAGI_BATTLE_NATIVE_BUILD=ON -DLAGI_BATTLE_EXECUTE_A3=ON `
  -DLAGI_WORLD_MAP_BRINGUP=ON -DLAGI_VDP2_SPRITE_COMPOSITE=ON `
  -DLAGI_RBG_DECODED_PAGES=ON -DLAGI_RBG_COEFFICIENT_ROWS=ON `
  -DLAGI_OPTIMIZE_NEPTUNE=ON -DLAGI_OPTIMIZE_RENDER_BRIDGE=ON `
  -DLAGI_BATTLE_LINK_AUDIT=OFF -DLAGI_GPU_PASS_TIMING=OFF

cmake --build cmake-build-vita-release --parallel 8
```

Use the corresponding `C:\Dev` paths on the work machine. Rider may reuse this configured directory. If `cmake --build` reports a missing `CMakeCache.txt`, configure the directory first. A generated Ninja packaging target is commonly `Lagi.vpk-vpk`; an all-target build also packages the VPK.

## CMake defaults on the current tree

| Option | Default |
| --- | --- |
| `LAGI_FULLRES` | ON |
| `LAGI_STOCHASTIC_FILTER` | ON |
| `LAGI_DITHER_LOOKUP` | ON |
| `LAGI_AZEL_PROBE` | ON |
| `LAGI_AZEL_RUINS_LINK_TRIM` | ON |
| `LAGI_BATTLE_NATIVE_BUILD` | ON |
| `LAGI_BATTLE_EXECUTE_A3` | ON |
| `LAGI_WORLD_MAP_BRINGUP` | ON |
| `LAGI_VDP2_SPRITE_COMPOSITE` | ON |
| `LAGI_RBG_DECODED_PAGES` | ON |
| `LAGI_RBG_COEFFICIENT_ROWS` | ON |
| `LAGI_OPTIMIZE_NEPTUNE` | ON |
| `LAGI_OPTIMIZE_RENDER_BRIDGE` | ON |
| `LAGI_BATTLE_LINK_AUDIT` | OFF |
| `LAGI_GPU_PASS_TIMING` | OFF |
| `LAGI_FIELD_MOTION_TRACE` | OFF |
| `LAGI_RECORD_AZEL_INPUT` | OFF |

Build directories retain cached options even after the project defaults change. When two machines behave differently, inspect their caches or configure fresh directories with the same flags. This was a real source of missed runtime features and performance changes during camp/battle testing.

`LAGI_FULLRES=ON` selects 960×544 gameplay. Lower-resolution rendering remains useful for comparisons; do not compare frame times between different resolution or filter settings as though they were the same workload.

## Device testing

Place your own discs under `ux0:data/lagi/Disc 1/` through `Disc 4/`, with matching `.cue` and `.bin` files. The latest runtime log is `ux0:data/lagi/lagi.log`. For crashes, archive that log, the video, the core dump and the matching unstripped `lagi.velf`/executable together.

Our main authored test path runs from the title and Ruins through flight, Excavation, Valley, Canus/camp and Garil. [Current status](STATUS.md) records which steps have hardware acceptance. A completed VPK is only a build result, not a runtime or rendering acceptance result.

Shaders are in `shaders/`. Portable and original-data fixtures live in `tests/`; logging/performance tools live in `tools/`. The SCSP DSP runtime ARM backend is the default and does not require a `dsp_backend.txt` override.
