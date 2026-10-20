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

## Wall-impact animated model restoration

The wall-impact caller 0605A1D0 calls the animated-model factory 06062EAA,
not the bone-debris factory. Its original arguments select owner slot 8,
hierarchy 0018, animation 0210 and pose **02CC** (R6=0210+00BC).
The retained comment incorrectly said 01CC and substituted bone debris.
The generated adapter now restores the animated model with a copied world
transform, native YXZ rotation, normal model draw and animation-end lifetime
(06062F5E). Task deletion releases only its model instance, not bundle assets.
This restores that specific omitted task, not all smoke/death effects.
`tests/battle_impact_model_test.cpp` exercises the production task, including
resource IDs, copy semantics, completion and failed-init cleanup. The earlier
suspected pose-stride issue was ruled out: sPoseData still has the native B4
layout. No speculative shared debris patch was made.

## Remaining work

- Other wall-impact mesh/debris effects still need hardware comparison.
- Cinematic clouds and missing rock-break/monster SFX: trace native scheduled
  callbacks, resource owners and sound events before declaring audio channels
  missing or adding hard-coded sound cues.
- EXP level/stat advancement and its result/pause presentation.

## Results remain lit until dismissal

Per the requested presentation, the generated result entry no longer starts
the A94A color offset, forces falloff to -31, or interpolates scene lighting
to zero ahead of the rating text. Its neutral fade reset, reward timing and
camera remain intact. This is an intentional presentation change: original
BTL_A3 0606F752 does request the early offset, not evidence of a missing call.
The existing BattleEnd task retains ownership of the black exit fade after
input/timeout. The two dismissal conditions now converge so simultaneous
button and timeout cannot create duplicate exits or advance past phase 7.
The generated-phase fixture checks both entry and actual dismissal code.
Visual timing still needs device confirmation.

## Death bile and descriptor-driven trails

Restored the omitted 0605A4EC factory and death caller 06058C8A..96:
speed 1000, mode 1, source +2E4, two native RNG samples, pitch in
[-08000000,0), full-turn yaw, and 120-frame lifetime. The trail descriptor
060A9624 supplies the VRAM-owner slot; the task does not assume its parent's
allocation. Mode 1 selects the native per-segment Gouraud draw. Its history
allocation is released when the new task is deleted.

Shared trail drawing now follows 060A0EAA/060A0DE4: width table at descriptor
+0C, optional Gouraud table at +10, one width/color block per segment,
texture/CLUT relocation and original segment order. Removed the fabricated
800-width taper and the incorrect inline color reads at descriptor +18.
No replacement textures, spawn cadence or invented light were added.
Production-code fixtures `battle_trail_draw_test.cpp` and
`arachnoth_death_bile_test.cpp` pass. Experimental full-resolution Vita build
compiles; actual death strands/colors require hardware confirmation.
`tests/arachnoth_bile_test.ps1 -DiscImage <original-disc>` additionally checks
the death call, RNG constants, lifetime and both native draw dispatches
directly against the original disc. Those checks, the result-dismissal fixture,
and E011 scheduled-effect production/native-anchor tests pass together.
