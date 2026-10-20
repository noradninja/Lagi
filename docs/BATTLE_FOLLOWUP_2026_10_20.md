# Battle follow-up, 2026-10-20

Hardware report: Devour now works. Saturn reference shots show hanging
stippled textured strands, wall-impact debris/smoke and cinematic clouds.
Rock-break/monster cinematic SFX remain absent. Victory must not visibly dim
before the results; user requests dimming only when exiting by input/timeout.
World-map recovery remains separate and unready for hardware testing.

## Recovered strand draw

Upstream `smokePuff_draw` explicitly skipped its draw. Original BTL_A3_2
0605B412..464 submits its two live world-space endpoints (+28 and +4C) to
MAIN 0602E136: width 0400, source VRAM-base+0CD0, size 0510, palette
VRAM-base+2EEC, mode 0108. This is a textured stippled strip, not a model
hierarchy or replacement smoke sprite. Its draw has no light/Gouraud input.
The shared Lagi ray service emits neutral Gouraud offsets while preserving
the native texture/palette/mesh flags and existing world-depth recording.

The generated adapter now restores that draw without changing spawn timing,
simulation, endpoint attachment, shader colors or pinned Azel source.
`tests/arachnoth_drool_test.ps1` checks original-disc draw constants/dispatch
and the actual production adapter. The full Devour dispatcher test still
passes and the experimental full-resolution Vita VPK builds. Actual strand
appearance needs device confirmation. This does not restore all mesh effects.

## EXP audit: award exists, progression incomplete

The current generated `battle_battleResultScreen.cpp` does add the modified
`m474_XPReceivedFromBattle` to `mainGameState.gameStats.m20_XP` in result state
0, saturating at 99,999,999. Dyne is also awarded. Displaying a reward number
alone is not the proof; the persistent game-state addition is present.

However `battleResultScreen_checkLevelUp` and the level-up presentation/stat
branch are unimplemented, as is `battleResultScreen_updateSub5`. The rating
calculator is also a stub returning Excellent, which affects the multiplier.
Do not claim leveling works or treat the pause display as the only defect.
Native thresholds, level-dependent stats and result states must be recovered
together; no fabricated thresholds or direct level increments were added.
No new-run log was supplied with these reference screenshots, so this is a
source-path audit, not a claim about the user's exact saved EXP value.

## Still open

- Wall-impact mesh/debris effects and death-bile spawning (0605A4EC remains
  explicitly unimplemented); do not replace them with generic particles.
- Cinematic clouds and missing rock-break/monster SFX: trace native scheduled
  callbacks, resource owners and sound events before declaring audio channels
  missing or adding hard-coded sound cues.
- EXP level/stat advancement and its result/pause presentation.
- Victory dimming order. Original BTL_A3 0606F752..75C itself contains a
  C210->A94A request before rating text; the retained source reflects that.
  Need to distinguish the affected layers/native result timing from an early
  whole-screen application before moving any authored fade or lighting calls.
  No speculative timing change was included in the strand fix.
