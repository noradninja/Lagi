## Unified point lighting and battle producers — experimental

- The native A11D **primary** point-light selector also uses the shared
  four-corner evaluator when its scene view is available. Its original
  model-origin directional contribution is suppressed in that case, avoiding
  double illumination. Scenes without a captured native view retain their
  original model-origin fallback.
- `include/lagi/lagi_corner_point_light.h` now owns the common four-corner
  evaluation. Native collection/environment light and up to four transient
  shots call it from the *same* Gouraud corner loop, accumulating into one
  RGB value and quantizing once; no per-light GXM rendering pass.
- Battle homing lasers publish Azel's updated `m10_laserPosition` and native
  VDP1 trail color. Gunshots publish Azel's `m14` segment position and
  weapon-specific VDP1 color table. Both enter the same frame-latched four-slot
  source pool. The attached 2026-10-26 log proves four stable task identities
  update to distinct positions on successive frames. Their added illumination
  is now 25% of the authored projectile RGB (`F8F880` becomes `3E3E20`) while
  the projectile sprite remains untouched. Visual acceptance remains pending.
- Sky-sampled ambient was removed after the 2026-10-26 device review. Native
  authored depth ambient remains unchanged in field, battle and cutscenes;
  RBG0 is still rendered as scene background. The early GPU finish formerly
  used to sample sky color has also been removed. The previously measured
  frame times do not establish a speedup from this removal; capture the new
  build on the same routes.
- All changes are experimental until VitaSDK compilation and side-by-side
  hardware logs/footage verify field pickups, walls, multi-shot battle
  effects and performance. Before shipping, scrutinize 30 FPS frame pacing
  during dense combat and ensure no transient lights persist between modes.
- Run `tests/field_laser_point_light_test.ps1` for source assertions.
  The current commit has not been VitaSDK-compiled by this assistant.

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

## 2026-10-25 hardware diagnostic follow-up
The new Vita log proves field beam heads and source colors are emitted, but none
of the bounded native model evaluators run during those shots. Azel updates
laser tasks after the earlier field model submissions, so the live per-frame
pool was cleared before any model could consume the previous frame's beams.
`begin_frame()` now promotes the completed producer pool into an immutable
one-frame-late model-light input, and clears only the new producer pool.
This preserves four sources, prevents long-lived stale lights, and keeps the
accepted collection special light independent.

The original `s_RGB8::toU32()` packs BGR rather than the RGB contract of
`record_transient_field_laser_light`. The adapter now explicitly packs the
native m0/m1/m2 channels into 0xRRGGBB. Neither fix changes the native
point-light distance evaluator or Gouraud interpolation.

**Pending hardware gate:** verify `[FieldLaserEval]` and
`[FieldLaserLight]` markers appear for consecutive shots; check sprite/light
tracking, color, and 30 FPS. An evaluation can still reject out-of-range
models. VitaSDK compilation and new device tests have not been performed.

## Per-corner Gouraud trial (not hardware-accepted)
The field LCS shot-light path now snapshots accepted Azel source positions alongside
model lighting and evaluates the **original native point-light function at each
quad corner**. The point's displacement and attenuation are calculated in world
space from the transformed corner position, then its vector is rotated into
the model-normal basis. This retains normal-facing diffuse terms, shot colors,
the four-source cap, and RGB555 quantization. Native collection-item lighting
is unchanged. Crucially, the model-origin range result is no longer an exclusive
culling rule for laser sources: a corner near a beam may contribute even if
its whole mesh's origin is outside range.

The renderer first makes a cheap 64-bit squared-distance check to avoid running
the native root/divider on remote corners. The existing `[FieldLaserCoverage]`
diagnostic now uses per-corner light vectors when reporting facing normals.

**Performance risk:** Each in-range source can require one fixed-point evaluator
per Gouraud corner, up to four sources per polygon. Check battle-independent
field frame time and 30 FPS on Vita before treating this as final. No VitaSDK
build or hardware validation has been run for this trial. Restore the model-origin
path if it causes unacceptable frame-time regression.

## Current status
The branch now includes an **experimental field-only end-to-end integration**:
- The LCS beam update publishes the last-written `m6C` head position and native LCS color index (`m27`) while the beam is active. Source identity is the actual LCS task address; the four-source pool is reset each frame.
- The bridge transforms each source's native world position through the captured field camera and evaluates the unchanged fixed-point point-light routine once per submitted model. Out-of-range and overflow cases are skipped. Model submission snapshots own the result across asynchronous rendering.
- The renderer adds up to four model-space point contributions before the existing Gouraud RGB555 clamp and quantization, preserving the baseline collection light and directional setup.
- Run `tests/field_laser_point_light_test.ps1` for source-level checks and `tests/transient_laser_light_test.cpp` as a host-native unit test.

**Not yet validated:** VitaSDK compilation, runtime illumination, correct color interpretation of the native LCS palette, shot/draw ordering, multiple simultaneous beams, and frame-time impact. Battle lasers remain disconnected. This is not yet a hardware-accepted rendering change.

## Post-validation lighting debug views

After generalized per-corner point lighting and battle projectile lighting
have passed Vita hardware validation,
provide two **display-only** lit diagnostics:

- **Lit Grayscale**: evaluate the *identical* accumulated lighting as normal,
  then convert the final lit output to luminance for visual inspection of
  attenuation, vertex-normal response and Gouraud interpolation.
- **Lit Color**: retain the existing complete native RGB lighting output.

Do not implement two independent light evaluators or quantize grayscale at an
earlier stage; otherwise this ceases to be a like-for-like comparison.
In the existing viewer/debug cycle, place **Lit Grayscale immediately before Lit Color**
(the transition is Grayscale → Color). Preserve Lit Color as the default display
when entering the viewer; cycle order must not silently change the default.
Do not enable the grayscale view as a shipping visual mode without explicit
approval.

## 2026-10-27 gradient and red collection-class correction

The capture paired with `lagi.log` showed the field shot drawn blue/cyan while
`[FieldLaserEmit]` reported `rgb=31,3,0`. The producer had sampled only
`m9C0[0][0]` and interpreted the VDP1 word in the opposite channel direction
from the rendered effect. The field light now averages all 32 RGB555 vertices
in the live eight-row LCS gradient, then expands that average to a saturated
31-channel peak. Position, range, falloff, four-source limit, and the native
per-corner evaluator are unchanged.

Native collection color class `2` covers the red-class pickups, including
Blast Chip, Dual Blast Chip, and Tri Blast Chip. That entire class now uses the
live dragon-stat red endpoint from the field LCS table. The other six
collection classes retain the accepted peak-preserving saturation path.

The battle homing-laser trail uses a distinct RGB555 channel contract. The
field-gradient correction must not reverse its red/blue interpretation:
`battle_gouraud555_hue_at_peak` preserves the previously accepted combat
mapping while the field beam continues to use its full-gradient average.
The combat-enabled Vita build compiles this producer; visual acceptance still
requires a fresh multi-laser capture on device.

`tests/field_laser_point_light_test.ps1`, the focused Vita C++ syntax check, and
a clean full-resolution Vita configure/build passed. The build linked
`lagi.velf`, created `lagi.self`, and packaged `Lagi.vpk`. This is compile and
package validation only: a new Vita/Vita TV capture must confirm the field
average follows the visible laser, every red-class pickup is red rather than
pink, the other collection colors remain accepted, and field/battle frame time
still meets the existing target.

