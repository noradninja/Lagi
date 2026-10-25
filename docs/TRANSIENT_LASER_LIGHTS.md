# Transient laser point lights — feature branch
Base: `feature/first-section-cleanup`; branch: `feature/laser-point-lights`.

## Scope and invariants
- **Hard maximum four simultaneous transient laser point lights**, shared across field and battle lasers. The accepted item-collection special light remains separate and unchanged.
- `include/lagi/lagi_transient_laser_lights.h` is the bounded, allocation-free producer payload. Each native laser task uses a stable identity; repeated updates replace the same slot. Excess sources are deterministically ignored (no eviction/popping).
- Azel determines native beam positions, authored colors, lifetimes, and movement. No independently simulated paths, invented tint or texture-ID classifications.
- Preserve the existing SH7604-style point evaluator, model-space normal convention, Gouraud interpolation and RGB555 rounding; combine extra lights before the **single** final quantization.
- Do not switch on laser lights in combat before hardware FPS validation. Avoid per-pixel lighting, CPU geometry rebuilding, and unbounded work per polygon.

## Work stages
1. **Field-only proof:** trace `s_LCSLaser::LaserBeamUpdate`, `LaserHomingUpdate`, and the native beam draw color tables. Publish the actual last-written moving beam-head position with a sampled authored RGB color. Account for the current view/world matrix convention.
2. **Frame bridge:** collect no more than four active source identities on the game thread. Verify Azel task update/draw order, and freeze the producer payload for Neptune's asynchronous frame reader. Ensure no stale sources persist into the next frame.
3. **Gouraud consumption:** use native point-light evaluation per candidate model, reject out-of-range sources before accumulating; retain the current ordinary and collection light untouched. Trace per-source and aggregate RGB555 clipping on a bounded sample.
4. **Field hardware gate:** check moving colored light follows LCS shot, source cleanup, view/camera movement, concurrent shots, and 30 FPS.
5. **Battle follow-up:** connect native `sHomingLaserTask_Draw` node head and `sGunShotTask` trajectory/color; enforce the **same four** source cap for all shots. Profile a many-enemy battle before enabling additional attack types.

## Current status
The branch and bounded four-slot payload plus unit test are committed. **No laser lights are emitted or consumed yet.** No visual changes are expected until stages 1–3 are implemented, compiled, and tested.
