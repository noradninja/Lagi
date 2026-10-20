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
- Hardware confirmation of level/stat advancement and result presentation;
  native ability-learning announcements remain a separate incomplete path.

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

## Native EXP growth and save compatibility

Restored BTL_A3 0606EF20 (cursor-weighted growth XP), 0606F0A2 (player
level/stat gains) and 0606F002 (four cumulative proficiency levels). The
12-byte rows remain in the live COMMON overlay at 0020701C: no threshold
or copyrighted table is committed. Signed/unsigned byte coefficients,
fixed-point rounding, growth-counter reset, HP/BP caps and current HP/BP
increases follow the native path. Result states 3..5 now show level-up
text/stat gains and advance to dismissal instead of the unimplemented branch.
The original rating calculator and ability-learning announcements are still
incomplete; this does not claim every results-screen effect is restored.

The omitted growth fields use an appended 48-byte typed stats extension in
the generated common header; all Vita consumers share it. Existing fields
retain their prior offsets. Save version 10001 includes those counters.
Version 10000 saves are checksum-validated, expanded in memory with zero
growth history, and rechecksummed. Their inventory, existing stats, battle
results and button configuration remain intact. Past growth weighting cannot
be reconstructed from old saves because it was never recorded. Reads do not
rewrite disk files. The existing temporary-file/rename write path is retained.

`save_growth_migration_test.cpp` checks byte preservation, current-version
roundtrip and corrupt/truncated rejection. `battle_growth_test.cpp` executes
the production growth code with synthetic rows to test weighting, multiple
levels, limits, reset and level cap. These are not on-device save acceptance
or proof of the pause-menu layout. Keep a backup before testing new saves;
older builds cannot read version 10001.

## Boss-entry crash after strand draw restoration

The 2026-10-20 12:43 recording/log and partial core
`psp2core-1792514601-0x003794398b-eboot.bin.psp2dmp.tmp` reach boss battle
after Start skips E011. The gzip expands to 9,062 bytes: no complete stack
or module inventory. Available thread entries establish a +4E000 runtime
relocation, independently matching the SCSP worker and render-thread entry
addresses in the supplied executable. PC 811BCC9C maps to linked 8116EC9C,
`smokePuff_draw`; LR maps to its typed Draw wrapper. The fault instruction
is `ldrh r3,[r3,#4]` with r3=0, reading the null VRAM allocation's base.

This was introduced by restoring the previously skipped strand draw without
also restoring its factory's allocation assignment. Native 0605A774..780
loads `dramAllocatorEnd[0]`'s file owner/allocation and assigns task +4.
The formation parent has no allocation to inherit. The generated spawn now
performs that explicit assignment; no renderer-side fallback, guessed bank,
or suppression of the draw was added. Start-skip flow is unchanged.

The strand fixture now extracts and executes the actual generated factory
with an allocation-less parent, then draws the result. Original-disc checks
cover both the allocation instructions/global and draw constants. This closes
the earlier test gap, which assumed an already valid allocation. Strand and
death-bile/trail fixtures pass; Vita boss-entry acceptance remains pending.

## Status-menu current EXP and next-level bar

Restored the independent `draw level curve` stub in the generated status
menu. MAIN 0601A05E..19C prints LV at (3,36), current EXP at (3,44), NEXT at
(3,46), and submits the authored polygon at x=-113 with an 11-pixel height.
Its fill is floor(63 * (XP-currentThreshold)/(nextThreshold-currentThreshold)),
using the same live COMMON 0020701C 12-byte rows as battle progression.
Command 1004, PMOD 04C0, color 8256, NTSC/PAL vertical placement and sorting
depth match the native caller/0602CAF4 submitter. No replacement font, layout
shift or world-space rendering is involved. Level 99 omits NEXT/fill; the
existing Floater branch is unchanged. Out-of-interval legacy data is clamped
for display only and never changes player EXP or level.

`tests/menu_growth_test.ps1 -DiscImage <disc>` checks original instruction
anchors and executes the production helper for empty/partial/full progress,
next-level boundaries, max level, NTSC/PAL, text positions and command bounds.
Hardware menu appearance remains pending. The menu can be checked directly
after loading, without entering the boss battle.

### First-charge invalid impact pose (1792520328)

The matching ELF and two thread-entry anchors establish a runtime slide of
0x2F000. Fault PC 810C596A resolves to READ_BE_S32, with its caller in
sStaticPoseData construction and pose index 02CC. This is distinct from the
previous missing strand VRAM allocation. The charge-impact adapter incorrectly
treated SH-2 instruction 76BC at 0605A23A as an unsigned addition: it actually
adds signed -68 to animation index 0210, selecting static pose 01CC.
The generated call now uses 01CC. The disc-backed regression checks the original
instruction, signed calculation and generated caller before executing the
production model fixture. No game assets are included. Hardware acceptance is
pending; persistent mouth strands and belly/enrage presentation remain separate
investigations, not claimed fixed by this pose correction.

### Idle mouth strand offsets

BTL_A3_2 0605569E/B0/C2 use logical SHLR16 on each RNG result. The
reconstruction converted the RNG word through signed fixedPoint::toInteger,
biasing all three offsets for half the RNG range. Restore ordered unsigned
draws, retaining the original live-battle emission predicate and spring task.
The strand fixture checks native instruction anchors and low/middle/high RNG
values against the authored position ranges. Battle VPK compilation and the
fixture pass; visible persistent mouth strands still need hardware confirmation.

### Authored two-shape boss deformation

Restore omitted BTL_A3_2 06055BEC -> 06054E38 -> MAIN 0601507A.
The caller targets sub-model +98, uses descriptors 060A9538/540 (hierarchies
000C/0010, poses 01C0/01C4), and supplies the existing m288[2] spring value.
The shared native two-source service reproduces 16.16 radius/translation and
truncated 2.14 vertex/normal/color blends. It leaves bone 0, animation rotations
and scales unchanged, and ignores native spring overshoot outside [0,1].
Changed mesh snapshots are invalidated individually; already-published frames
retain immutable data. No whole-actor scale, new animation timing or replacement
shape is introduced. Disc-backed fixture and battle VPK pass; swelling and mouth
deformation appearance remain hardware-pending.
