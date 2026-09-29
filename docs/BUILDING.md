# Building Lagi

Last updated: 2026-09-28

## Current development environment

The current hardware-tested setup uses:

- Windows 11
- VitaSDK
- CMake
- Ninja
- VitaSDK GCC/G++ 15.2
- C++20
- Sony `psp2cgc` for GXM shader compilation

Current local project path:

```text
E:\dev\Lagi
```

Current VitaSDK path:

```text
E:\dev\VitaSDK
```

Current shader compiler path used by the Windows development machine:

```text
E:\PSVITA\sdk\host_tools\bin\psp2cgc.exe
```

The repository also supports locating `psp2cgc` through PATH, `PSP2CGC`, or `SCE_PSP2_SDK_DIR`.

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

For ordinary C++ changes:

```powershell
cd E:\dev\Lagi
git pull
cd build
cmake --build . -j 8
```

If CMake configuration, shader sources, shader embedding rules, or generated build inputs changed, rerun configure first:

```powershell
cd E:\dev\Lagi
git pull
cd build
cmake ..
cmake --build . -j 8
```

## Azel source

Azel is included as a submodule under:

```text
extern/Azel
```

The current pinned reference is:

```text
c52329fb257561abff05531a8335e34137d67098
```

Lagi-owned Vita adaptation code should be modified in the Lagi repository rather than editing the pinned Azel submodule unless an upstream change is intentionally being made.

## GXM shaders

Current readable shader sources live under:

```text
shaders/
```

The current Basic Wing renderer uses:

- color/debug shaders,
- `texture_v.cg`,
- `texture_f.cg`,
- `textured_lit_f.cg`,
- `gouraud_debug_f.cg`.

The lit and grayscale paths deliberately reuse the proven texture vertex shader. There is no separate active `textured_lit_v.cg` path.

CMake invokes `psp2cgc`, then embeds generated GXP binaries with the active VitaSDK toolchain's `objcopy` using:

```text
-I binary -O elf32-littlearm -B arm
```

This avoids relying on a separate `arm-vita-eabi-objcopy` executable being available on the shell PATH.

## Game data

No Panzer Dragoon Saga game data is distributed with Lagi.

For current hardware testing, place a user's own Disc 1 CUE/BIN dump under:

```text
ux0:data/lagi/Disc 1/
```

Example:

```text
ux0:data/lagi/Disc 1/
    Panzer Dragoon Saga Disc 1.cue
    Panzer Dragoon Saga Disc 1.bin
```

The current CUE/disc path supports MODE1/2352 and MODE1/2048 data tracks and mounts the ISO9660 filesystem directly from the disc image.

Current viewer data is read from files including:

- `COMMON.DAT`
- `DRAGON0.MCB`
- `DRAGON0.CGB`

## Persistent runtime log

Each launch creates/truncates:

```text
ux0:data/lagi/lagi.log
```

The logger uses Vita-native I/O and flushes writes promptly so diagnostic information is likely to survive abnormal exits.

After testing, retrieve the file with VitaShell if a runtime/data issue needs investigation.

Useful log areas include:

- platform startup,
- COMMON.DAT parsing,
- dragon metadata,
- hierarchy/geometry validation,
- VDP1 polygon descriptors,
- texture decode statistics,
- lighting payload statistics,
- morph animation decoding.

## Expected current hardware status

A healthy viewer build should include successful status for the major runtime/data paths, including:

- Vita platform/framebuffer
- log open
- Saturn memory readers
- Disc 1 CUE/BIN + ISO9660
- COMMON.DAT tables
- sound table 79/79
- dragon COMMON data
- DRAGON0 hierarchy/hotpoints
- DRAGON0 geometry
- DRAGON0 VDP1 polygon records
- DRAGON0 lighting data
- DRAGON0 CGB references
- DRAGON0 mode-1 textures
- DRAGON0 morph flap animation
- native GXM Basic Wing rendering
- Azel root task / task loop

Exact ordering can vary as bring-up code evolves.

## Hardware viewer controls

Current development controls:

- SELECT: toggle status console / 3D viewer.
- START + SELECT: exit.
- Left stick: rotate yaw/pitch.
- Right stick Y: dolly camera.
- Triangle: reset viewer camera.
- L / R: cycle viewer modes.

Camera:

```text
default distance: 3.0
minimum distance: 0.75
maximum distance: 8.0
vertical FOV: 50 degrees
```

## Viewer modes

```text
Mode 0 — original decoded Saturn texture baseline
Mode 1 — texture + RGB555 Gouraud lighting
Mode 2 — RGB555 Gouraud grayscale diagnostic
Mode 3 — polygon debug colors
Mode 4 — wireframe
```

Modes 0-3 use the hardware-validated:

```text
SCE_GXM_CULL_CW
```

Mode 4 intentionally uses:

```text
SCE_GXM_CULL_NONE
```

Filled modes use LESS_EQUAL depth testing.

Wireframe uses strict LESS so duplicate shared-edge fragments do not fight at equal depth.

## Current texture expectations

Basic Wing currently uses 71 unique decoded textures.

