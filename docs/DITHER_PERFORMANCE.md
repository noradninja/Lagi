# Screen-space dither performance experiment

Branch: `feature/fullres-dithered-filtering`.

`LAGI_STOCHASTIC_FILTER=ON` retains the same four screen-space coordinate
offsets for decoded title, text, UI backgrounds/tiles, VDP1 sprites and
Gouraud particle sprites, and resolved Cinepak presentation. Radar-map
sprites remain point-filtered. World polygon and Cinepak reconstruction
pipelines are unchanged.

`LAGI_DITHER_LOOKUP=ON` replaces screen parity arithmetic in the two RGBA
presentation fragment shaders with a repeating, point-sampled 2x2 FP16
offset texture. Its texels encode the exact quarter-texel offsets, already
biased by -0.5. The lookup uses rasterizer WPOS, not sprite UVs. Texture unit
0 remains the source; unit 1 contains the offset texture. The immutable
16-byte payload owns a GPU-mapped allocation for the renderer lifetime.
This adds a dependent texture lookup and is an experiment, not a proven win.
The raw VRAM title fallback still uses arithmetic and exact palette decode.

Configure the existing build directory to preserve all other settings:

```powershell
Set-Location E:\dev\Lagi
git switch feature/fullres-dithered-filtering
git pull --ff-only origin feature/fullres-dithered-filtering
cmake -S . -B build-fullres -DLAGI_STOCHASTIC_FILTER=ON -DLAGI_DITHER_LOOKUP=ON
cmake --build build-fullres --parallel 32
```

The startup marker is `enabled=1 ... sampling=lookup-point-v1`. Set only
`LAGI_DITHER_LOOKUP=OFF` to compare with `direct-point-v1`. Set
`LAGI_STOCHASTIC_FILTER=OFF` for the hardware-bilinear reference.

Use identical stationary camera views with large/overlapping sprites, plus
the same full-screen menu, for all three modes. Capture equal-duration
windows after warm-up, including frame interval/vblank counts, ScenePerf,
and a visual recording. Keep resolution, gameplay state, audio, logging,
and other build switches unchanged. Save each log before another run.

The acceptance gates remain no measurable dither penalty against bilinear
and sustained 30 fps in full-screen menus. The existing bilinear menu drop
must also be resolved; reducing parity arithmetic does not prove either
gate. Current shader compilation and CPU model tests cannot establish Vita
sampler behavior, visual equivalence, or GPU performance.

Run the CPU model/layout test with:

```powershell
powershell -NoProfile -File tests/dither_lookup_test.ps1
```
