# Native target selection and indicator visibility

Goal 7 follow-up to the completed-image UI-priority correction (46774df).
No extern/Azel edits; hardware angle-dependent visibility is not accepted.

## Authored quadrant eligibility already executes

Disc1 BTL_A3.PRG LBA258144, 385044 bytes, base06054000:
target-system update0605CFAC uses the dragon's current quadrant and each
target's authored mask80000000/40000000/20000000/10000000. It resets active
targets' selectable index, filters gun eligibility, sorts the native pointer
array and assigns one-based indices only to selectable entries. The generated
production source retains this gate; it is not replaced by an all-visible
Lagi targeting adapter.

The actual production update and eligibility functions now execute in a host
fixture covering2048 quadrant/state combinations, laser-only bit2, disabled
100000, non-enemy1 and retired40000, plus repeated quadrant changes and
selected-pointer/index ownership. Task transforms, distance and projection
services are mocked; this proves these decisions, not Vita world positions.

Native weak-marker update060695F0..0606970A gates on selected-target bit200000
and disabled/retired state, not directly on quadrant. The gun-selection engine
owns200000. Normal cursor update additionally uses the selectable indexm5A.
Native draws06069094/0606970C pass target+4 into visibility06068BD0, matching
the reconstruction; blindly changing that pointer to target+8 or adding a
marker-only facing rule would not be a source-faithful correction.

## Confirmed debug-projection defect corrected

Gun eligibility0605C75E..0605C8EA normally returns true after native flags and
altitude checks when debug flag1F is clear. With flag1F set it checks the
original position's near/far range and projected viewport. Native0605C8AE..
0605C8DC requires left<=X<=right and bottom<=Y<=top. getVdp1ScreenResolution
returns left/top/right/bottom. The reconstruction reversed both Y comparisons,
rejecting every ordinary interior point for the full debug-projection path.

A guarded generated-source adapter restores just those comparisons. The
production fixture failed on the previous code's interior viewport test before
the change. Coverage includes203x203=41209 viewport samples, all four
boundaries and just-outside samples, near/far equality and outside values, plus
the native non-debug projection bypass. No normal-mode camera/facing behavior
is changed. This is not claimed as the cause of the user's all-angle report.

## Open validation

Retest weak-marker overlap on46774df or later, and capture ordinary selection
while moving between quadrants. If the wrong-side issue remains, trace the
live authored hotspot flags, selectable indices, target positions and selected
flag lifecycle rather than inventing an extra visibility filter. Accepted
ordering, normal combat performance and Goals1-6 remain regression gates.

## Hardware regression and renderer depth diagnosis — 2026-11-18

User hardware acceptance: Garil queen-loss **Scatter works**. The WEAK
sprites and text remain visible through the enemy from inappropriate
angles in both Garil Desert and preceding Valley combat, after the
earlier quadrant-only marker-gating attempt. Keep Scatter untouched.

Original Disc 1 `BTL_A5.PRG` at Saturn `060B10D8` includes a
Nanyd hotspot with flags `F0000200`; the high nibble explicitly allows
all four quadrants. Therefore a target-quadrant gate cannot solve the
visibility problem for this authored hotspot.

Further source path:

- `battleTargetable.cpp::isSpriteVisible` projects the existing
  hotspot/view-space `m4_pPosition` and tests viewport and clip planes.
  It does **not** test occlusion by the 3D enemy mesh.
- The ordinary WEAK sprite and label are emitted by
  `sBaldorSubTask0_draw`, including the paired original VDP1
  textured sprites. The other two cursor draws use the same target
  position. All three previously provided Saturn bucket keys but
  **no Neptune world-depth tags**.
- `lagi_runtime.cpp` captures producer-owned
  `lagiRecordVdp1WorldDepth` metadata. Without it,
  `neptune_renderer.cpp` puts these commands in the foreground
  screen UI pass with GXM depth function ALWAYS. This is why a
  target with all-quadrant eligibility appears above the mesh
  instead of behind it.
- `neptune_renderer.cpp` routes tagged scene commands to
  WorldEffects with GXM LEQUAL and derives depth from view-space Z.
  Both field-hosted (scene mode 3) and direct (mode 4) battles
  support native scene depth.

Atomic implementation:
- `8ff9614` registers a scoped depth-tagging producer for the three
  shared cursor/WEAK draw callbacks in the generated native battle
  source. The scope only tags commands actually emitted by that
  callback using its existing target view Z. It does not alter
  target masks, flags, damage, texture IDs, or unrelated HUD sprites.
- `7362abe` adds a host fixture for multi-command tagging,
  absence of emitted commands, null target pointers, and nonpositive
  view Z. Equivalent fixture compiled and passed under GCC;
  full Vita build not yet verified.

The user's log `lagi(20261118-202354).log` confirms a
native battle-enabled build and Garil battle mode transitions, but
does not include a per-weak-marker occlusion trace. This diagnosis
combines confirmed native hotspot data and traced Neptune pass
semantics; actual occlusion remains a **hardware acceptance gate**.

Next device comparison: leave the Scatter regression alone; enter
Valley and Garil battle, orbit the dragon so the same weak hotspot
moves behind/in front of its mesh, and observe BOTH sprite and WEAK
text. Confirm markers occlude on the rear side, remain visible on the
exposed side, and do not affect ordinary targeting or other HUD UI.
Use the existing `spriteWorldTagged` scene statistics for supporting
evidence if available.