The mode-1 decoder follows the original VDP1 data:

```text
CMDSRCA << 3 -> texture bytes in DRAGON0.CGB
CMDCOLR << 3 -> 16-entry LUT in DRAGON0.CGB
```

Expected visual behavior:

- nearest-neighbor sampling,
- no mipmaps,
- correct CMDCTRL flips,
- transparent dot handling,
- VDP1 end-code behavior,
- RGB555-derived source color.

Mode 0 is the clean reference for checking texture geometry and UV behavior without lighting.

## Current RGB555 Gouraud expectations

Mode 1 is the current Saturn-faithful lit reference.

The path deliberately does not use normal triangle-interpolated vertex lighting.

For each original Saturn quad:

1. the four projected source corners are retained,
2. the fragment position is mapped back into the original quad,
3. all four Gouraud corner RGB offsets are bilinearly interpolated,
4. the offset is added to the source texture in Saturn-style 5-bit channel space,
5. each channel is clamped to 0..31,
6. the result is quantized to an integer 5-bit value,
7. the 5-bit value is expanded only for the Vita RGBA8888 target.

Visible stepped color/lighting bands are expected and intentional.

A smooth modern 8-bit gradient is not the target.

## Grayscale lighting diagnostic

Mode 2 uses the same:

- animated normals,
- current camera-relative light,
- RGB555 corner values,
- inverse-bilinear quad reconstruction,
- final 5-bit quantization

as Mode 1.

It then displays the result as grayscale.

Use Mode 2 when checking:

- banding,
- quad interpolation,
- light movement,
- hierarchy/normal animation,
- lighting discontinuities without texture detail.

## Morph-screen light reference

The standalone viewer currently mirrors the pinned Azel dragon morph-viewer defaults:

```text
setupLight(0, 0, 0x10000, 0x161918)
generateLightFalloffMap(0x030102, 0, 0)
```

The light remains fixed relative to the camera while the viewer rotates the model.

The current standalone view-depth-to-falloff mapping is an adaptation for this viewer. Do not treat it as the final field/battle lighting implementation.

## Morph-screen animation

The Basic Wing viewer decodes:

```text
dragonAnimOffsets[0]
-> DRAGON0.MCB animation table entry 0x10C
```

This is the default animation selected by pinned Azel's morph-screen setup.

The decoded skeletal animation updates both:

- model vertex positions,
- transformed lighting normals.

Animation timing is independent of render timing and runs at an exact logical 30 Hz.

If rendering takes longer, playback catches up by elapsed animation ticks instead of slowing down.

Expected successful status:

```text
[PASS] DRAGON0 MORPH FLAP ANIM
```

If animation data validation fails, the viewer falls back to the static pose and reports:

```text
[INFO] DRAGON0 MORPH ANIM STATIC
```

## 30 Hz presentation

The Basic Wing viewer is intentionally capped to 30 presented frames per second.

The Vita display still scans at 60 Hz. Lagi synchronizes presentation so a new game/render frame is presented every two display intervals.

This keeps:

- texture-only mode,
- RGB555-lit mode,
- grayscale diagnostic,
- polygon debug mode,
- wireframe

on the same camera/input cadence.

Animation also runs at 30 Hz, but from its own time source rather than from rendered frame count.

## Hardware validation checklist

When changing the renderer, compare against this known-good baseline:

- Mode 0 geometry and textures remain correctly aligned.
- Modes 0-3 expose the same visible faces.
- Wing membranes retain the hardware-validated CW orientation.
- Mode 1 preserves quad-wide lighting without a diagonal triangle seam.
- Mode 1 shows visible RGB555 color banding.
- Mode 2 shows the same moving/banded light independently of texture.
- Morph-screen flap articulation remains coherent.
- Animated lighting moves with the animated hierarchy.
- Animation speed remains constant at close zoom.
- Camera movement rate remains the same in every mode.
- No partial-frame scanout tearing appears.
- Triangle reset returns to the standard 3.0-distance view.

## Known issue

One isolated stray triangle remains in the Basic Wing rendering.

Do not change the global culling convention to address it. CW has already been hardware-validated for the rest of the model.

Treat the stray triangle as a separate mesh/model-data investigation.

## Vita application assets

The VPK now packages the repository's Vita presentation assets automatically:

```text
sce_sys/icon0.png
sce_sys/livearea/contents/bg0.png
sce_sys/livearea/contents/startup.png
sce_sys/livearea/contents/template.xml
```

`template.xml` references `bg0.png` as the LiveArea background and `startup.png` as the gate/startup image.

Because the VPK packaging rules changed, rerun `cmake ..` once after pulling this milestone before rebuilding.

## Current development direction

The Basic Wing viewer is now a reference implementation, not the final game renderer.

The next graphics work should reuse its proven pieces while moving into live game state:

- original VDP1 texture decode,
- original quad identity,
- RGB555 Gouraud calculation,
- animated hierarchy/normals,
- CW solid-face convention,
- 30 Hz presentation.

Avoid replacing these with conventional smooth modern rendering simply for convenience; they are now part of the intended PDS visual target.
