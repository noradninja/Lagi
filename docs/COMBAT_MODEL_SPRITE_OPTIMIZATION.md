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
