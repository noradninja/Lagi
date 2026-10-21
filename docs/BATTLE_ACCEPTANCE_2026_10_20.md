# Latest battle acceptance and follow-up

User confirms f138a83's compositor and previously missing water coverage work
on hardware. Preserve that route. Target renderer time is ideally 25 ms,
maximum 33 ms; builds and unit checks do not establish either deadline.

## Priority queue

1. Recover performance without removing restored effects/water.
2. Post-boss Edge dialogue must be a subtitle, not title-card presentation.
3. Remove weak-point indicators when the boss dies.
4. Depth-test cinematic dirt sprites against geometry.
5. Pre-boss cinematic must use its native boss music.
6. Restore Energy Prism trails, light and camera sequence.
7. Title Load must fade to black without displaying overwritten VRAM.

Each issue receives a separate commit. No effect-specific fake positions,
blanket desaturation or renderer-side gameplay decisions.

## Performance baseline and first optimization

The supplied log has 131 mode-4 non-menu ScenePerf samples: median render
30,412 us, p90 36,085 us, maximum 39,450 us; 19 exceed 33,000 us. Median CPU
preparation is 9,180 us and finish wait 19,328 us. Mode-1 non-menu samples have
median render 26,286 us with the new compositor inactive. Finish wait includes
scheduling/queued rendering; these overlapping counters cannot be added or
interpreted as an isolated GPU timestamp. Heartbeat samples include transitions
and results, not an exact-route controlled benchmark.

Specialize only the published RBG format CHSZ=16, CHCN=8bpp, bitmap off,
one-word/flip-enabled pattern names. The variant includes the generic shader
and fixes only these four format constants, eliminating unused 4bpp/8x8 and
alternate-name branches. Rotation, coefficients, screen-over, wrapping, tile
flip, palette, window and compositing math remain shared. Other formats and
the frontend continue using the generic program. Each program's uniform
offsets are reflected independently and restored when switching routes.

Skip the final-target clear only when the compositor will overwrite every
pixel; its VDP1 input surface still clears. Legacy routes retain their clear.
This removes redundant transfer/finish work without exposing old pixels.

The specialized GXP is 7,980 bytes vs 9,460 for generic; binary size alone is
not a frame-time result. Exhaustive format eligibility and existing composition
tests pass. Hardware timing recovery remains unmeasured. A fresh log should
include `[RBG0TileFormat] specialized8bit16=1`; retain compositor ON for the
water/combat comparison.

## Post-boss subtitle presentation

Native 060590AE..B6 passes the speech descriptor to 0606D68A, not the boxed
attack-name helper. 0606D7A4 writes directly to text region (3,25,38,4);
0606D9D0 clears that region at timer completion. Restore this text-only task
for the post-boss descriptor, keeping its original index/duration and leaving
other battle announcements unchanged. Parent-task teardown also clears the
subtitle. This changes presentation, not speech playback or its audio clock.
The disc-backed fixture verifies native region/call anchors and descriptor,
then runs the production task through display, duration and cleanup. Host
fixture and battle VPK pass; hardware positioning remains pending.

## Boss target retirement

Retire all three Arachnoth form target lists at death-state entry using the
existing native `0x40000` retirement flag and distance-sort invalidation.
Previously this cleanup ran only after the death dialogue finished. This is
an intentional lifetime correction after Azel decides death, not a recovered
original call at that address. The generic cursor task already stops drawing
when this flag is set; no renderer-specific boss filtering is added. Retain
the later cleanup as an idempotent safety net. The host fixture exercises all
three lists and repeated cleanup. Host test and battle VPK pass; device
confirmation of indicator removal remains pending.

## Compositor register-decision lookup

The follow-up log currently at the supplied path includes
`[RBG0TileFormat] specialized8bit16=1 genericFallback=0`. The earlier statement
that specialization was unconfirmed must not be treated as evidence that it
failed. Compositor-active steady ScenePerf samples remain over budget; their
finish waits include queued GPU work, not isolated per-pass timestamps.

Move register-only priority, selector, condition and ratio decisions from the
full-screen fragment program into a 256-entry RGBA8 point-sampled lookup.
Build it from the same frame's published Azel state after previous GPU work
finishes. Retain per-pixel RBG presence, final source/destination colors,
RGB555 quantization, exact /32 ratio and saturating addition on GPU. No change
to water coverage, mesh stipple, geometry or scene eligibility. Adds 1 KiB
payload (allocator granularity may be larger) and one dependent texture sample;
reduces per-fragment selector arithmetic/branches. This is a performance
candidate, not a measured speedup: device timing is still required.

The exhaustive host fixture compares every alpha byte across all conditions,
thresholds, background priorities and rotated bank priority/ratio values against
the previous shader's independent decision oracle. Existing RGB555 ratio tests
also pass. Expected new device marker: `register decision lookup ready`.
