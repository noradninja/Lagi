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

The current native GXM viewer shaders are readable Cg sources under `shaders/` and are compiled during the build with Sony's `psp2cgc`. This includes the color/debug, texture, Gouraud diagnostic, and combined textured-lighting programs. CMake embeds the resulting GXP binaries with the active VitaSDK toolchain's `objcopy`.

The current Windows development setup finds `psp2cgc.exe` at `E:/PSVITA/sdk/host_tools/bin`. Other setups can expose it through PATH, `PSP2CGC`, or `SCE_PSP2_SDK_DIR`.

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
- L / R: cycle viewer mode:
  - 0: textured baseline
  - 1: textured + RGB555 Gouraud lighting, CW culled
  - 2: Gouraud grayscale diagnostic
  - 3: polygon debug colors
  - 4: wireframe

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

- Shader embedding uses CMake's `CMAKE_OBJCOPY` from the active VitaSDK toolchain rather than requiring `arm-vita-eabi-objcopy` to be present on the Windows shell PATH. This keeps the textured-shader build compatible with the existing VitaSDK CMake configuration.

- Windows VitaSDK's GNU objcopy treats `--input` / `--output` as ambiguous. Shader embedding now uses the canonical BFD flags `-I binary -O elf32-littlearm -B arm`, which are accepted by the bundled `arm-vita-eabi-objcopy.exe`.


## Combined textured-lighting hardware test

After pulling the current milestone, rerun `cmake ..` once because two additional Cg shaders are generated and embedded. A successful build should show compilation/embedding steps for `textured_lit_v.cg` and `textured_lit_f.cg`.

On hardware, compare viewer mode 0 directly with mode 1. Geometry scale, projection, UV orientation, transparency cutout, and texture selection should remain identical. Only the interpolated lighting modulation should change. The main visual inspection targets are the broad wing surfaces, diagonal interpolation across the two-triangle Saturn-quad split, and seams between hierarchy/model sections.


For the wing-lighting diagnostic, compare modes 1 and 2 at the same camera angle. If the triangular/inverted-looking wing patches disappear in mode 2, the issue is back-face visibility rather than a mismatch in Saturn quad corner order. If they remain on front-facing surfaces, the next step is to instrument stored-normal direction versus geometric face normal per quad.

## Vita presentation assets

The VPK packages `sce_sys/icon0.png`, `sce_sys/livearea/contents/bg0.png`, `startup.png`, and `template.xml` automatically. No separate asset-copy step is required after pulling the current branch.


## RGB555 / bilinear lighting hardware test

The current textured-lighting mode now draws each Saturn quad separately so five fragment uniforms can carry the original four screen-space corners and four per-corner RGB Gouraud values. This is intentionally less draw-call-efficient than the texture-only batch path; it is the correctness/reference implementation before optimization.

After pulling this milestone, rerun `cmake ..` once because the textured-lighting shader interface changed and the old scalar-light vertex shader is no longer generated. Then rebuild normally.

Compare textured baseline mode 0 against lit modes 1/2. On the wings, the previous diagonal half-quad lighting wedges should disappear or be substantially reduced because lighting is evaluated from a single bilinear quad coordinate instead of triangle varyings. The light should also visibly step in Saturn-style 5-bit increments. Mode 2 still enables CCW culling for comparison; mode 1 remains two-sided.


For the current wing-winding test, viewer mode 2 uses CW culling. Compare it against mode 1 at the same camera angle. If the correct wing membrane faces remain visible and the inverted-looking flats disappear, CW is the effective front-face winding for Lagi's current GXM projection path and can replace the temporary diagnostic.


CW culling is now the hardware-validated default for the textured Saturn lighting path. The earlier two-sided/culling comparison mode is no longer part of the normal viewer cycle.


## Camera-relative Azel light test

Modes 1 and 2 now recompute lighting every frame from the Basic Wing menu's Azel light defaults. The test light remains fixed in camera space, so rotating the dragon with the left stick should visibly move the 5-bit highlight/shadow pattern over the model. Mode 1 applies those values additively to the texture; mode 2 displays the same quantized result as grayscale.

The standalone viewer reproduces Azel's light color, setupLight vector convention, 32-entry falloff table generation, RGB channel ordering, 5-bit clamp/quantization, and additive Gouraud representation. The viewer maps its current view-space depth into the dragon menu's 16-unit far-clip falloff range; live field/battle camera/light state is still a later integration step.


All solid viewer modes now use the hardware-validated CW front-face culling. Wireframe remains uncullled by design. When comparing modes 0-3, geometry visibility should therefore remain consistent and only the rendering/debug presentation should change.


## RGB555 output banding test

Modes 1 and 2 now quantize the final interpolated lighting result back to 5 bits per channel before display. The Vita framebuffer remains RGBA8888, but the shader only emits values corresponding to the Saturn's 0..31 channel grid.

On broad, smoothly lit surfaces the highlight/shadow transition should therefore resolve into visible color bands rather than an 8-bit-smooth gradient. Mode 2 is the easiest validation view because the same quantized Gouraud values are displayed as grayscale without texture detail.


## Morph-screen animation and detail zoom

The Basic Wing viewer now attempts to decode and play the same default flap animation used by Azel's dragon morph screen (Basic Wing animation table entry `0x10C`). A successful boot shows `[PASS] DRAGON0 MORPH FLAP ANIM`; if the native animation data fails validation, the viewer falls back to the static pose and reports `[INFO] DRAGON0 MORPH ANIM STATIC`.

The animation is displayed at 30 Hz while the viewer continues presenting at 60 Hz. Animated positions and normals feed every viewer mode, including the camera-relative RGB555 Gouraud calculation.

Right-stick Y still controls camera dolly, but the near zoom limit is now `0.75` instead of `1.4`. Triangle still resets yaw, pitch, and distance to the normal `3.0` overview.


The morph-screen flap now runs from an independent 30 Hz clock rather than one animation step per two rendered frames. For hardware validation, zoom from the normal overview into the closest detail range: animation playback speed should remain unchanged even if the renderer falls from 60 FPS to roughly 30 FPS.


## 30 Hz viewer cap

All Basic Wing viewer modes are now capped to one presented frame every two Vita vblanks. Texture-only, polygon-debug, wireframe, RGB555-lit, and grayscale-lighting modes should therefore have the same camera/input rate. The cap is vblank-relative rather than a blind two-vblank sleep, so a more expensive rendered frame that already spans one vblank only waits for the remaining presentation slot.


## Hardware-tested graphics baseline — 2026-09-28

At the end of this session the expected Basic Wing viewer baseline on Vita hardware is:

- 960x544 native GXM output, fixed 30 Hz presentation.
- Right stick Y can dolly from the normal 3.0 overview down to 0.75 for close inspection.
- Basic Wing morph-screen flap animation plays at an independent 30 Hz and must not slow when rendering becomes more expensive.
- Mode 0: original decoded Saturn texture baseline.
- Mode 1: decoded texture + quad-bilinear RGB555 Gouraud lighting + final RGB555 output quantization.
- Mode 2: the same 5-bit Gouraud result displayed as grayscale.
- Mode 3: polygon debug colors.
- Mode 4: uncullled wireframe diagnostic.
- Modes 0-3 use the hardware-validated CW front-face winding.
- Visible 5-bit lighting/color banding is expected and intentional; a smooth 8-bit gradient is considered incorrect for this Saturn-faithful path.
