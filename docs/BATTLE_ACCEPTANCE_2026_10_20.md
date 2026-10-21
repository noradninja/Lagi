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
