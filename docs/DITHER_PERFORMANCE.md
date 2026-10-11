# Accepted screen-space dither performance checkpoint

Accepted source checkpoint: `508bf7544f4847726d2b758a5872c7c25077ee3c`, from `feature/fullres-dithered-filtering`, integrated into main on 2026-10-10.

The supplied Vita log identifies `[DitherFilter] enabled=1 pattern=screen-2x2 sampling=lookup-point-v1`. The user reports that large sprites no longer cause the additional frame loss seen with arithmetic dithering and accepted this implementation as effectively free versus hardware bilinear in tested gameplay. This is user hardware acceptance, not a claim of identical matched-frame GPU timings or universal zero cost.

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
This adds a dependent texture lookup; its performance benefit is accepted on the user's tested Vita route, not inferred from shader binary size.
The raw VRAM title fallback still uses arithmetic and exact palette decode.

Configure the existing build directory to preserve all other settings:

```powershell
Set-Location E:\dev\Lagi
git switch main
git pull --ff-only origin main
cmake -S . -B build-fullres -DLAGI_FULLRES=ON -DLAGI_STOCHASTIC_FILTER=ON -DLAGI_DITHER_LOOKUP=ON
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

The original target included no dither penalty and sustained 30 fps in full-screen menus. The user accepts the dither-specific result, but reports that full-screen menus still fall below 30 fps just as they do with hardware bilinear. The menu target is therefore unresolved and separate from this accepted merge. Do not describe it as fixed. Shader compilation and CPU model tests passed, including renderer syntax in lookup, arithmetic and disabled configurations. The final merged main package has not been rebuilt or hardware-tested in this session; user hardware evidence applies to the accepted source checkpoint above.

Run the CPU model/layout test with:

```powershell
powershell -NoProfile -File tests/dither_lookup_test.ps1
```
