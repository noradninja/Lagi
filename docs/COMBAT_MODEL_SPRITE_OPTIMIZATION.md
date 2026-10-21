# Combat model and sprite preparation

Patergo hardware feedback shows renderer median 34.338 ms, CPU preparation
14.990 ms, object processing 4.701 ms and position sync 3.254 ms. Median
geometry is 1765 polygons / 10590 vertices, versus 992 / 5952 in the first
Queen fight. Sprite/UI CPU composition remains near 1.2 ms, but that cannot
exclude GPU sprite overdraw. Native weak-point reticles submit VDP1 sprites,
not the model polygons counted by ScenePerf.

## Dynamic upload streams

`probeGpuAlloc` uses USER_RW_UNCACHE memory. The steady-state position sync
previously alternated base, texture and Gouraud GPU buffers per vertex. It now
copies the base dynamic span contiguously, then visits texture positions and
Gouraud positions in separate passes. It writes exactly the same base vertices
and position components; UVs, colors, Gouraud payloads, static prefix and all
fallback/debug buffers remain intact. No model, animation, lighting or filtering
work is removed. The existing actual-loop fixture verifies these contracts,
consecutive frames, rebuilds and empty spans. Store coalescing is a performance
hypothesis, not a measured Vita gain.

## Sprite texture lookup

The shared UI/world-sprite texture cache previously scanned all decoded entries
for each lookup. It now uses an unordered index with an exact 64-bit key packing
all four original descriptor fields (PMOD, COLR, SRCA, SIZE). Hash collisions are
resolved by full-key equality. Indices, not vector pointers, survive vector
reallocation. Both successful allocation paths register their entries; failures
do not. The existing 16-color direct/CRAM LUT freshness check and in-place palette
refresh remain intact. Draw order, filtering, shaders and allocations on a true
miss are unchanged. No batching/reordering or particle/weak-point suppression.

## Sprite workload counters

ScenePerf now adds `spriteDraws`, `spriteWorldTagged`, `spriteShaded`,
`spriteBboxPixels`, `spriteCacheHits`, `spriteCacheMisses`,
`spritePaletteRefresh`, and `spriteSlotSkips`. Counters reset every scene render
and accumulate across its UI passes. Draws count submitted textured quads, not
solid lines/polygons; worldTagged is the existing world-depth flag, not a complete
semantic particle count. Misses count lookup misses, including failed decodes.
The clipped AABB pixel sum includes transparent holes and overlapping quads and
is only an overdraw-pressure proxy, not true fragment coverage or GPU timing.
Slot skips expose the pre-existing 128-sprite limit without changing it. No
per-sprite clock reads or additional per-frame log lines were introduced.

The analyzer exposes counts separately as WorkloadMedian, with null for older
logs rather than inventing zeros. Tests cover descriptor fields/lookup misses,
4096 reallocation-safe indexed entries, clipped coverage, resets, LUT freshness,
the actual upload loop, missing and positive counter attribution. Both native
battle and normal fullres/dither Vita VPKs compile. On-device gains and visual
correctness remain pending; none of these host checks proves a frame-time win.

```powershell
cd E:\dev\Lagi
git switch feature/battle-mode-bringup
git pull --ff-only
cmake --build build-battle-vita -j 32
```

Keep the existing fullres/dither and render-bridge experiment settings. Any
representative effects-heavy battle is useful; exact random encounter repeats
are not required. Record slowdown moments and retain the log, checking sprites,
radar palettes, particles and debug modes too. Compare timing versus workload
within/across captures, not enemy names alone.
